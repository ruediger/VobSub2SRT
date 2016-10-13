AM_CPPFLAGS = -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -D_REENTRANT
AM_CFLAGS = -std=gnu99 -Wundef -Wall -Wno-switch -Wno-parentheses -Wpointer-arith -Wredundant-decls -Wstrict-prototypes -Wmissing-prototypes -Wdisabled-optimization -Wno-pointer-sign -Wdeclaration-after-statement
AM_CXXFLAGS = -ansi -pedantic -Wall -Wextra -Wno-long-long

bin_PROGRAMS = vobsub2srt
vobsub2srt_SOURCES = src/vobsub2srt.c++ \
  src/langcodes.h++ \
  src/langcodes.c++ \
  src/cmd_options.h++ \
  src/cmd_options.c++ \
  mplayer/mp_msg.c \
  mplayer/mp_msg.h \
  mplayer/spudec.c \
  mplayer/spudec.h \
  mplayer/unrar_exec.c \
  mplayer/unrar_exec.h \
  mplayer/vobsub.c \
  mplayer/vobsub.h
vobsub2srt_CPPFLAGS = $(Tesseract_CFLAGS)
vobsub2srt_LDFLAGS = $(Tesseract_LIBS)

man1_MANS = doc/vobsub2srt.1

bashcompdir = @bashcompdir@
if ENABLE_BASH_COMPLETION
dist_bashcomp_DATA = doc/completion.sh
endif

