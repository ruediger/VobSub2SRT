# Tesseract OCR

if(Tesseract_INCLUDE_DIR AND Tesseract_LIBRARIES)
  set(Tesseract_FIND_QUIETLY TRUE)
endif()

find_path(Tesseract_INCLUDE_DIR tesseract/baseapi.h
  HINTS
  /usr/include
  /usr/local/include)

find_library(Tesseract_LIBRARIES NAMES tesseract_full tesseract_api tesseract
  HINTS
  /usr/lib
  /usr/local/lib)

find_library(Tiff_LIBRARY NAMES tiff
  HINTS
  /usr/lib
  /usr/local/lib)

if(BUILD_STATIC)
# -llept -lgif -lwebp -ltiff -lpng -ljpeg -lz
find_library(Lept_LIBRARY NAMES lept
  HINTS
  /usr/lib
  /usr/local/lib)

find_library(Webp_LIBRARY NAMES webp
  HINTS
  /usr/lib
  /usr/local/lib)

find_package(GIF)
find_package(JPEG)
find_package(PNG)
#find_package(TIFF) TODO replace manual find_library call
find_package(ZLIB)

endif()

if(TESSERACT_DATA_PATH)
  add_definitions(-DTESSERACT_DATA_PATH="${TESSERACT_DATA_PATH}")
endif()

set(CMAKE_REQUIRED_INCLUDES ${Tesseract_INCLUDE_DIR})
check_cxx_source_compiles(
  "#include \"tesseract/baseapi.h\"
   using namespace tesseract;
   int main() {
   }"
  TESSERACT_NAMESPACE)
if(TESSERACT_NAMESPACE)
  add_definitions("-DCONFIG_TESSERACT_NAMESPACE")
else()
  message(WARNING "You are using an old Tesseract version. Support for Tesseract 2 is deprecated and will be removed in the future!")
endif()
list(REMOVE_ITEM CMAKE_REQUIRED_INCLUDES ${Tesseract_INCLUDE_DIR})

if(BUILD_STATIC)
  set(Tesseract_LIBRARIES ${Tesseract_LIBRARIES} ${Lept_LIBRARY} ${PNG_LIBRARY} ${Tiff_LIBRARY} ${Webp_LIBRARY} ${GIF_LIBRARY} ${JPEG_LIBRARY} ${ZLIB_LIBRARY})
else()
  set(Tesseract_LIBRARIES ${Tesseract_LIBRARIES} ${Tiff_LIBRARY})
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Tesseract DEFAULT_MSG Tesseract_LIBRARIES Tesseract_INCLUDE_DIR)
mark_as_advanced(Tesseract_INCLUDE_DIR Tesseract_LIBRARIES)
