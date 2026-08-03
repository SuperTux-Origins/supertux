#!/usr/bin/env bash
# Builds SDL2 (+ optional SDL2_image) as static libraries for wasm32 via
# emcmake/emmake. Expects (set by nix/wasm.nix):
#   SDL_SRC          - extracted SDL2 source tree
#   SDL_IMAGE_SRC    - optional extracted SDL2_image source tree
#   NIX_BUILD_CORES  - parallel jobs
#
# Deliberately does NOT use Emscripten's -sUSE_SDL=2 port (needs network in
# the sandbox). Output layout after installPhase in wasm.nix:
#   $out/lib/libSDL2.a  $out/lib/libSDL2_image.a  $out/include/...
set -euo pipefail

export EM_CACHE="${TMPDIR:-/tmp}/emcache"
mkdir -p "$EM_CACHE"

echo "==> SDL2 static (wasm32)"
cp -a "$SDL_SRC" SDL2-src
chmod -R u+w SDL2-src
mkdir -p build-sdl2
cd build-sdl2
emcmake cmake ../SDL2-src \
  -DCMAKE_BUILD_TYPE=Release \
  -DSDL_SHARED=OFF \
  -DSDL_STATIC=ON \
  -DSDL_TEST=OFF
emmake make -j"${NIX_BUILD_CORES:-$(nproc)}"
cd ..

if [ -n "${SDL_IMAGE_SRC:-}" ]; then
  echo "==> SDL2_image static (wasm32, stb backend preferred)"
  cp -a "$SDL_IMAGE_SRC" SDL2_image-src
  chmod -R u+w SDL2_image-src
  mkdir -p build-sdl2-image
  cd build-sdl2-image
  # Point image at the SDL2 we just built (headers + static lib).
  emcmake cmake ../SDL2_image-src \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=OFF \
    -DSDL2IMAGE_SAMPLES=OFF \
    -DSDL2IMAGE_VENDORED=ON \
    -DSDL2IMAGE_BACKEND_STB=ON \
    -DSDL2IMAGE_JPG=ON \
    -DSDL2IMAGE_PNG=ON \
    -DSDL2IMAGE_TIF=OFF \
    -DSDL2IMAGE_WEBP=OFF \
    -DSDL2_DIR="$PWD/../build-sdl2" \
    -DSDL2_INCLUDE_DIR="$PWD/../SDL2-src/include" \
    -DSDL2_LIBRARY="$PWD/../build-sdl2/libSDL2.a"
  emmake make -j"${NIX_BUILD_CORES:-$(nproc)}" || {
    # Older SDL2_image releases use different option names; retry minimal.
    echo "retry SDL2_image with reduced options..."
    rm -rf ./*
    emcmake cmake ../SDL2_image-src \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_SHARED_LIBS=OFF \
      -DSDL2_DIR="$PWD/../build-sdl2" \
      -DSDL2_INCLUDE_DIR="$PWD/../SDL2-src/include" \
      -DSDL2_LIBRARY="$PWD/../build-sdl2/libSDL2.a"
    emmake make -j"${NIX_BUILD_CORES:-$(nproc)}"
  }
  cd ..
fi

echo "==> wasm SDL libs build finished"
