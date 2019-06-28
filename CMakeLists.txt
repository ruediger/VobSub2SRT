project(vobsub2srt)

cmake_minimum_required(VERSION 2.6.4 FATAL_ERROR)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMakeModules)

if(NOT CMAKE_BUILD_TYPE)
    set(
        CMAKE_BUILD_TYPE
        Debug
        CACHE
        STRING
        "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
endif()

message(STATUS "Source: ${CMAKE_SOURCE_DIR}")
message(STATUS "Binary: ${CMAKE_BINARY_DIR}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

if(${CMAKE_BINARY_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
    message(FATAL_ERROR "In-source builds are not permitted. Make a separate folder for building:\nmkdir build; cd build; cmake ..\nBefore that, remove the files that cmake just created:\nrm -rf CMakeCache.txt CMakeFiles")
endif()

if(BUILD_STATIC)
  message(WARNING "Building a statically linked version of VobSub2SRT is NOT recommended.  You might run into library dependency issues.  Please check the README!")
  set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
endif()

set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

set(INSTALL_EXECUTABLES_PATH ${CMAKE_INSTALL_PREFIX}/bin)
set(INSTALL_LIBRARIES_PATH ${CMAKE_INSTALL_PREFIX}/lib)
set(INSTALL_HEADERS_PATH ${CMAKE_INSTALL_PREFIX}/include)
set(INSTALL_ETC_PATH ${CMAKE_INSTALL_PREFIX}/etc)
set(INSTALL_PC_PATH ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig)
set(INSTALL_DATA_DIR_BASE "${CMAKE_INSTALL_PREFIX}/share" CACHE STRING "Custom data installation directory without suffixes")
set(INSTALL_DOC_DIR_BASE "${INSTALL_DATA_DIR_BASE}/doc" CACHE STRING "Custom doc installation directory without suffixes")
set(INSTALL_DOC_DIR "${INSTALL_DOC_DIR_BASE}/${CMAKE_PROJECT_NAME}" CACHE STRING "Custom doc installation directory")
set(INSTALL_MAN_DIR "${INSTALL_DATA_DIR_BASE}/man/man1" CACHE STRING "Custom manpage installation directory without suffixes")

install(FILES "${CMAKE_SOURCE_DIR}/debian/copyright" DESTINATION ${INSTALL_DOC_DIR})
install(FILES "${CMAKE_SOURCE_DIR}/README.org" DESTINATION ${INSTALL_DOC_DIR} RENAME README)

add_definitions("-DINSTALL_PREFIX=\"${CMAKE_INSTALL_PREFIX}\"")

include(CheckIncludeFile)
include(CheckCCompilerFlag)
include(CheckCSourceCompiles)
include(CheckCSourceRuns)
include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)
include(CheckCXXSourceRuns)

set(CMAKE_C_FLAGS "-std=gnu99")
set(CMAKE_CXX_FLAGS "-ansi -pedantic -Wall -Wextra -Wno-long-long -std=gnu++11")

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -mtune=native -march=native -DNDEBUG -fomit-frame-pointer -ffast-math") # TODO -Ofast GCC 4.6
set(CMAKE_C_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
set(CMAKE_CXX_FLAGS_DEBUG "-g3 -DDEBUG")
set(CMAKE_C_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
# TODO RelWithDebInfo MinSizeRel? maybe move -march=native -ffast-math etc. to a different build type

find_package(Threads)
find_package(Tesseract)

add_subdirectory(mplayer)
add_subdirectory(src)
add_subdirectory(doc)

#### Detect Version
if(NOT VOBSUB2SRT_VERSION)
  if(EXISTS "${vobsub2srt_SOURCE_DIR}/version")
    file(READ "${vobsub2srt_SOURCE_DIR}/version" VOBSUB2SRT_VERSION)
    string(REGEX REPLACE "\n" "" VOBSUB2SRT_VERSION "${VOBSUB2SRT_VERSION}")
  elseif(EXISTS "${vobsub2srt_SOURCE_DIR}/.git")
    if(NOT GIT_FOUND)
      find_package(Git QUIET)
    endif()
    if(GIT_FOUND)
      execute_process(
        COMMAND "${GIT_EXECUTABLE}" describe --tags --dirty --always
        WORKING_DIRECTORY "${vobsub2srt_SOURCE_DIR}"
        OUTPUT_VARIABLE VOBSUB2SRT_VERSION
        RESULT_VARIABLE EXECUTE_GIT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
      string(REGEX REPLACE "^v" "" VOBSUB2SRT_VERSION "${VOBSUB2SRT_VERSION}")
    endif()
  endif()
endif()

if(NOT VOBSUB2SRT_VERSION)
  set(VOBSUB2SRT_VERSION "unknown-dirty")
endif()

message(STATUS "vobsub2srt version: ${VOBSUB2SRT_VERSION}")

#### uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/CMakeModules/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

#### Package Generation
execute_process (
  COMMAND          /usr/bin/dpkg --print-architecture
  OUTPUT_VARIABLE  CPACK_DEBIAN_PACKAGE_ARCHITECTURE
  RESULT_VARIABLE  EXECUTE_RESULT
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET
)
if(EXECUTE_RESULT)
  message(STATUS "dpkg not found: No package generation.")
else()
  message(STATUS "Debian architecture: ${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
  set(CPACK_GENERATOR "DEB")
  set(CPACK_PACKAGE_NAME "${CMAKE_PROJECT_NAME}")
  set(CPACK_PACKAGE_VERSION "${VOBSUB2SRT_VERSION}")
  set(CPACK_PACKAGE_CONTACT "Rüdiger Sonderfeld <ruediger@c-plusplus.de>")
  set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/ruediger/VobSub2SRT")
  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Converts VobSub subtitles (.idx/.srt format) into .srt subtitles.")
  set(CPACK_PACKAGE_DESCRIPTION "VobSub2SRT is a simple command line program to convert .idx / .sub subtitles\ninto .srt text subtitles by using OCR. It is based on code from the MPlayer\nproject - a really great movie player.\nTesseract is used as OCR software.")

  set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/debian/copyright")
  set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.org")
  set(CPACK_STRIP_FILES TRUE)

  set(CPACK_DEBIAN_PACKAGE_SECTION "video")
  set(CPACK_DEBIAN_PACKAGE_DEPENDS libtesseract-dev)
  set(CPACK_DEBIAN_PACKAGE_BUILD_DEPENDS libavutil-dev libtiff5-dev libtesseract-dev tesseract-ocr-eng cmake pkg-config)
#  set(CPACK_DEBIAN_GIT_DCH TRUE)
  set(CPACK_DEBIAN_UPDATE_CHANGELOG TRUE)
  set(PPA_DEBIAN_VERSION "ppa1")

  include(ppa_config.cmake OPTIONAL)

  set(DPUT_HOST "ppa:ruediger-c-plusplus/vobsub2srt")

  include(CPack)

  if(ENABLE_PPA)
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "any") # can be build on any system
    include(UploadPPA)
  endif()
endif()
