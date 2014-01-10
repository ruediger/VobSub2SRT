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
using namespace std;

#include "langcodes.h++"
#include "cmd_options.h++"

typedef void* vob_t;
typedef void* spu_t;

// helper struct for caching and fixing end_pts in some cases
typedef struct {
  unsigned start_pts, end_pts;
  char *text;
} sub_text_t;

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

  enum { length = sizeof("HH:MM:SS,MSS") };
  char buf[length];
  snprintf(buf, length, "%02d:%02d:%02d,%03d", h, m, s, ms);
  return std::string(buf);
}

/// Dumps the image data to <subtitlename>-<subtitleid>.pgm in Netbpm PGM format
void dump_pgm(std::string const &filename, unsigned counter, unsigned width, unsigned height,
              unsigned stride, unsigned char const *image, size_t image_size) {

  char buf[500];
  snprintf(buf, sizeof(buf), "%s-%03u.pgm", filename.c_str(), counter);
  FILE *pgm = fopen(buf, "wb");
  if(pgm) {
    fprintf(pgm, "P5\n%u %u %u\n", width, height, 255u);
    for(unsigned i = 0; i < image_size; i += stride) {
      fwrite(image + i, 1, width, pgm);
    }
    fclose(pgm);
  }
}

#ifdef CONFIG_TESSERACT_NAMESPACE
using namespace tesseract;
#endif

#ifndef TESSERACT_DATA_PATH
#define TESSERACT_DATA_PATH "/usr/share/tesseract-ocr/tessdata" // TODO check this in cmake
#endif

