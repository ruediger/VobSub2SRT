include_directories(${vobsub2srt_SOURCE_DIR}/mplayer)

link_directories(${Libavutil_LIBRARY_DIRS})
include_directories(${Libavutil_INCLUDE_DIRS})
include_directories(${Tesseract_INCLUDE_DIR})

add_executable(vobsub2srt vobsub2srt.c++)
target_link_libraries(vobsub2srt mplayer ${Libavutil_LIBRARIES} ${Tesseract_LIBRARIES})

install(TARGETS vobsub2srt RUNTIME DESTINATION ${INSTALL_EXECUTABLES_PATH})
