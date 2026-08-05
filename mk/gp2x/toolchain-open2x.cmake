# CMake toolchain file for Open2x (GP2X apps toolchain).
#
# Usage:
#   export OPEN2X_ROOT=/opt/open2x/gcc-4.1.1-glibc-2.3.6
#   cmake -S . -B build-gp2x \
#     -DCMAKE_TOOLCHAIN_FILE=mk/gp2x/toolchain-open2x.cmake \
#     -DENABLE_GP2X=ON -DENABLE_RES320X240=ON \
#     -DENABLE_SDL2=OFF -DENABLE_OPENGL=OFF -DENABLE_SOUND=OFF \
#     -DOPEN2X_ROOT=$OPEN2X_ROOT
#
# See mk/gp2x/CROSSCOMPILE.md

if(NOT DEFINED ENV{OPEN2X_ROOT} AND NOT OPEN2X_ROOT)
  set(OPEN2X_ROOT "/opt/open2x/gcc-4.1.1-glibc-2.3.6" CACHE PATH "Open2x apps toolchain + libpack prefix")
elseif(DEFINED ENV{OPEN2X_ROOT} AND NOT OPEN2X_ROOT)
  set(OPEN2X_ROOT "$ENV{OPEN2X_ROOT}" CACHE PATH "Open2x apps toolchain + libpack prefix")
endif()

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(OPEN2X_TRIPLE "arm-open2x-linux")
set(CMAKE_C_COMPILER   "${OPEN2X_ROOT}/bin/${OPEN2X_TRIPLE}-gcc")
set(CMAKE_CXX_COMPILER "${OPEN2X_ROOT}/bin/${OPEN2X_TRIPLE}-g++")
set(CMAKE_AR           "${OPEN2X_ROOT}/bin/${OPEN2X_TRIPLE}-ar"      CACHE FILEPATH "" FORCE)
set(CMAKE_RANLIB       "${OPEN2X_ROOT}/bin/${OPEN2X_TRIPLE}-ranlib"  CACHE FILEPATH "" FORCE)
set(CMAKE_STRIP        "${OPEN2X_ROOT}/bin/${OPEN2X_TRIPLE}-strip"   CACHE FILEPATH "" FORCE)

# Soft-float userland (classic Open2x libpack).
set(OPEN2X_FLAGS "-msoft-float -fomit-frame-pointer")
set(CMAKE_C_FLAGS_INIT   "${OPEN2X_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${OPEN2X_FLAGS}")

set(CMAKE_FIND_ROOT_PATH "${OPEN2X_ROOT}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Hint for CMakeLists.txt SDL discovery.
set(OPEN2X_ROOT "${OPEN2X_ROOT}" CACHE PATH "Open2x prefix" FORCE)
