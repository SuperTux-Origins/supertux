message(STATUS "Checking for SDL2")
if(WIN32)
  if(VCPKG_BUILD)
    find_package(SDL2 CONFIG REQUIRED)
    find_library(SDL2IMAGE_LIBRARIES SDL2_image)
    find_library(SDL2MAIN_LIBRARIES SDL2main)
  else()
    find_path(SDL2_INCLUDE_DIRS NAMES SDL.h PATHS "${DEPENDENCY_FOLDER}/include/SDL2")
    find_path(SDL2IMAGE_INCLUDE_DIRS NAMES SDL_image.h PATHS "${DEPENDENCY_FOLDER}/include/SDL2_image")
    find_library(SDL2_LIBRARIES NAMES SDL2 PATHS "${DEPENDENCY_FOLDER}/lib")
    find_library(SDL2IMAGE_LIBRARIES NAMES SDL2_image PATHS "${DEPENDENCY_FOLDER}/lib")
    find_library(SDL2MAIN_LIBRARIES NAMES SDL2main PATHS "${DEPENDENCY_FOLDER}/lib")
  endif()
else()
  if(NOT EMSCRIPTEN)
    if(VCPKG_BUILD)
      find_package(SDL2 CONFIG REQUIRED)
      find_package(sdl2-image CONFIG REQUIRED)
      find_path(SDL2_INCLUDE_DIRS SDL2/SDL.h)
      find_path(SDL2IMAGE_INCLUDE_DIRS SDL2/SDL_image.h)
      set(SDL2_INCLUDE_DIRS ${SDL2_INCLUDE_DIRS}/SDL2)
      set(SDL2IMAGE_INCLUDE_DIRS ${SDL2IMAGE_INCLUDE_DIRS}/SDL2)
      find_library(SDL2_LIBRARIES SDL2)
      find_library(SDL2IMAGE_LIBRARIES SDL2_image)
    elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
      # 32-bit
      find_library(SDL2_LIBRARIES SDL2)
      find_library(SDL2IMAGE_LIBRARIES SDL2_image)
      find_path(SDL2_INCLUDE_DIRS SDL2/SDL.h)
      find_path(SDL2IMAGE_INCLUDE_DIRS SDL2/SDL_image.h)
      set(SDL2_INCLUDE_DIRS ${SDL2_INCLUDE_DIRS}/SDL2)
      set(SDL2IMAGE_INCLUDE_DIRS ${SDL2IMAGE_INCLUDE_DIRS}/SDL2)
    else()
      # 64-bit
      include(FindPkgConfig)
      if(NOT PKG_CONFIG_FOUND)
        message(WARNING "Trying to find libraries using pkg_search_module, but pkg-config was not found. Install pkg-config to continue.")
      else()
        pkg_search_module(SDL2 REQUIRED sdl2>=2.0.1)
        pkg_search_module(SDL2IMAGE REQUIRED SDL2_image>=2.0.0)
      endif()
    endif()
  endif()
endif()
set(HAVE_SDL TRUE)

message(STATUS "  SDL2_LIBRARIES: ${SDL2_LIBRARIES}")
message(STATUS "  SDL2_INCLUDE_DIRS: ${SDL2_INCLUDE_DIRS}")
message(STATUS "  SDL2_LIBRARY_DIRS: ${SDL2_LIBRARY_DIRS}")
message(STATUS "  SDL2_CFLAGS: ${SDL2_CFLAGS}")
message(STATUS "  SDL2_LDFLAGS: ${SDL2_LDFLAGS}")
message(STATUS "  SDL2_CFLAGS_OTHER: ${SDL2_CFLAGS_OTHER}")
message(STATUS "  SDL2_LDFLAGS_OTHER: ${SDL2_LDFLAGS_OTHER}")

message(STATUS "  SDL2IMAGE_LIBRARIES: ${SDL2IMAGE_LIBRARIES}")
message(STATUS "  SDL2IMAGE_INCLUDE_DIRS: ${SDL2IMAGE_INCLUDE_DIRS}")
message(STATUS "  SDL2IMAGE_LIBRARY_DIRS: ${SDL2IMAGE_LIBRARY_DIRS}")
message(STATUS "  SDL2IMAGE_CFLAGS: ${SDL2IMAGE_CFLAGS}")
message(STATUS "  SDL2IMAGE_LDFLAGS: ${SDL2IMAGE_LDFLAGS}")
message(STATUS "  SDL2IMAGE_CFLAGS_OTHER: ${SDL2IMAGE_CFLAGS_OTHER}")
message(STATUS "  SDL2IMAGE_LDFLAGS_OTHER: ${SDL2IMAGE_LDFLAGS_OTHER}")

add_library(LibSDL2 INTERFACE IMPORTED)
set_target_properties(LibSDL2 PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIRS}"
  INTERFACE_COMPILE_OPTIONS "${SDL2_CFLAGS};${SDL2_CFLAGS_OTHER}"
  INTERFACE_LINK_LIBRARIES "${SDL2_LIBRARIES}"
  INTERFACE_LINK_DIRECTORIES "${SDL2_LIBRARY_DIRS}"
  INTERFACE_LINK_OPTIONS "${SDL2_LDFLAGS};${SDL2_LDFLAGS_OTHER}"
  )

add_library(LibSDL2main INTERFACE IMPORTED)
set_target_properties(LibSDL2main PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIRS}"
  INTERFACE_COMPILE_OPTIONS "${SDL2_CFLAGS};${SDL2_CFLAGS_OTHER}"
  INTERFACE_LINK_LIBRARIES "${SDL2MAIN_LIBRARIES}"
  INTERFACE_LINK_DIRECTORIES "${SDL2_LIBRARY_DIRS}"
  INTERFACE_LINK_OPTIONS "${SDL2_LDFLAGS};${SDL2_LDFLAGS_OTHER}"
  )

add_library(LibSDL2_image INTERFACE IMPORTED)
set_target_properties(LibSDL2_image PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${SDL2IMAGE_INCLUDE_DIRS}"
  INTERFACE_COMPILE_OPTIONS "${SDL2IMAGE_CFLAGS};${SDL2IMAGE_CFLAGS_OTHER}"
  INTERFACE_LINK_LIBRARIES "${SDL2IMAGE_LIBRARIES}"
  INTERFACE_LINK_DIRECTORIES "${SDL2IMAGE_LIBRARY_DIRS}"
  INTERFACE_LINK_OPTIONS "${SDL2IMAGE_LDFLAGS};${SDL2IMAGE_LDFLAGS_OTHER}"
  )

mark_as_advanced(
  SDL2_INCLUDE_DIRS
  SDL2_LIBRARIES
  )

mark_as_advanced(
  SDL2IMAGE_INCLUDE_DIRS
  SDL2IMAGE_LIBRARIES
  )

# EOF #
