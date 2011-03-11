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
using namespace std;

#include "langcodes.h++"
#include "cmd_options.h++"

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
  snprintf(buf, sizeof(buf), "%s-%03u.pgm", filename.c_str(), counter);
  FILE *pgm = fopen(buf, "wb");
  if(pgm) {
    fprintf(pgm, "P5\n%u %u %u\n", width, height, 255u);
    fwrite(image, 1, image_size, pgm);
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
  std::string ifo_file;
  std::string subname;
  std::string lang;
  std::string tesseract_data_path = TESSERACT_DATA_PATH;

  {
    cmd_options opts;
    opts.
      add_option("dump-images", dump_images, "dump subtitles as image files (<subname>-<number>.pgm).").
      add_option("verbose", verb, "extra verbosity").
      add_option("ifo", ifo_file, "name of the ifo file. default: tries to open <subname>.ifo. ifo file is optional!").
      add_option("lang", lang, "language to select", 'l').
      add_option("tesseract-data", tesseract_data_path, "path to tesseract data (Default: " TESSERACT_DATA_PATH ")").
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
  if(not vob) {
    cerr << "Couldn't open VobSub files '" << subname << ".idx/.sub'\n";
    return 1;
  }

  // Handle stream Ids and language
  if(verbose > 0) { // TODO mplayer prints this too
    // Print languages/indices
    unsigned const index_count = vobsub_get_indexes_count(vob);
    cerr << "Index Count: " << index_count << endl;
    for(unsigned i = 0; i < index_count; ++i) {
      int id = vobsub_get_id_by_index(vob, i);
      cerr << "Id: " << id << " Lang: " << (id > 0 ? vobsub_get_id(vob, id) : "<no id>") << endl;
    }
  }

  char const *tess_lang = "eng"; // default english
  if(not lang.empty()) {
    if(vobsub_set_from_lang(vob, lang.c_str()) < 0) {
      cerr << "No matching language for '" << lang << "' found! (Trying to use default)\n";
    }
    else {
      // convert two letter lang code into three letter lang code (required by tesseract)
      char const *const lang3 = iso639_1_to_639_3(lang.c_str());
      if(lang3) {
        tess_lang = lang3;
      }
    }
  }
  else if(vobsub_id >= 0) { // try to set correct tesseract lang for default stream
    char const *const lang1 = vobsub_get_id(vob, vobsub_id);
    if(lang1) {
      char const *const lang3 = iso639_1_to_639_3(lang1);
      if(lang3) {
        tess_lang = lang3;
      }
    }
  }

  // Init Tesseract
#ifdef CONFIG_TESSERACT_NAMESPACE
  TessBaseAPI tess_base_api;
  tess_base_api.Init(tesseract_data_path.c_str(), tess_lang);
#else
  TessBaseAPI::SimpleInit(tesseract_data_path.c_str(), tess_lang, false); // TODO params
#endif

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
  unsigned last_start_pts = 0;
  unsigned sub_counter = 1;
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
      if (start_pts == last_start_pts) continue;
      last_start_pts = start_pts;

      if(verbose > 0 and static_cast<unsigned>(timestamp) != start_pts) {
        cerr << sub_counter << ": time stamp from .idx (" << timestamp << ") doesn't match time stamp from .sub ("
             << start_pts << ")\n";
      }

      if(dump_images) {
        dump_pgm(subname, sub_counter, width, height, image, image_size);
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
      fprintf(srtout, "%u\n%s --> %s\n%s\n\n", sub_counter, pts2srt(start_pts).c_str(), pts2srt(end_pts).c_str(), text);
      delete[]text;
      ++sub_counter;
    }
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
