/*
 *  VobSub2SRT is a simple command line program to convert .idx/.sub subtitles
 *  into .srt text subtitles by using OCR (tesseract). See README.
 *
 *  Copyright (C) 2010 Rüdiger Sonderfeld <ruediger@c-plusplus.de>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// MPlayer stuff
#include "mp_msg.h" // mplayer message framework
#include "vobsub.h"
#include "spudec.h"

// Tesseract OCR
#include "tesseract/baseapi.h"

#include <iostream>
#include <string>
#include <cstdio>
#include <vector>
#include <cstdlib>
#include <cstring>
using namespace std;

typedef void* vob_t;
typedef void* spu_t;

/** Converts time stamp in pts format to a string containing the time stamp for the srt format
 *
 * pts (presentation time stamp) is given with a 90kHz resolution (1/90 ms). srt expects a time stamp as  HH:MM:SS:MSS.
 */
std::string pts2srt(unsigned pts) {
  unsigned ms = pts/90;
  unsigned const h = ms / (3600 * 1000);
  ms -= h * 3600 * 1000;
  unsigned const m = ms / (60 * 1000);
  ms -= m * 60 * 1000;
  unsigned const s = ms / 1000;
  ms %= 1000;

  enum { length = 13 }; // HH:MM:SS:MSS\0
  char buf[length];
  snprintf(buf, length, "%02d:%02d:%02d,%03d", h, m, s, ms);
  return std::string(buf);
}

/// Dumps the image data to <subtitlename>-<subtitleid>.pgm in Netbpm PGM format
void dump_pgm(std::string const &filename, unsigned counter, unsigned width, unsigned height,
              unsigned char const *image, size_t image_size) {
  char buf[50];
  snprintf(buf, sizeof(buf), "%s-%u.pgm", filename.c_str(), counter);
  FILE *pgm = fopen(buf, "wb");
  if(pgm) {
    fprintf(pgm, "P5\n%u %u %u\n", width, height, 255u);
    fwrite(image, 1, image_size, pgm);
    fclose(pgm);
  }
}

/// Handle argc/argv
struct cmd_options {
  cmd_options(bool handle_help = true)
    : exit(false), handle_help(handle_help)
  {
    options.reserve(10); // reserve a default amount of options here to save some reallocations
  }

  cmd_options &add_option(char const *name, bool &val, char const *description, char short_name = '\0') {
    options.push_back(option(name, option::Bool, val, description, short_name));
    return *this;
  }
  cmd_options &add_option(char const *name, std::string &val, char const *description, char short_name = '\0') {
    options.push_back(option(name, option::String, val, description, short_name));
    return *this;
  }
  cmd_options &add_option(char const *name, int &val, char const *description, char short_name = '\0') {
    options.push_back(option(name, option::String, val, description, short_name));
    return *this;
  }

  cmd_options &add_unnamed(std::string &val, char const *help_name, char const *description) {
    unnamed_args.push_back(unnamed(val, help_name, description));
    return *this;
  }

  bool parse_cmd(int argc, char **argv) const {
    size_t current_unnamed = 0;
    bool parse_options = true; // set to false after --
    for(int i = 1; i < argc; ++i) {
      if(parse_options and argv[i][0] == '-') {
        unsigned offset = 1;
        if(argv[i][1] == '-') {
          if(argv[i][2] == '\0') {
            // "--" stops option parsing and handles only unnamed args (to allow e.g. files starting with -)
            parse_options = false;
          }
          offset = 2;
        }
        if(handle_help and (strcmp(argv[i]+offset, "help") == 0 or strcmp(argv[i]+offset, "h") == 0)) {
          help(argv[0]);
          continue;
        }
        for(std::vector<option>::const_iterator j = options.begin(); j != options.end(); ++i) {
          if(strcmp(argv[i]+offset, j->name) == 0 or
             (j->short_name != '\0' and argv[i][offset+1] == j->short_name and argv[i][offset+1] == '\0')) {
            if(j->type == option::Bool) {
              *j->ref.flag = true;
              break;
            }

            if(i+1 >= argc or argv[i+1][0] == '-') { // Check if next argv is an option or argument
              cerr << "option " << argv[i] << " is missing an argument\n";
              exit = true;
              return false;
            }
            ++i;

            if(j->type == option::String) {
              *j->ref.str = argv[i];
            }
            else if(j->type == option::Int) {
              char *endptr;
              *j->ref.i = strtol(argv[i], &endptr, 10);
              if(*endptr != '\0') {
                cerr << "option " << argv[i] << " expects a number as argument but got '" << argv[i] << "'\n";
                exit = true;
                return false;
              }
            }
            break;
          }
        }
      }
      else if(unnamed_args.size() > current_unnamed) {
        *unnamed_args[current_unnamed].str = argv[i];
        ++current_unnamed;
      }
      else {
        help(argv[0]);
      }
    }
    return not exit;
  }

  bool should_exit() const { // returns true if an error occurred and the program should exit
    return exit;
  }
private:
  mutable bool exit;

