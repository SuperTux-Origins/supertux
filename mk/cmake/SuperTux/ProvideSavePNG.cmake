# SDL_SavePNG needs libpng. On Emscripten use the official ports (-sUSE_LIBPNG /
# -sUSE_ZLIB) instead of system find_package (no host zlib/png under emcmake).

if(EMSCRIPTEN)
  message(STATUS "Emscripten: LibSavePNG via -sUSE_LIBPNG=1 -sUSE_ZLIB=1 ports")
  file(GLOB SAVEPNG_SOURCES_CXX RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    external/SDL_SavePNG/savepng.c)
  add_library(LibSavePNG STATIC ${SAVEPNG_SOURCES_CXX})
  target_include_directories(LibSavePNG SYSTEM PUBLIC external/SDL_SavePNG)
  # Ports inject headers on the em++ include path; no separate PNG::PNG target.
  target_link_libraries(LibSavePNG PUBLIC LibSDL2)
  # Ensure link line pulls the ports (also added to global USE_FLAGS).
  target_link_options(LibSavePNG INTERFACE "SHELL:-sUSE_LIBPNG=1" "SHELL:-sUSE_ZLIB=1")
else()
  find_package(PNG REQUIRED)
  find_package(ZLIB REQUIRED)

  file(GLOB SAVEPNG_SOURCES_CXX RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    external/SDL_SavePNG/savepng.c)
  add_library(LibSavePNG STATIC ${SAVEPNG_SOURCES_CXX})
  target_include_directories(LibSavePNG SYSTEM PUBLIC
    ${PNG_INCLUDE_DIRS}
    external/SDL_SavePNG)
  target_link_libraries(LibSavePNG PUBLIC LibSDL2 ${PNG_LIBRARIES} ZLIB::ZLIB)
endif()

# EOF #
