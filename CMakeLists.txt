#
# SuperTux - root build script
# Copyright (C) 2006 Christoph Sommer <christoph.sommer@2006.expires.deltadevelopment.de>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#


#
# INSTRUCTIONS:
# -------------
#
# Create a directory build/ and change to it. Run
#
#   cmake ..
#
# This creates a set of Makefiles to build the project. Run
#
#   make
#

### CMake configuration

cmake_minimum_required(VERSION 3.1)
if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
  cmake_policy(SET CMP0008 NEW)
  cmake_policy(SET CMP0023 NEW)
  cmake_policy(SET CMP0048 NEW)
endif()

## Project name to use as command prefix.
project(SUPERTUX VERSION 0.6.2)

# FIXME: This is for FindOpusfile and some other stuff, could just
# copy them over or integrate better into wstsound
set(TINYCMMC_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/external/tinycmmc/modules/")
find_package(tinycmmc CONFIG)
message(STATUS "tinycmmc module path: ${TINYCMMC_MODULE_PATH}")
list(APPEND CMAKE_MODULE_PATH ${TINYCMMC_MODULE_PATH})

# Using C++20 here, C++17 would almost be enough, but is lacking filesystem on some platforms
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/mk/cmake)
include(ConfigureFiles)
include(ExternalProject)
include(CheckCXXCompilerFlag)
include(CheckSymbolExists)

## For autopackage
set(APPDATADIR "${CMAKE_INSTALL_PREFIX}/share/games/supertux-origins")

set(BUILD_DATA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data")
set(BUILD_CONFIG_DATA_DIR "${CMAKE_BINARY_DIR}/data")

# Options for install
if(WIN32 AND NOT UNIX)
  set(INSTALL_SUBDIR_BIN "bin" CACHE STRING "Installation subdir for binaries")
  set(INSTALL_SUBDIR_SHARE "data" CACHE STRING "Installation subdir for data")
  set(INSTALL_SUBDIR_DOC "doc" CACHE STRING "Installation subdir for docs")
else()
  if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin" AND DISABLE_CPACK_BUNDLING)
    set(INSTALL_SUBDIR_BIN "SuperTux.app/Contents/MacOS" CACHE STRING "Installation subdir for binaries")
    set(INSTALL_SUBDIR_SHARE "SuperTux.app/Contents/Resources/data" CACHE STRING "Installation subdir for data")
    set(INSTALL_SUBDIR_DOC "SuperTux.app/Contents/Resources" CACHE STRING "Installation subdir for docs")
  else()
    set(INSTALL_SUBDIR_BIN "games" CACHE STRING "Installation subdir for binaries")
    set(INSTALL_SUBDIR_SHARE "share/games/supertux-origins" CACHE STRING "Installation subdir for data")
    set(INSTALL_SUBDIR_DOC "share/doc/supertux-origins" CACHE STRING "Installation subdir for docs")
  endif()
endif()

if(EMSCRIPTEN)
  set(CMAKE_EXECUTABLE_SUFFIX .html)
  set(IS_EMSCRIPTEN_BUILD ON)

  set(USE_FLAGS " -sUSE_SDL=2 -sFULL_ES2=1 -sUSE_SDL_IMAGE=2 -sUSE_FREETYPE=2 -sUSE_PTHREADS=1 -sDISABLE_EXCEPTION_CATCHING=0 --preload-file ${BUILD_CONFIG_DATA_DIR} -sTOTAL_MEMORY=67108864 -sALLOW_MEMORY_GROWTH=1 -sSDL2_IMAGE_FORMATS='[\"png\",\"jpg\"]' --use-preload-plugins")
  if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(USE_FLAGS "${USE_FLAGS} -fsanitize=undefined -sSAFE_HEAP=1 -sASSERTIONS=1 -sDEMANGLE_SUPPORT=1")
  endif()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${USE_FLAGS} -sERROR_ON_UNDEFINED_SYMBOLS=0")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${USE_FLAGS} -sERROR_ON_UNDEFINED_SYMBOLS=0")
  set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} ${USE_FLAGS} -sERROR_ON_UNDEFINED_SYMBOLS=0 -lidbfs.js")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} ${USE_FLAGS} -sERROR_ON_UNDEFINED_SYMBOLS=0 -lidbfs.js")
endif()

# TODO: Add " OR ANDROID OR IOS" to this
if(EMSCRIPTEN OR UBUNTU_TOUCH OR ANDROID)
  option(REMOVE_QUIT_BUTTON "Remove the option to quit the game (useful on mobile devices)" ON)