  void help(char const *progname) const {
    cerr << "usage: " << progname;
    for(std::vector<option>::const_iterator i = options.begin(); i != options.end(); ++i) {
      cerr << " --" << i->name;
    }
    if(handle_help) {
      cerr << " --help";
    }
    for(std::vector<unnamed>::const_iterator i = unnamed_args.begin(); i != unnamed_args.end(); ++i) {
      cerr << " <" << i->name << '>';
    }
    cerr << "\n\n";
    for(std::vector<option>::const_iterator i = options.begin(); i != options.end(); ++i) {
      cerr << "\t--" << i->name;
      if(i->type != option::Bool) {
        cerr << " <arg>";
      }
      if(i->short_name != '\0') {
        cerr << " (or -" << i->short_name << ')';
      }
      cerr << "\t... " << i->description << '\n';
    }
    if(handle_help) {
      cerr << "\t--help (or -h)\t... show help information\n";
    }
    for(std::vector<unnamed>::const_iterator i = unnamed_args.begin(); i != unnamed_args.end(); ++i) {
      cerr << "\t<" << i->name << ">\t... " << i->description << '\n';
    }
    exit = true;
  }

  struct option {
    enum arg_type { Bool, String, Int } type;
    union {
      bool *flag;
      std::string *str;
      int *i;
    } ref;
    char const *name;
    char const *description;
    char short_name;

    template<typename T>
    option(char const *name, arg_type type, T &r, char const *description, char short_name)
      : type(type), name(name), description(description), short_name(short_name)
    {
      switch(type) {
      case Bool:
        ref.flag = reinterpret_cast<bool*>(&r);
        break;
      case String:
        ref.str = reinterpret_cast<std::string*>(&r);
        break;
      case Int:
        ref.i = reinterpret_cast<int*>(&r);
        break;
      }
    }
  };
  std::vector<option> options;

  struct unnamed {
    std::string *str;
    char const *name;
    char const *description;
    unnamed(std::string &s, char const *name, char const *description)
      : str(&s), name(name), description(description)
    { }
  };
  std::vector<unnamed> unnamed_args;
  bool handle_help;
};

int main(int argc, char **argv) {
  bool dump_images = false;
  bool verb = false;
  std::string ifo_file;
  std::string subname;

  {
    cmd_options opts;
    opts.
      add_option("dump_images", dump_images, "dump subtitles as image files (<subname>-<number>.pgm).").
      add_option("verbose", verb, "extra verbosity").
      add_option("ifo", ifo_file, "name of the ifo file. default: tries to open <subname>.ifo. ifo file is optional!").
      add_unnamed(subname, "subname", "name of the subtitle files WITHOUT .idx/.sub ending! (REQUIRED)");
    if(not opts.parse_cmd(argc, argv) or subname.empty()) {
      return 1;
    }
  }

  // Init the mplayer part
  verbose = verb; // mplayer verbose level
  mp_msg_init();

  // Init Tesseract
  TessBaseAPI::SimpleInit("/usr/share/tesseract-ocr/tessdata", "eng", false); // TODO params

  // Open the sub/idx subtitles
  spu_t spu;
  vob_t vob = vobsub_open(subname.c_str(), ifo_file.empty() ? 0x0 : ifo_file.c_str(), 1, &spu);
  if(not vob) {
    cerr << "Couldn't open VobSub files '" << subname << ".idx/.sub'\n";
    return 1;
  }

  // Open srt output file
  string const srt_filename = subname + ".srt";
  FILE *srtout = fopen(srt_filename.c_str(), "w");
  if(not srtout) {
    perror("could not open .srt file");
  }

  // Read subtitles and convert
  void *packet;
  int timestamp; // pts100
  int len;
  unsigned sub_counter = 1;
  while( (len = vobsub_get_next_packet(vob, &packet, &timestamp)) > 0) {
    if(timestamp >= 0) {
      spudec_assemble(spu, reinterpret_cast<unsigned char*>(packet), len, timestamp);
      spudec_heartbeat(spu, timestamp);
      unsigned char const *image;
      size_t image_size;
      unsigned width, height, stride, start_pts, end_pts;
      spudec_get_data(spu, &image, &image_size, &width, &height, &stride, &start_pts, &end_pts);
      if(verbose > 0 and static_cast<unsigned>(timestamp) != start_pts) {
        cerr << sub_counter << ": time stamp from .idx (" << timestamp << ") doesn't match time stamp from .sub ("
             << start_pts << ")\n";
      }

      if(dump_images) {
        dump_pgm(subname, sub_counter, width, height, image, image_size);
      }

      char *text = TessBaseAPI::TesseractRect(image, 1, stride, 0, 0, width, height);
      if(not text) {
        cerr << "OCR failed\n";
        continue;
      }
      cout << "Text: " << text << endl; // DEBUG
      fprintf(srtout, "%u\n%s --> %s\n%s\n\n", sub_counter, pts2srt(timestamp).c_str(), pts2srt(end_pts).c_str(), text);
      delete[]text;
      ++sub_counter;
    }
  }

  TessBaseAPI::End();
  fclose(srtout);
  cout << "Wrote Subtitles to '" << srt_filename << "'\n";
  vobsub_close(vob);
  spudec_free(spu);
}
