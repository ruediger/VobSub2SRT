include_directories(${vobsub2srt_SOURCE_DIR}/mplayer)

link_directories(${Libavutil_LIBRARY_DIRS})
include_directories(${Libavutil_INCLUDE_DIRS})
include_directories(${Tesseract_INCLUDE_DIRS})

set(vobsub2srt_sources
  vobsub2srt.c++
  langcodes.h++
  langcodes.c++
  cmd_options.h++
  cmd_options.c++)

add_executable(vobsub2srt ${vobsub2srt_sources})
if(BUILD_STATIC)
  set_target_properties(vobsub2srt PROPERTIES
                        LINK_SEARCH_END_STATIC ON
                        LINK_FLAGS -static)
endif()
target_link_libraries(vobsub2srt mplayer ${Libavutil_LIBRARIES} ${Tesseract_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS vobsub2srt RUNTIME DESTINATION ${INSTALL_EXECUTABLES_PATH})
