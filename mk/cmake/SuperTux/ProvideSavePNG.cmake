# SDL_SavePNG needs libpng. Under EMSCRIPTEN the -sUSE_LIBPNG port would
# download at compile time (fails offline in nix). R36S sysroot often lacks
# libpng. In both cases provide a stub implementing SDL_SavePNG_RW only
# (SDL_SavePNG is a macro in savepng.h).

if(EMSCRIPTEN OR (SUPERTUX_R36S AND NOT DEFINED PNG_FOUND))
  # Try real PNG first on R36S.
endif()

find_package(PNG QUIET)
find_package(ZLIB QUIET)

if(NOT PNG_FOUND AND NOT EMSCRIPTEN)
  find_path(PNG_PNG_INCLUDE_DIR png.h
    PATHS ${CMAKE_SYSROOT}/usr/include ${CMAKE_SYSROOT}/usr/include/libpng16
    NO_DEFAULT_PATH)
  find_library(PNG_LIBRARY NAMES png png16
    PATHS ${CMAKE_SYSROOT}/usr/lib ${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu
    NO_DEFAULT_PATH)
  if(PNG_PNG_INCLUDE_DIR AND PNG_LIBRARY)
    set(PNG_FOUND TRUE)
    set(PNG_INCLUDE_DIRS "${PNG_PNG_INCLUDE_DIR}")
    set(PNG_LIBRARIES "${PNG_LIBRARY}")
    message(STATUS "Found PNG (sysroot): ${PNG_LIBRARY}")
  endif()
endif()

if(NOT ZLIB_FOUND AND NOT EMSCRIPTEN)
  find_path(ZLIB_INCLUDE_DIR zlib.h PATHS ${CMAKE_SYSROOT}/usr/include NO_DEFAULT_PATH)
  find_library(ZLIB_LIBRARY NAMES z
    PATHS ${CMAKE_SYSROOT}/usr/lib ${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu NO_DEFAULT_PATH)
  if(ZLIB_INCLUDE_DIR AND ZLIB_LIBRARY)
    set(ZLIB_FOUND TRUE)
    if(NOT TARGET ZLIB::ZLIB)
      add_library(ZLIB::ZLIB UNKNOWN IMPORTED)
      set_target_properties(ZLIB::ZLIB PROPERTIES
        IMPORTED_LOCATION "${ZLIB_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${ZLIB_INCLUDE_DIR}")
    endif()
  endif()
endif()

if(EMSCRIPTEN OR (NOT PNG_FOUND AND SUPERTUX_R36S))
  message(STATUS "LibSavePNG: stub (EMSCRIPTEN or R36S without libpng) — screenshots disabled")
  file(WRITE "${CMAKE_BINARY_DIR}/savepng_stub.c"
"#include <SDL.h>\n"
"/* savepng.h defines SDL_SavePNG as a macro; only implement SDL_SavePNG_RW. */\n"
"int SDL_SavePNG_RW(SDL_Surface *surface, SDL_RWops *dst, int freedst) {\n"
"  (void)surface;\n"
"  if (dst && freedst) SDL_RWclose(dst);\n"
"  SDL_SetError(\"SDL_SavePNG stub: libpng not linked\");\n"
"  return -1;\n"
"}\n"
"SDL_Surface *SDL_PNGFormatAlpha(SDL_Surface *src) { (void)src; return NULL; }\n"
)
  add_library(LibSavePNG STATIC "${CMAKE_BINARY_DIR}/savepng_stub.c")
  target_include_directories(LibSavePNG SYSTEM PUBLIC external/SDL_SavePNG)
  target_link_libraries(LibSavePNG PUBLIC LibSDL2)
  return()
endif()

if(NOT PNG_FOUND)
  message(FATAL_ERROR "Could NOT find PNG (libpng).")
endif()

file(GLOB SAVEPNG_SOURCES_CXX RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  external/SDL_SavePNG/savepng.c)
add_library(LibSavePNG STATIC ${SAVEPNG_SOURCES_CXX})
target_include_directories(LibSavePNG SYSTEM PUBLIC ${PNG_INCLUDE_DIRS} external/SDL_SavePNG)
if(TARGET ZLIB::ZLIB)
  target_link_libraries(LibSavePNG PUBLIC LibSDL2 ${PNG_LIBRARIES} ZLIB::ZLIB)
else()
  target_link_libraries(LibSavePNG PUBLIC LibSDL2 ${PNG_LIBRARIES})
endif()
