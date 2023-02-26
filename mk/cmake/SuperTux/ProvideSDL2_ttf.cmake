find_package(SDL2_ttf QUIET)

if(TARGET SDL2_ttf::SDL2_ttf)
  message(STATUS "Found preinstalled SDL2_ttf")

  add_library(LibSDL2_ttf ALIAS SDL2_ttf::SDL2_ttf)
elseif(TARGET SDL2_ttf)
  message(STATUS "Found preinstalled SDL2_ttf")

  add_library(LibSDL2_ttf ALIAS SDL2_ttf)
else()
  message(FATAL "Could NOT find SDL2_ttf")
endif()

# EOF #
