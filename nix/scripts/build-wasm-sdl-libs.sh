#!/usr/bin/env bash
# Builds SDL2 (+ optional SDL2_image) as static libraries for wasm32 via
# emcmake/emmake. Expects (set by nix/wasm.nix):
#   SDL_SRC          - extracted SDL2 source tree
#   SDL_IMAGE_SRC    - optional extracted SDL2_image source tree
#   NIX_BUILD_CORES  - parallel jobs
#
# Installs into $PWD/prefix so SDL2_image can find SDL2 via explicit
# SDL2_LIBRARY / SDL2_INCLUDE_DIR (SDL2_image's PrivateSDL2 finder does not
# always honour CMAKE_PREFIX_PATH alone under emcmake).
set -euo pipefail

export EM_CACHE="${TMPDIR:-/tmp}/emcache"
mkdir -p "$EM_CACHE"

PREFIX="$PWD/prefix"
mkdir -p "$PREFIX"

echo "==> SDL2 static (wasm32) → $PREFIX"
cp -a "$SDL_SRC" SDL2-src
chmod -R u+w SDL2-src
mkdir -p build-sdl2
cd build-sdl2
emcmake cmake ../SDL2-src \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$PREFIX" \
  -DSDL_SHARED=OFF \
  -DSDL_STATIC=ON \
  -DSDL_TEST=OFF \
  -DSDL_STATIC_PIC=ON
emmake make -j"${NIX_BUILD_CORES:-$(nproc)}"
emmake make install
cd ..

if [ ! -f "$PREFIX/lib/libSDL2.a" ]; then
  find build-sdl2 -name 'libSDL2.a' -exec cp {} "$PREFIX/lib/" \; || true
fi
mkdir -p "$PREFIX/include"
if [ ! -d "$PREFIX/include/SDL2" ] && [ -d SDL2-src/include ]; then
  cp -a SDL2-src/include/. "$PREFIX/include/" || true
fi

# Paths for SDL2_image's FindPrivateSDL2 / sdl_find_sdl2
SDL2_LIB="$PREFIX/lib/libSDL2.a"
# Prefer the installed SDL2/ subdir (SDL.h lives there after make install).
if [ -f "$PREFIX/include/SDL2/SDL.h" ]; then
  SDL2_INC="$PREFIX/include/SDL2"
elif [ -f "$PREFIX/include/SDL.h" ]; then
  SDL2_INC="$PREFIX/include"
else
  SDL2_INC="$PREFIX/include/SDL2"
fi

echo "==> SDL2 ready: lib=$SDL2_LIB include=$SDL2_INC"
ls -la "$SDL2_LIB"
ls -la "$SDL2_INC/SDL.h"

if [ -n "${SDL_IMAGE_SRC:-}" ]; then
  echo "==> SDL2_image static (wasm32, stb backend)"
  cp -a "$SDL_IMAGE_SRC" SDL2_image-src
  chmod -R u+w SDL2_image-src
  mkdir -p build-sdl2-image
  cd build-sdl2-image

  # Explicit SDL2_LIBRARY / SDL2_INCLUDE_DIR for FindPrivateSDL2.cmake.
  # Also pass SDL2_DIR for Config-mode discovery of the installed package.
  common_args=(
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_INSTALL_PREFIX="$PREFIX"
    -DCMAKE_PREFIX_PATH="$PREFIX"
    -DSDL2_DIR="$PREFIX/lib/cmake/SDL2"
    -DSDL2_LIBRARY="$SDL2_LIB"
    -DSDL2_INCLUDE_DIR="$SDL2_INC"
    -DPrivateSDL2_LIBRARY="$SDL2_LIB"
    -DPrivateSDL2_INCLUDE_DIR="$SDL2_INC"
    -DBUILD_SHARED_LIBS=OFF
    -DSDL2IMAGE_SAMPLES=OFF
  )

  set +e
  emcmake cmake ../SDL2_image-src \
    "${common_args[@]}" \
    -DSDL2IMAGE_VENDORED=ON \
    -DSDL2IMAGE_BACKEND_STB=ON \
    -DSDL2IMAGE_AVIF=OFF \
    -DSDL2IMAGE_BMP=ON \
    -DSDL2IMAGE_GIF=ON \
    -DSDL2IMAGE_JPG=ON \
    -DSDL2IMAGE_JXL=OFF \
    -DSDL2IMAGE_LBM=OFF \
    -DSDL2IMAGE_PCX=OFF \
    -DSDL2IMAGE_PNG=ON \
    -DSDL2IMAGE_PNM=OFF \
    -DSDL2IMAGE_QOI=OFF \
    -DSDL2IMAGE_SVG=OFF \
    -DSDL2IMAGE_TGA=OFF \
    -DSDL2IMAGE_TIF=OFF \
    -DSDL2IMAGE_WEBP=OFF \
    -DSDL2IMAGE_XCF=OFF \
    -DSDL2IMAGE_XPM=OFF \
    -DSDL2IMAGE_XV=OFF
  cfg=$?
  set -e
  if [ "$cfg" -ne 0 ]; then
    echo "retry SDL2_image with minimal options + explicit SDL2 paths..."
    rm -rf ./*
    emcmake cmake ../SDL2_image-src \
      "${common_args[@]}"
  fi
  emmake make -j"${NIX_BUILD_CORES:-$(nproc)}"
  emmake make install || true
  if [ ! -f "$PREFIX/lib/libSDL2_image.a" ]; then
    find . -name 'libSDL2_image.a' -exec cp {} "$PREFIX/lib/" \; || true
  fi
  if [ ! -f "$PREFIX/include/SDL_image.h" ]; then
    if [ -f ../SDL2_image-src/include/SDL_image.h ]; then
      cp ../SDL2_image-src/include/SDL_image.h "$PREFIX/include/"
    elif [ -f ../SDL2_image-src/SDL_image.h ]; then
      cp ../SDL2_image-src/SDL_image.h "$PREFIX/include/"
    fi
  fi
  # Also under include/SDL2 for #include <SDL2/SDL_image.h> style
  if [ -f "$PREFIX/include/SDL_image.h" ] && [ ! -f "$PREFIX/include/SDL2/SDL_image.h" ]; then
    mkdir -p "$PREFIX/include/SDL2"
    cp "$PREFIX/include/SDL_image.h" "$PREFIX/include/SDL2/"
  fi
  cd ..
fi

echo "==> wasm SDL libs build finished (prefix=$PREFIX)"
ls -la "$PREFIX/lib" || true
ls -la "$PREFIX/include" | head -20 || true