else()
  option(REMOVE_QUIT_BUTTON "Remove the option to quit the game (useful on mobile devices)" OFF)
endif()

option(STEAM_BUILD "Prepare build for Steam" OFF)
option(IS_SUPERTUX_RELEASE "Build as official SuperTux release" OFF)

set(SUPERTUX_SYSTEM_NAME ${CMAKE_SYSTEM_NAME})
set(VCPKG_BUILD OFF CACHE BOOL "Use dependencies installed via vcpkg (not dependency package)")
set(VCPKG_APPLOCAL_DEPS ${VCPKG_BUILD} BOOL)

# Detect mobile builds
option(UBUNTU_TOUCH "Compile the project for an Ubuntu Touch target" OFF)

# Mobile builds
if(UBUNTU_TOUCH OR ANDROID)
  option(HIDE_NONMOBILE_OPTIONS "Hide options that are impractical on mobile devices (e. g. changing screen resolution)" ON)
else()
  option(HIDE_NONMOBILE_OPTIONS "Hide options that are impractical on mobile devices (e. g. changing screen resolution)" OFF)
endif()

if(WIN32)
  include(SuperTux/Win32)
endif()

## Check platform-dependent build options
include(ConfigureChecks)

## Some additional compiler switches
include(SuperTux/ClangTidy)
include(SuperTux/WarningFlags)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-aliasing")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  add_definitions(-DMACOSX)
endif()

# Build dependencies
function(build_dependencies)
  set(BUILD_TESTS OFF)

  find_package(wstsound)
  if(NOT TARGET wstsound::wstsound)
    add_subdirectory(external/wstsound/ EXCLUDE_FROM_ALL)
  endif()

  find_package(logmich)
  find_package(prio)
endfunction()
build_dependencies()

## Add lots of dependencies to compiler switches
include(SuperTux/ProvideGlm)
include(SuperTux/ProvideFmt)
include(SuperTux/ProvideSDL2)
# include(SuperTux/ProvideOpenAL)
include(SuperTux/ProvidePhysfs)
include(SuperTux/ProvideCurl)
include(SuperTux/ProvideSquirrel)
include(SuperTux/ProvideSDL2_ttf)
include(SuperTux/ProvideSexpcpp)
include(SuperTux/ProvideSavePNG)
include(SuperTux/ProvideOpenGL)

## Build stuff
include(SuperTux/BuildVersion)
include(SuperTux/BuildDocumentation)
include(SuperTux/BuildMessagePot)
include(SuperTux/BuildMiniswigWrapper)

