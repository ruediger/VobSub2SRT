/*
 *  VobSub2SRT is a simple command line program to convert .idx/.sub subtitles
 *  into .srt text subtitles by using OCR (tesseract). See README.
 *
 *  Copyright (C) 2010-2016 Rüdiger Sonderfeld <ruediger@c-plusplus.de>
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

#include <unistd.h>
#include <iostream>
#include <string>
#include <cstdio>
#include <vector>
#include <atomic>
#include <algorithm>
#include <thread>
#include <mutex>
using namespace std;

#include "langcodes.h++"
#include "cmd_options.h++"

typedef void* vob_t;
typedef void* spu_t;

// helper struct for caching and fixing end_pts in some cases
struct sub_text_t {
  sub_text_t(unsigned counter, unsigned start_pts, unsigned end_pts, char const *text)
    : counter(counter), start_pts(start_pts), end_pts(end_pts), text(text)
  { }
  unsigned counter, start_pts, end_pts;
  char const *text;
};

/** Converts time stamp in pts format to a string containing the time stamp for the srt format
 *
 * pts (presentation time stamp) is given with a 90kHz resolution (1/90 ms).
 * srt expects a time stamp as  HH:MM:SS:MSS.
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
  snprintf(buf, sizeof(buf), "%s-%04u.pgm", filename.c_str(), counter);
  FILE *pgm = fopen(buf, "wb");
  if(pgm) {
    fprintf(pgm, "P5\n%u %u %u\n", width, height, 255u);
    for(unsigned i = 0; i < image_size; i += stride) {
      fwrite(image + i, 1, width, pgm);
    }
    fclose(pgm);
  }
}

using namespace tesseract;

#define TESSERACT_DEFAULT_PATH "<builtin default>"
#ifndef TESSERACT_DATA_PATH
#define TESSERACT_DATA_PATH TESSERACT_DEFAULT_PATH
#endif

TessBaseAPI* init_tesseract(std::string tesseract_data_path, char const *tess_lang, std::string blacklist) {
  char const *tess_path = NULL;
  if (tesseract_data_path != TESSERACT_DEFAULT_PATH)
    tess_path = tesseract_data_path.c_str();

  TessBaseAPI *tess_base_api = new TessBaseAPI();
  if(tess_base_api->Init(tess_path, tess_lang) == -1) {
    delete tess_base_api;
    cerr << "Failed to initialize tesseract (OCR).\n";
    return NULL;
  }
  if(not blacklist.empty()) {
    tess_base_api->SetVariable("tessedit_char_blacklist", blacklist.c_str());
  }
  return tess_base_api;
}

void do_ocr(TessBaseAPI *tess_base_api, atomic<bool> *done, vector<sub_text_t> *conv_subs, mutex *mut,
            unsigned counter, unsigned width, unsigned height, unsigned stride,
            unsigned char *image_cpy, unsigned start_pts, unsigned end_pts, bool verb) {

  char *text = tess_base_api->TesseractRect(image_cpy, 1, stride, 0, 0, width, height);
  free(image_cpy);

  if(not text) {
    cerr << "ERROR: OCR failed for " << counter << '\n';
    char const errormsg[] = "VobSub2SRT ERROR: OCR failure!";
    // using raw memory is evil but that's the way Tesseract works
    // If we switch to C++11 we can use unique_ptr.
    text = new char[sizeof(errormsg)];
    memcpy(text, errormsg, sizeof(errormsg));
  }
  else {
    size_t size = strlen(text);
    while (size > 0 and isspace(text[--size])) {
      text[size] = '\0';
    }
  }
  if(verb) {
    cout << counter << " Text: " << text << endl;
  }
  mut->lock();
  conv_subs->push_back(sub_text_t(counter, start_pts, end_pts, text));
  mut->unlock();
  done->store(true);
}

struct ocr_thread_t {
  ocr_thread_t(TessBaseAPI *tess_base_api)
    : tess_base_api(tess_base_api)
  { }
  thread *t = NULL;
  atomic<bool> done{false};
  TessBaseAPI *tess_base_api = NULL;
};

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
  int y_threshold = 0;
  int min_width = 9;
  int min_height = 1;
  int max_threads = 0;

  {
    /************************************************************************************
     * Any option added here should be added to doc/vobsub2srt.1 and doc/completion.sh! *
     ************************************************************************************/
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
      add_option("y-threshold", y_threshold, "Y (luminance) threshold below which colors treated as black (Default: 0)").
      add_option("min-width", min_width, "Minimum width in pixels to consider a subpicture for OCR (Default: 9)").
      add_option("min-height", min_height, "Minimum height in pixels to consider a subpicture for OCR (Default: 1)").
      add_option("max-threads", max_threads, "Maximum number of threads to use to do the OCR, use 0 to autodetect the number of cores (Default: 0)").
      add_unnamed(subname, "subname", "name of the subtitle files WITHOUT .idx/.sub ending! (REQUIRED)");
    if(not opts.parse_cmd(argc, argv) or subname.empty()) {
      return 1;
    }
  }

  // Init the mplayer part
  verbose = verb; // mplayer verbose level
  mp_msg_init();

  // Set Y threshold from command-line arg only if given
  if (y_threshold) {
    cout << "Using Y palette threshold: " << y_threshold << "\n";
  }

  // Open the sub/idx subtitles
  spu_t spu;
  vob_t vob = vobsub_open(subname.c_str(), ifo_file.empty() ? 0x0 : ifo_file.c_str(), 1, y_threshold, &spu);
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

  // Open srt output file
  string const srt_filename = subname + ".srt";
  FILE *srtout = fopen(srt_filename.c_str(), "w");
  if(not srtout) {
    perror("could not open .srt file");
    return 1;
  }

  if (max_threads <= 0)
    max_threads = thread::hardware_concurrency();

  vector<ocr_thread_t*> threads;

  // Read subtitles and convert
  void *packet;
  int timestamp; // pts100
  int len;
  unsigned last_start_pts = 0;
  unsigned sub_counter = 1;

  vector<sub_text_t> conv_subs;
  conv_subs.reserve(200); // TODO better estimate
  mutex mut;

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

      if(width < (unsigned int)min_width || height < (unsigned int)min_height) {
        cerr << "WARNING: Image too small " << sub_counter << ", size: " << image_size << " bytes, "
             << width << "x" << height << " pixels, expected at least " << min_width << "x" << min_height << "\n";
        continue;
      }

      if(verbose > 0 and static_cast<unsigned>(timestamp) != start_pts) {
        cerr << sub_counter << ": time stamp from .idx (" << timestamp
             << ") doesn't match time stamp from .sub ("
             << start_pts << ")\n";
      }

      if(dump_images) {
        dump_pgm(subname, sub_counter, width, height, stride, image, image_size);
      }

      ocr_thread_t *ocr_thread = NULL;
      if (threads.size() < static_cast<unsigned>(max_threads)) {
        TessBaseAPI *tess_base_api = init_tesseract(tesseract_data_path, tess_lang, blacklist);
        if (tess_base_api == NULL)
          return -1;
        ocr_thread = new ocr_thread_t(tess_base_api);
        threads.push_back(ocr_thread);
      } else if (max_threads == 1) {
        ocr_thread = threads[0];
      } else {
        while (ocr_thread == NULL) {
          for (unsigned i=0; i < threads.size(); i++) {
            if (threads[i]->done) {
              threads[i]->t->join();
              delete threads[i]->t;
              ocr_thread = threads[i];
              break;
            }
          }
          if (ocr_thread == NULL)
            usleep(50);
        }
      }

      unsigned char *image_cpy = (unsigned char *)malloc(image_size);
      memcpy(image_cpy, image, image_size);

      if (max_threads == 1)
        do_ocr(ocr_thread->tess_base_api, &ocr_thread->done, &conv_subs, &mut, sub_counter, width, height, stride, image_cpy, start_pts, end_pts, verb);
      else {
        ocr_thread->done = false;
        ocr_thread->t = new thread(do_ocr, ocr_thread->tess_base_api, &ocr_thread->done, &conv_subs, &mut, sub_counter, width, height, stride, image_cpy, start_pts, end_pts, verb);
      }

      ++sub_counter;
    }
  }

  for(unsigned i = 0; i < threads.size(); ++i) {
    if (threads[i]->t != NULL) {
      threads[i]->t->join();
      delete threads[i]->t;
    }
    threads[i]->tess_base_api->End();
    delete threads[i]->tess_base_api;
    delete threads[i];
  }

  struct {
      bool operator()(sub_text_t a, sub_text_t b) const {
          return a.counter < b.counter;
      }
  } sort_fct;
  sort(conv_subs.begin(), conv_subs.end(), sort_fct);

  // write the file, fixing end_pts when needed
  for(unsigned i = 0; i < conv_subs.size(); ++i) {
    if(conv_subs[i].end_pts == UINT_MAX && i+1 < conv_subs.size())
      conv_subs[i].end_pts = conv_subs[i+1].start_pts;

    fprintf(srtout, "%u\n%s --> %s\n%s\n\n", conv_subs[i].counter, pts2srt(conv_subs[i].start_pts).c_str(),
            pts2srt(conv_subs[i].end_pts).c_str(), conv_subs[i].text);

    delete[]conv_subs[i].text;
    conv_subs[i].text = 0x0;
  }

  fclose(srtout);
  cout << "Wrote Subtitles to '" << srt_filename << "'\n";
  vobsub_close(vob);
  spudec_free(spu);
}
