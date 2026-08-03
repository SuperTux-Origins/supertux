#!/usr/bin/env bash
# Builds SDL2 (+ optional SDL2_image) as static libraries for wasm32 via
# emcmake/emmake. Expects (set by nix/wasm.nix):
#   SDL_SRC          - extracted SDL2 source tree
#   SDL_IMAGE_SRC    - optional extracted SDL2_image source tree
#   NIX_BUILD_CORES  - parallel jobs
#
# Installs into $PWD/prefix so SDL2_image can find SDL2 via CMAKE_PREFIX_PATH.
# Deliberately does NOT use Emscripten's -sUSE_SDL=2 port (needs network).
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

# Ensure a plain libSDL2.a name exists for our CMake SDL2_ROOT logic.
if [ ! -f "$PREFIX/lib/libSDL2.a" ]; then
  find build-sdl2 -name 'libSDL2.a' -exec cp {} "$PREFIX/lib/" \; || true
fi
mkdir -p "$PREFIX/include"
if [ ! -d "$PREFIX/include/SDL2" ] && [ -d SDL2-src/include ]; then
  cp -a SDL2-src/include/. "$PREFIX/include/" || true
fi

if [ -n "${SDL_IMAGE_SRC:-}" ]; then
  echo "==> SDL2_image static (wasm32, stb backend)"
  cp -a "$SDL_IMAGE_SRC" SDL2_image-src
  chmod -R u+w SDL2_image-src
  mkdir -p build-sdl2-image
  cd build-sdl2-image

  set +e
  emcmake cmake ../SDL2_image-src \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$PREFIX" \
    -DCMAKE_PREFIX_PATH="$PREFIX" \
    -DBUILD_SHARED_LIBS=OFF \
    -DSDL2IMAGE_SAMPLES=OFF \
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
    echo "retry SDL2_image with minimal options..."
    rm -rf ./*
    emcmake cmake ../SDL2_image-src \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX="$PREFIX" \
      -DCMAKE_PREFIX_PATH="$PREFIX" \
      -DBUILD_SHARED_LIBS=OFF \
      -DSDL2IMAGE_SAMPLES=OFF
  fi
  emmake make -j"${NIX_BUILD_CORES:-$(nproc)}"
  emmake make install || true
  if [ ! -f "$PREFIX/lib/libSDL2_image.a" ]; then
    find . -name 'libSDL2_image.a' -exec cp {} "$PREFIX/lib/" \; || true
  fi
  if [ ! -f "$PREFIX/include/SDL_image.h" ] && [ -f ../SDL2_image-src/include/SDL_image.h ]; then
    cp ../SDL2_image-src/include/SDL_image.h "$PREFIX/include/"
  fi
  cd ..
fi

echo "==> wasm SDL libs build finished (prefix=$PREFIX)"
ls -la "$PREFIX/lib" || true
ls -la "$PREFIX/include" | head -20 || true
