# SDL2_ttf — Emscripten: prefer FREETYPE_* from nix (Windstille-style freetypeWasm);
# else hand-built FreeType from FREETYPE_SOURCE_DIR; else offline stub.
# Desktop/R36S: system package or SDL2_TTF_SOURCE_DIR.

if(EMSCRIPTEN)
  set(_st_have_ttf_src FALSE)
  if(SDL2_TTF_SOURCE_DIR AND EXISTS "${SDL2_TTF_SOURCE_DIR}/SDL_ttf.c")
    set(_st_have_ttf_src TRUE)
  endif()

  # 1) Prebuilt FreeType (nix freetypeWasm): FREETYPE_INCLUDE_DIRS + FREETYPE_LIBRARY
  if(_st_have_ttf_src AND FREETYPE_INCLUDE_DIRS AND FREETYPE_LIBRARY)
    message(STATUS "Emscripten: SDL_ttf + prebuilt FreeType (${FREETYPE_LIBRARY})")
    add_library(LibSDL2_ttf STATIC "${SDL2_TTF_SOURCE_DIR}/SDL_ttf.c")
    target_include_directories(LibSDL2_ttf PUBLIC
      "${SDL2_TTF_SOURCE_DIR}"
      ${FREETYPE_INCLUDE_DIRS})
    target_compile_definitions(LibSDL2_ttf PRIVATE TTF_USE_HARFBUZZ=0)
    target_link_libraries(LibSDL2_ttf PUBLIC ${FREETYPE_LIBRARY})
    if(TARGET LibSDL2)
      target_link_libraries(LibSDL2_ttf PUBLIC LibSDL2)
    endif()
    return()
  endif()

  # 2) Fallback: compile a minimal FreeType set from FREETYPE_SOURCE_DIR
  if(_st_have_ttf_src AND FREETYPE_SOURCE_DIR
      AND EXISTS "${FREETYPE_SOURCE_DIR}/include/ft2build.h")
    message(STATUS "Emscripten: SDL_ttf + in-tree FreeType sources (${FREETYPE_SOURCE_DIR})")
    set(_ft_srcs
      ${FREETYPE_SOURCE_DIR}/src/autofit/autofit.c
      ${FREETYPE_SOURCE_DIR}/src/base/ftbase.c
      ${FREETYPE_SOURCE_DIR}/src/base/ftbbox.c
      ${FREETYPE_SOURCE_DIR}/src/base/ftbitmap.c
      ${FREETYPE_SOURCE_DIR}/src/base/ftdebug.c
      ${FREETYPE_SOURCE_DIR}/src/base/ftglyph.c
      ${FREETYPE_SOURCE_DIR}/src/base/ftinit.c
      ${FREETYPE_SOURCE_DIR}/src/base/ftstroke.c
      ${FREETYPE_SOURCE_DIR}/src/base/ftsystem.c
      ${FREETYPE_SOURCE_DIR}/src/base/ftfstype.c
      ${FREETYPE_SOURCE_DIR}/src/base/ftgasp.c
      ${FREETYPE_SOURCE_DIR}/src/base/ftmm.c
      ${FREETYPE_SOURCE_DIR}/src/base/fttype1.c
      ${FREETYPE_SOURCE_DIR}/src/cff/cff.c
      ${FREETYPE_SOURCE_DIR}/src/pshinter/pshinter.c
      ${FREETYPE_SOURCE_DIR}/src/psnames/psnames.c
      ${FREETYPE_SOURCE_DIR}/src/psaux/psaux.c
      ${FREETYPE_SOURCE_DIR}/src/raster/raster.c
      ${FREETYPE_SOURCE_DIR}/src/sfnt/sfnt.c
      ${FREETYPE_SOURCE_DIR}/src/smooth/smooth.c
      ${FREETYPE_SOURCE_DIR}/src/truetype/truetype.c
    )
    add_library(supertux_freetype_wasm STATIC ${_ft_srcs})
    target_include_directories(supertux_freetype_wasm PUBLIC
      "${FREETYPE_SOURCE_DIR}/include")
    target_compile_definitions(supertux_freetype_wasm PRIVATE
      FT2_BUILD_LIBRARY DARWIN_NO_CARBON)
    target_include_directories(supertux_freetype_wasm PRIVATE
      "${FREETYPE_SOURCE_DIR}/src")
    add_library(LibSDL2_ttf STATIC "${SDL2_TTF_SOURCE_DIR}/SDL_ttf.c")
    target_include_directories(LibSDL2_ttf PUBLIC
      "${SDL2_TTF_SOURCE_DIR}"
      "${FREETYPE_SOURCE_DIR}/include")
    target_compile_definitions(LibSDL2_ttf PRIVATE TTF_USE_HARFBUZZ=0)
    target_link_libraries(LibSDL2_ttf PUBLIC supertux_freetype_wasm)
    if(TARGET LibSDL2)
      target_link_libraries(LibSDL2_ttf PUBLIC LibSDL2)
    endif()
    return()
  endif()

  message(STATUS "Emscripten: offline SDL_ttf stub (need SDL2_TTF_SOURCE_DIR + FreeType)")
  # Publish as SDL_ttf.h so #include <SDL_ttf.h> works without shadowing real SDL_ttf.
  configure_file(
    "${CMAKE_SOURCE_DIR}/mk/emscripten/SDL_ttf_stub.h"
    "${CMAKE_BINARY_DIR}/supertux_sdl_ttf_stub/SDL_ttf.h"
    COPYONLY)
  add_library(LibSDL2_ttf STATIC "${CMAKE_SOURCE_DIR}/mk/emscripten/sdl_ttf_stub.c")
  target_include_directories(LibSDL2_ttf PUBLIC
    "${CMAKE_BINARY_DIR}/supertux_sdl_ttf_stub"
    "${CMAKE_SOURCE_DIR}/mk/emscripten")
  if(TARGET LibSDL2)
    target_link_libraries(LibSDL2_ttf PUBLIC LibSDL2)
  endif()
  return()
