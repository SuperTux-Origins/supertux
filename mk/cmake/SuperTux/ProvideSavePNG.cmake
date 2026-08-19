# SDL_SavePNG needs libpng (+ zlib).

if(EMSCRIPTEN)
  message(STATUS "Emscripten: LibSavePNG via -sUSE_LIBPNG=1 -sUSE_ZLIB=1 ports")
  file(GLOB SAVEPNG_SOURCES_CXX RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    external/SDL_SavePNG/savepng.c)
  add_library(LibSavePNG STATIC ${SAVEPNG_SOURCES_CXX})
  target_include_directories(LibSavePNG SYSTEM PUBLIC external/SDL_SavePNG)
  target_link_libraries(LibSavePNG PUBLIC LibSDL2)
  target_link_options(LibSavePNG INTERFACE "SHELL:-sUSE_LIBPNG=1" "SHELL:-sUSE_ZLIB=1")
  return()
endif()

find_package(PNG QUIET)
find_package(ZLIB QUIET)

if(NOT PNG_FOUND)
  find_path(PNG_PNG_INCLUDE_DIR png.h
    PATHS
      ${CMAKE_SYSROOT}/usr/include
      ${CMAKE_SYSROOT}/usr/include/libpng16
      ${CMAKE_SYSROOT}/usr/include/libpng12
    NO_DEFAULT_PATH)
  find_library(PNG_LIBRARY
    NAMES png png16 png12
    PATHS
      ${CMAKE_SYSROOT}/usr/lib
      ${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu
      ${CMAKE_SYSROOT}/lib
      ${CMAKE_SYSROOT}/lib/aarch64-linux-gnu
    NO_DEFAULT_PATH)
  if(PNG_PNG_INCLUDE_DIR AND PNG_LIBRARY)
    set(PNG_FOUND TRUE)
    set(PNG_INCLUDE_DIRS "${PNG_PNG_INCLUDE_DIR}")
    set(PNG_LIBRARIES "${PNG_LIBRARY}")
    message(STATUS "Found PNG (sysroot): ${PNG_LIBRARY}")
  endif()
endif()

if(NOT ZLIB_FOUND)
  find_path(ZLIB_INCLUDE_DIR zlib.h
    PATHS ${CMAKE_SYSROOT}/usr/include NO_DEFAULT_PATH)
  find_library(ZLIB_LIBRARY
    NAMES z
    PATHS
      ${CMAKE_SYSROOT}/usr/lib
      ${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu
    NO_DEFAULT_PATH)
  if(ZLIB_INCLUDE_DIR AND ZLIB_LIBRARY)
    set(ZLIB_FOUND TRUE)
    if(NOT TARGET ZLIB::ZLIB)
      add_library(ZLIB::ZLIB UNKNOWN IMPORTED)
      set_target_properties(ZLIB::ZLIB PROPERTIES
        IMPORTED_LOCATION "${ZLIB_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${ZLIB_INCLUDE_DIR}")
    endif()
    message(STATUS "Found ZLIB (sysroot): ${ZLIB_LIBRARY}")
  endif()
endif()

if(NOT PNG_FOUND AND SUPERTUX_R36S)
  # Screenshots disabled until libpng is in the ArkOS sysroot.
  message(WARNING "R36S: libpng missing — LibSavePNG is a no-op stub (screenshots disabled)")
  file(WRITE "${CMAKE_BINARY_DIR}/savepng_stub.c"
"#include <SDL.h>
#include \"savepng.h\"
int SDL_SavePNG(SDL_Surface *surface, const char *file) { (void)surface; (void)file; return -1; }
int SDL_SavePNG_RW(SDL_Surface *surface, SDL_RWops *dst, int freedst) {
  (void)surface; (void)dst; if (freedst) SDL_RWclose(dst); return -1;
}
")
  add_library(LibSavePNG STATIC "${CMAKE_BINARY_DIR}/savepng_stub.c")
  target_include_directories(LibSavePNG SYSTEM PUBLIC external/SDL_SavePNG)
  target_link_libraries(LibSavePNG PUBLIC LibSDL2)
  return()
endif()

if(NOT PNG_FOUND)
  message(FATAL_ERROR
    "Could NOT find PNG (libpng).\n"
    "  For R36S: install libpng-dev into the ArkOS sysroot.")
endif()

file(GLOB SAVEPNG_SOURCES_CXX RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  external/SDL_SavePNG/savepng.c)
add_library(LibSavePNG STATIC ${SAVEPNG_SOURCES_CXX})
target_include_directories(LibSavePNG SYSTEM PUBLIC
  ${PNG_INCLUDE_DIRS}
  external/SDL_SavePNG)
if(TARGET ZLIB::ZLIB)
  target_link_libraries(LibSavePNG PUBLIC LibSDL2 ${PNG_LIBRARIES} ZLIB::ZLIB)
else()
  target_link_libraries(LibSavePNG PUBLIC LibSDL2 ${PNG_LIBRARIES})
endif()

# EOF #
