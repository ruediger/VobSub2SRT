find_program(GZIP gzip
  HINTS
  /bin
  /usr/bin
  /usr/local/bin)

if(GZIP-NOTFOUND)
  add_custom_target(documentation
    DEPENDS vobsub2srt.1)

  install(FILES vobsub2srt.1 DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man1)
else()
  add_custom_command(
    OUTPUT vobsub2srt.1.gz
    COMMAND ${GZIP} -9 -c vobsub2srt.1 > vobsub2srt.1.gz
    MAIN_DEPENDENCY vobsub2srt.1
    WORKING_DIRECTORY "${vobsub2srt_SOURCE_DIR}/doc")

  add_custom_target(documentation
    DEPENDS vobsub2srt.1.gz)

  install(FILES vobsub2srt.1.gz DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man1)
endif()

option(BASH_COMPLETION_PATH "Install the bash completion script to the given path. Should be set to the value of $BASH_COMPLETION_DIR if available.")
if(BASH_COMPLETION_PATH)
  message(STATUS "Bash completion path: ${BASH_COMPLETION_PATH}")
  install(FILES completion.sh DESTINATION ${BASH_COMPLETION_PATH} RENAME vobsub2srt)
endif()
