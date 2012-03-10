install(FILES vobsub2srt.1 DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man1)

option(BASH_COMPLETION_PATH "Install the bash completion script to the given path. Should be set to the value of $BASH_COMPLETION_DIR if available.")
if(BASH_COMPLETION_PATH)
  message(STATUS "Bash completion path: ${BASH_COMPLETION_PATH}")
  install(FILES completion.sh DESTINATION ${BASH_COMPLETION_PATH} RENAME vobsub2srt)
endif()
