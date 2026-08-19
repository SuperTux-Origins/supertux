# SDL2_ttf — skipped under Emscripten when FreeType is provided via -sUSE_FREETYPE=2
# (see CMakeLists.txt EMSCRIPTEN USE_FLAGS).  Other platforms require system SDL2_ttf.

if(EMSCRIPTEN)
  message(STATUS "Emscripten: SDL2_ttf via FreeType port (-sUSE_FREETYPE); LibSDL2_ttf is INTERFACE")
  add_library(LibSDL2_ttf INTERFACE)
  # Headers still needed for SDL_ttf.h includes in sources — emscripten ports stage them.
  return()
endif()

find_package(SDL2_ttf QUIET)

if(TARGET SDL2_ttf::SDL2_ttf)
  message(STATUS "Found preinstalled SDL2_ttf")
  add_library(LibSDL2_ttf ALIAS SDL2_ttf::SDL2_ttf)
elseif(TARGET SDL2_ttf)
  message(STATUS "Found preinstalled SDL2_ttf")
  add_library(LibSDL2_ttf ALIAS SDL2_ttf)
else()
  message(FATAL_ERROR "Could NOT find SDL2_ttf")
endif()

# EOF #
