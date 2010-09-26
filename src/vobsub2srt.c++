
// MPlayer stuff
#include "mp_msg.h" // mplayer message framework
#include "vobsub.h"

#include <iostream>
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

  void *data;
  int timestamp;
  while(vobsub_get_next_packet(vob, &data, &timestamp) != -1) {
    // spdec_assemble, spudec_draw ...
    // tesseract
    
  }

  vobsub_close(vob);
}
