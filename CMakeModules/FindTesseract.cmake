# Tesseract OCR

if(Tesseract_INCLUDE_DIR AND Tesseract_LIBRARIES)
  set(Tesseract_FIND_QUIETLY TRUE)
endif()

find_path(Tesseract_INCLUDE_DIR tesseract/baseapi.h
  HINTS
  /usr
  /usr/local)

find_library(Tesseract_LIBRARIES NAMES tesseract_full
  HINTS
  /usr
  /usr/local)

find_library(Tiff_LIBRARY NAMES tiff
  HINTS
  /usr
  /usr/local)

set(Tesseract_LIBRARIES ${Tesseract_LIBRARIES} ${Tiff_LIBRARY})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Tesseract DEFAULT_MSG Tesseract_LIBRARIES Tesseract_INCLUDE_DIR)
mark_as_advanced(Tesseract_INCLUDE_DIR Tesseract_LIBRARIES)
