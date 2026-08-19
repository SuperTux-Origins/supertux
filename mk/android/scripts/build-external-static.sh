#!/usr/bin/env bash
# Build a single external/ CMake project as a static library for one Android ABI.
# Usage:
#   NDK=... ABI=arm64-v8a API=22 ./mk/android/scripts/build-external-static.sh external/logmich out/logmich
set -euo pipefail
SRC="${1:?path to external project}"
OUT="${2:?install prefix}"
: "${NDK:?set NDK to Android NDK root}"
: "${ABI:=arm64-v8a}"
: "${API:=22}"
CMAKE_TOOLCHAIN="$NDK/build/cmake/android.toolchain.cmake"
BUILD_DIR=$(mktemp -d)
cmake -S "$SRC" -B "$BUILD_DIR" \
  -DCMAKE_TOOLCHAIN_FILE="$CMAKE_TOOLCHAIN" \
  -DANDROID_ABI="$ABI" \
  -DANDROID_PLATFORM="android-$API" \
  -DANDROID_STL=c++_shared \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$OUT" \
  -DBUILD_TESTS=OFF \
  -DPRIO_USE_JSONCPP=OFF \
  -DWARNINGS=OFF \
  -DWERROR=OFF
cmake --build "$BUILD_DIR" --target install -j"$(nproc)"
echo "Installed to $OUT"
