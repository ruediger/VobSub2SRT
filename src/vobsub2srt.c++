
// MPlayer stuff
#include "mp_msg.h" // mplayer message framework
#include "vobsub.h"
#include "spudec.h"

#include <iostream>
#include <cstdio>
#include <string>
using namespace std;

typedef void* vob_t;

int main(int argc, char **argv) {
  if(argc < 2 or argc > 3) {
    cerr << "usage: " << argv[0] << " <subname> [<ifo>]\n\n\t<subname> ... without .idx/.sub suffix.\n\t<ifo> ... optional path to ifo file\n";
    return 1;
  }
  verbose = 1; // mplayer verbose level
  mp_msg_init();

  void *spu;
  vob_t vob = vobsub_open(argv[1], argc == 3 ? argv[3] : 0x0, 1, &spu);
  if(not vob) {
    cerr << "Couldn't open VobSub\n";
    return 1;
  }

  string srt_filename = argv[0];
  srt_filename += ".srt";
  FILE *srtout = fopen(srt_filename.c_str(), "w");
  if(not srtout) {
    perror("could not open .srt file");
  }

  void *packet;
  int timestamp; // pts100
  int len;
  unsigned sub_counter = 1;
  while( (len = vobsub_get_next_packet(vob, &packet, &timestamp)) > 0) {
    if(timestamp >= 0) {
      spudec_assemble(spu, reinterpret_cast<unsigned char*>(packet), len, timestamp);
      // spudec_draw ...
      // tesseract
#if 0
      fprintf(srtout, "%u\n%02d:%02d:%02d,%03d --> %02d:%02d:%02d,%03d\n%s\n\n", sub_counter
              //00:02:26,407 --> 00:02:31,356
              );
#endif
      ++sub_counter;
    }
  }

  fclose(srtout);
  vobsub_close(vob);
  spudec_free(spu);
}
