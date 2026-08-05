# CMake toolchain file for OpenWiz (GP2X Wiz).
#
# Usage:
#   export OPENWIZ_ROOT=/opt/openwiz/toolchain
#   cmake -S . -B build-wiz \
#     -DCMAKE_TOOLCHAIN_FILE=mk/gp2x/toolchain-openwiz.cmake \
#     -DENABLE_GP2X=ON -DENABLE_RES320X240=ON \
#     -DENABLE_SDL2=OFF -DENABLE_OPENGL=OFF -DENABLE_SOUND=OFF \
#     -DOPENWIZ_ROOT=$OPENWIZ_ROOT
#
# Triple may be arm-openwiz-linux-gnu or arm-gp2xwiz-linux-gnu depending
# on the SDK drop; override OPENWIZ_TRIPLE if needed.
# See mk/gp2x/CROSSCOMPILE.md

if(NOT DEFINED ENV{OPENWIZ_ROOT} AND NOT OPENWIZ_ROOT)
  set(OPENWIZ_ROOT "/opt/openwiz/toolchain" CACHE PATH "OpenWiz toolchain + SDL prefix")
elseif(DEFINED ENV{OPENWIZ_ROOT} AND NOT OPENWIZ_ROOT)
  set(OPENWIZ_ROOT "$ENV{OPENWIZ_ROOT}" CACHE PATH "OpenWiz toolchain + SDL prefix")
endif()

if(NOT OPENWIZ_TRIPLE)
  if(EXISTS "${OPENWIZ_ROOT}/bin/arm-openwiz-linux-gnu-gcc")
    set(OPENWIZ_TRIPLE "arm-openwiz-linux-gnu")
  elseif(EXISTS "${OPENWIZ_ROOT}/bin/arm-gp2xwiz-linux-gnu-gcc")
    set(OPENWIZ_TRIPLE "arm-gp2xwiz-linux-gnu")
  else()
    set(OPENWIZ_TRIPLE "arm-openwiz-linux-gnu")
  endif()
endif()
set(OPENWIZ_TRIPLE "${OPENWIZ_TRIPLE}" CACHE STRING "OpenWiz compiler triple")

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER   "${OPENWIZ_ROOT}/bin/${OPENWIZ_TRIPLE}-gcc")
set(CMAKE_CXX_COMPILER "${OPENWIZ_ROOT}/bin/${OPENWIZ_TRIPLE}-g++")
set(CMAKE_AR           "${OPENWIZ_ROOT}/bin/${OPENWIZ_TRIPLE}-ar"      CACHE FILEPATH "" FORCE)
set(CMAKE_RANLIB       "${OPENWIZ_ROOT}/bin/${OPENWIZ_TRIPLE}-ranlib"  CACHE FILEPATH "" FORCE)
set(CMAKE_STRIP        "${OPENWIZ_ROOT}/bin/${OPENWIZ_TRIPLE}-strip"   CACHE FILEPATH "" FORCE)

# ARM926EJ-S; float ABI depends on the particular OpenWiz build — leave
# default unless the SDK docs require -msoft-float.
set(OPENWIZ_FLAGS "-mcpu=arm926ej-s -mtune=arm926ej-s -fomit-frame-pointer")
set(CMAKE_C_FLAGS_INIT   "${OPENWIZ_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${OPENWIZ_FLAGS}")

set(CMAKE_FIND_ROOT_PATH "${OPENWIZ_ROOT}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(OPENWIZ_ROOT "${OPENWIZ_ROOT}" CACHE PATH "OpenWiz prefix" FORCE)