endif()

find_package(SDL2_ttf QUIET)

if(TARGET SDL2_ttf::SDL2_ttf)
  message(STATUS "Found preinstalled SDL2_ttf (config)")
  add_library(LibSDL2_ttf ALIAS SDL2_ttf::SDL2_ttf)
  return()
elseif(TARGET SDL2_ttf)
  message(STATUS "Found preinstalled SDL2_ttf")
  add_library(LibSDL2_ttf ALIAS SDL2_ttf)
  return()
endif()

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_SDL2_TTF QUIET SDL2_ttf)
endif()

find_path(SDL2_TTF_INCLUDE_DIR SDL_ttf.h
  HINTS ${PC_SDL2_TTF_INCLUDE_DIRS}
  PATH_SUFFIXES SDL2
  PATHS
    ${CMAKE_SYSROOT}/usr/include
    ${CMAKE_SYSROOT}/usr/include/SDL2
    ${CMAKE_SYSROOT}/usr/local/include
    ${CMAKE_SYSROOT}/usr/local/include/SDL2
)

find_library(SDL2_TTF_LIBRARY
  NAMES SDL2_ttf
  HINTS ${PC_SDL2_TTF_LIBRARY_DIRS}
  PATHS
    ${CMAKE_SYSROOT}/usr/lib
    ${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu
    ${CMAKE_SYSROOT}/lib
    ${CMAKE_SYSROOT}/lib/aarch64-linux-gnu
)

if(SDL2_TTF_INCLUDE_DIR AND SDL2_TTF_LIBRARY)
  message(STATUS "Found SDL2_ttf: ${SDL2_TTF_LIBRARY}")
  add_library(LibSDL2_ttf UNKNOWN IMPORTED)
  set_target_properties(LibSDL2_ttf PROPERTIES
    IMPORTED_LOCATION "${SDL2_TTF_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${SDL2_TTF_INCLUDE_DIR}")
  return()
endif()

if(SDL2_TTF_SOURCE_DIR AND EXISTS "${SDL2_TTF_SOURCE_DIR}/SDL_ttf.c")
  message(STATUS "Building SDL2_ttf from SDL2_TTF_SOURCE_DIR=${SDL2_TTF_SOURCE_DIR}")
  add_library(LibSDL2_ttf STATIC "${SDL2_TTF_SOURCE_DIR}/SDL_ttf.c")
  target_include_directories(LibSDL2_ttf PUBLIC "${SDL2_TTF_SOURCE_DIR}")
  find_package(Freetype QUIET)
  if(TARGET Freetype::Freetype)
    target_link_libraries(LibSDL2_ttf PUBLIC Freetype::Freetype)
  elseif(FREETYPE_INCLUDE_DIRS AND FREETYPE_LIBRARIES)
    target_include_directories(LibSDL2_ttf PUBLIC ${FREETYPE_INCLUDE_DIRS})
    target_link_libraries(LibSDL2_ttf PUBLIC ${FREETYPE_LIBRARIES})
  else()
    message(WARNING "SDL2_ttf from source needs FreeType")
  endif()
  if(TARGET LibSDL2)
    target_link_libraries(LibSDL2_ttf PUBLIC LibSDL2)
  endif()
  return()
endif()

message(FATAL_ERROR "SDL2_ttf not found (install libsdl2-ttf-dev or set SDL2_TTF_SOURCE_DIR)")