int main(int argc, char **argv) {
  bool dump_images = false;
  bool verb = false;
  bool list_languages = false;
  std::string ifo_file;
  std::string subname;
  std::string lang;
  std::string tess_lang_user;
  std::string blacklist;
  std::string tesseract_data_path = TESSERACT_DATA_PATH;
  int index = -1;

  {
    cmd_options opts;
    opts.
      add_option("dump-images", dump_images, "dump subtitles as image files (<subname>-<number>.pgm).").
      add_option("verbose", verb, "extra verbosity").
      add_option("ifo", ifo_file, "name of the ifo file. default: tries to open <subname>.ifo. ifo file is optional!").
      add_option("lang", lang, "language to select", 'l').
      add_option("langlist", list_languages, "list languages and exit").
      add_option("index", index, "subtitle index", 'i').
      add_option("tesseract-lang", tess_lang_user, "set tesseract language (Default: auto detect)").
      add_option("tesseract-data", tesseract_data_path, "path to tesseract data (Default: " TESSERACT_DATA_PATH ")").
      add_option("blacklist", blacklist, "Character blacklist to improve the OCR (e.g. \"|\\/`_~<>\")").
      add_unnamed(subname, "subname", "name of the subtitle files WITHOUT .idx/.sub ending! (REQUIRED)");
    if(not opts.parse_cmd(argc, argv) or subname.empty()) {
      return 1;
    }
  }

  // Init the mplayer part
  verbose = verb; // mplayer verbose level
  mp_msg_init();

  // Open the sub/idx subtitles
  spu_t spu;
  vob_t vob = vobsub_open(subname.c_str(), ifo_file.empty() ? 0x0 : ifo_file.c_str(), 1, &spu);
  if(not vob or vobsub_get_indexes_count(vob) == 0) {
    cerr << "Couldn't open VobSub files '" << subname << ".idx/.sub'\n";
    return 1;
  }

  // list languages and exit
  if(list_languages) {
    cout << "Languages:\n";
    for(size_t i = 0; i < vobsub_get_indexes_count(vob); ++i) {
      char const *const id = vobsub_get_id(vob, i);
      cout << i << ": " << (id ? id : "(no id)") << '\n';
    }
    return 0;
  }

  // Handle stream Ids and language

  if(not lang.empty() and index >= 0) {
    cerr << "Setting both lang and index not supported.\n";
    return 1;
  }

  // default english
  char const *tess_lang = tess_lang_user.empty() ? "eng" : tess_lang_user.c_str();
  if(not lang.empty()) {
    if(vobsub_set_from_lang(vob, lang.c_str()) < 0) {
      cerr << "No matching language for '" << lang << "' found! (Trying to use default)\n";
    }
    else if(tess_lang_user.empty()) {
      // convert two letter lang code into three letter lang code (required by tesseract)
      char const *const lang3 = iso639_1_to_639_3(lang.c_str());
      if(lang3) {
        tess_lang = lang3;
      }
    }
  }
  else {
    if(index >= 0) {
      if(static_cast<unsigned>(index) >= vobsub_get_indexes_count(vob)) {
        cerr << "Index argument out of range: " << index << " ("
             << vobsub_get_indexes_count(vob) << ")\n";
        return 1;
      }
      vobsub_id = index;
    }

    if(vobsub_id >= 0) { // try to set correct tesseract lang for default stream
      char const *const lang1 = vobsub_get_id(vob, vobsub_id);
      if(lang1 and tess_lang_user.empty()) {
        char const *const lang3 = iso639_1_to_639_3(lang1);
        if(lang3) {
          tess_lang = lang3;
        }
      }
    }
  }

  // Init Tesseract
#ifdef CONFIG_TESSERACT_NAMESPACE
  TessBaseAPI tess_base_api;
  if(tess_base_api.Init(tesseract_data_path.c_str(), tess_lang) == -1) {
    cerr << "Failed to initialize tesseract (OCR).\n";
    return 1;
  }
  if(not blacklist.empty()) {
    tess_base_api.SetVariable("tessedit_char_blacklist", blacklist.c_str());
  }
#else
  TessBaseAPI::SimpleInit(tesseract_data_path.c_str(), tess_lang, false); // TODO params
  if(not blacklist.empty()) {
    TessBaseAPI::SetVariable("tessedit_char_blacklist", blacklist.c_str());
  }
#endif

  // Open srt output file
  string const srt_filename = subname + ".srt";
  FILE *srtout = fopen(srt_filename.c_str(), "w");
  if(not srtout) {
    perror("could not open .srt file");
    return 1;
  }

  // Read subtitles and convert
  void *packet;
  int timestamp; // pts100
  int len;
  unsigned last_start_pts = 0;
  unsigned sub_counter = 1;
  sub_text_t sub_text;
  std::vector<sub_text_t> conv_subs;
  while( (len = vobsub_get_next_packet(vob, &packet, &timestamp)) > 0) {
    if(timestamp >= 0) {
      spudec_assemble(spu, reinterpret_cast<unsigned char*>(packet), len, timestamp);
      spudec_heartbeat(spu, timestamp);
      unsigned char const *image;
      size_t image_size;
      unsigned width, height, stride, start_pts, end_pts;
      spudec_get_data(spu, &image, &image_size, &width, &height, &stride, &start_pts, &end_pts);

      // skip this packet if it is another packet of a subtitle that
      // was decoded from multiple mpeg packets.
      if (start_pts == last_start_pts) {
        continue;
      }
      last_start_pts = start_pts;

      if(verbose > 0 and static_cast<unsigned>(timestamp) != start_pts) {
        cerr << sub_counter << ": time stamp from .idx (" << timestamp << ") doesn't match time stamp from .sub ("
             << start_pts << ")\n";
      }

      if(dump_images) {
        dump_pgm(subname, sub_counter, width, height, stride, image, image_size);
      }

#ifdef CONFIG_TESSERACT_NAMESPACE
      char *text = tess_base_api.TesseractRect(image, 1, stride, 0, 0, width, height);
#else
      char *text = TessBaseAPI::TesseractRect(image, 1, stride, 0, 0, width, height);
#endif
      if(not text) {
        cerr << "ERROR: OCR failed for " << sub_counter << '\n';
        continue;
      }
      if(verb) {
        cout << sub_counter << " Text: " << text << endl;
      }
      sub_text.start_pts = start_pts;
      sub_text.end_pts = end_pts;
      sub_text.text = text;
      conv_subs.push_back(sub_text);
      ++sub_counter;
    }
  }

  // write the file, fixing end_pts when needed
  for (unsigned i = 0; i < conv_subs.size(); i++)
  {
      if(conv_subs[i].end_pts == UINT_MAX && i+1 < conv_subs.size())
        conv_subs[i].end_pts = conv_subs[i+1].start_pts;

      fprintf(srtout, "%u\n%s --> %s\n%s\n\n", i, pts2srt(conv_subs[i].start_pts).c_str(), pts2srt(conv_subs[i].end_pts).c_str(), conv_subs[i].text);
      delete[]conv_subs[i].text;
  }

#ifdef CONFIG_TESSERACT_NAMESPACE
  tess_base_api.End();
#else
  TessBaseAPI::End();
#endif
  fclose(srtout);
  cout << "Wrote Subtitles to '" << srt_filename << "'\n";
  vobsub_close(vob);
  spudec_free(spu);
}