## Build list of sources for supertux binary
file(GLOB SUPERTUX_SOURCES_C RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} external/obstack/*.c)

file(GLOB SUPERTUX_SOURCES_CXX RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*/*.cpp src/supertux/menu/*.cpp src/video/sdl/*.cpp src/video/null/*.cpp)
file(GLOB SUPERTUX_RESOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "${PROJECT_BINARY_DIR}/tmp/*.rc")

if(HAVE_OPENGL)
  file(GLOB SUPERTUX_OPENGL_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/video/gl/*.cpp)
  set(SUPERTUX_SOURCES_CXX ${SUPERTUX_SOURCES_CXX} ${SUPERTUX_OPENGL_SOURCES})
endif()

## Sort source lists to have deterministic linking order
list(SORT SUPERTUX_SOURCES_C)
list(SORT SUPERTUX_SOURCES_CXX)
list(SORT SUPERTUX_RESOURCES)

if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/src/scripting/wrapper.cpp)
  set(SUPERTUX_SOURCES_CXX ${SUPERTUX_SOURCES_CXX} ${CMAKE_CURRENT_SOURCE_DIR}/src/scripting/wrapper.cpp)
endif()

## On Windows, add an icon
if(WIN32)
  if(MINGW)
    add_custom_command(
      OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/supertux_rc.o
      COMMAND ${CMAKE_RC_COMPILER} -I${CMAKE_CURRENT_SOURCE_DIR}/data/images/engine/icons -i${CMAKE_CURRENT_SOURCE_DIR}/data/images/engine/icons/supertux.rc -o ${CMAKE_CURRENT_BINARY_DIR}/supertux_rc.o)
    set(SUPERTUX_SOURCES_C ${SUPERTUX_SOURCES_C} ${CMAKE_CURRENT_BINARY_DIR}/supertux_rc.o)
  endif()
endif()

## Generate supertux executable in the right place

#set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR})
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

## Add target for supertux binary

add_library(supertux-origins_c OBJECT ${SUPERTUX_SOURCES_C})
add_library(supertux-origins_lib STATIC ${CMAKE_BINARY_DIR}/version.h ${SUPERTUX_SOURCES_CXX} ${SUPERTUX_RESOURCES} $<TARGET_OBJECTS:supertux-origins_c>)
target_include_directories(supertux-origins_lib PUBLIC ${CMAKE_BINARY_DIR} src/)

if(WIN32)
  add_executable(supertux-origins WIN32 src/main.cpp ${CMAKE_CURRENT_SOURCE_DIR}/data/images/engine/icons/supertux.rc)
  target_link_libraries(supertux-origins LibSDL2main)
else()
  add_executable(supertux-origins src/main.cpp)
endif()
target_link_libraries(supertux-origins supertux-origins_lib)

set_target_properties(supertux-origins_lib PROPERTIES OUTPUT_NAME supertux-origins_lib)
set_target_properties(supertux-origins_lib PROPERTIES COMPILE_FLAGS "${SUPERTUX-ORIGINS_EXTRA_WARNING_FLAGS}")
if(EMSCRIPTEN)
  target_link_options(supertux-origins PUBLIC -sEXPORTED_FUNCTIONS=['_main','_set_resolution','_save_config','_onDownloadProgress','_onDownloadFinished','_onDownloadError','_onDownloadAborted','_getExceptionMessage'] PUBLIC -sEXPORTED_RUNTIME_METHODS=['ccall','cwrap'] PUBLIC -sEXTRA_EXPORTED_RUNTIME_METHODS=['ccall','cwrap'])
endif()

if(WIN32 AND NOT VCPKG_BUILD AND NOT MINGW)
  ## Copy dlls on windows
  add_custom_command(TARGET supertux-origins_lib POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${DEPENDENCY_FOLDER}/dll"
    $<TARGET_FILE_DIR:supertux-origins_lib>)
endif()

## Some additional include paths
target_include_directories(supertux-origins_lib SYSTEM PUBLIC
  external/obstack/
  )

# Include altivec wrapper on ppc
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^ppc.*")
  target_include_directories(supertux-origins_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/ppc)
endif()

## Link supertux binary with squirrel and other libraries
if(NOT EMSCRIPTEN)
  target_link_libraries(supertux-origins_lib PUBLIC LibSDL2 LibSDL2_image)
endif()

target_link_libraries(supertux-origins_lib PUBLIC LibSDL2_ttf)
target_link_libraries(supertux-origins_lib PUBLIC LibSquirrel)
target_link_libraries(supertux-origins_lib PUBLIC LibSqstdlib)
target_link_libraries(supertux-origins_lib PUBLIC LibSexp)
target_link_libraries(supertux-origins_lib PUBLIC LibSavePNG)
target_link_libraries(supertux-origins_lib PUBLIC LibGlm)
target_link_libraries(supertux-origins_lib PUBLIC LibFmt)
target_link_libraries(supertux-origins_lib PUBLIC LibPhysfs)
target_link_libraries(supertux-origins_lib PUBLIC wstsound::wstsound)
target_link_libraries(supertux-origins_lib PUBLIC prio::prio)
target_link_libraries(supertux-origins_lib PUBLIC logmich::logmich)

if(NOT EMSCRIPTEN AND NOT WIN32)
  find_package(xdgcpp CONFIG)
  target_link_libraries(supertux-origins_lib PUBLIC xdgcpp)
endif()

if(HAVE_OPENGL)
  target_link_libraries(supertux-origins_lib PUBLIC LibOpenGL)
endif()

if(NOT EMSCRIPTEN)
  target_link_libraries(supertux-origins_lib PUBLIC LibCurl)
endif()

## Install stuff
include(SuperTux/BuildInstall)

## Create config.h now that INSTALL_SUBDIR_* have been set.
configure_file(config.h.cmake ${CMAKE_BINARY_DIR}/config.h )

## Build tests
include(SuperTux/BuildTests)

## CPack/Installation-specific stuff
include(SuperTux/BuildCPack)

# move some config clutter to the advanced section
mark_as_advanced(
  INSTALL_SUBDIR_BIN
  INSTALL_SUBDIR_SHARE
  INSTALL_SUBDIR_DOC
  )

mark_as_advanced(
  CMAKE_BACKWARDS_COMPATIBILITY
  CMAKE_BUILD_TYPE
  CMAKE_INSTALL_PREFIX
  EXECUTABLE_OUTPUT_PATH
  LIBRARY_OUTPUT_PATH
  CMAKE_OSX_ARCHITECTURES
  CMAKE_OSX_SYSROOT
  )

mark_as_advanced(
  APPDATADIR
  )

# EOF #
