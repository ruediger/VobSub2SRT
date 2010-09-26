# Find libavutil (part of ffmpeg)

include(FindPkgConfig)
pkg_check_modules(Libavutil REQUIRED libavutil)
