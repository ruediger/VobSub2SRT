add_definitions("-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -D_REENTRANT")
add_definitions("-Wundef -Wall -Wno-switch -Wno-parentheses -Wpointer-arith -Wredundant-decls -Wstrict-prototypes -Wmissing-prototypes -Wdisabled-optimization -Wno-pointer-sign -Wdeclaration-after-statement")

set(mplayer_sources
  mp_msg.c
  mp_msg.h
  spudec.c
  spudec.h
  unrar_exec.c
  unrar_exec.h
  vobsub.c
  vobsub.h
  )

add_library(mplayer STATIC ${mplayer_sources})
