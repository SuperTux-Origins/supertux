## libcurl is unused by Origins (no networking code). Keep the module so
## CMakeLists include(ProvideCurl) stays valid; never link curl.

message(STATUS "Skipping libcurl (unused by SuperTux Origins)")
set(HAVE_LIBCURL FALSE)
if(NOT TARGET LibCurl)
  add_library(LibCurl INTERFACE IMPORTED)
endif()

# EOF #
