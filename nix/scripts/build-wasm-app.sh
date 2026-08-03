#!/usr/bin/env bash
# Cross-compiles SuperTux Milestone 1 to wasm32 + HTML via emcmake.
# Expects (set by nix/wasm.nix):
#   APP_NAME         - binary/html basename (e.g. supertux-milestone1)
#   SRC_DIR          - repo root (contains CMakeLists.txt + src/)
#   SDL_WASM_LIBS    - prebuilt SDL2 (+ image) prefix (include/ + lib/)
#   DATA_DIR         - optional path to data/ for --preload-file (may be absent)
#   ENABLE_SOUND     - 0|1 (default 0 for first bring-up; mixer not in wasm libs yet)
#   ENABLE_GLES2     - 0|1 (default 1 — WebGL via GLES2 path)
#   CMAKE_VERBOSE    - if 1, pass --verbose to cmake --build
set -euo pipefail

export EM_CACHE="${TMPDIR:-/tmp}/emcache"
mkdir -p "$EM_CACHE"

# Static zlib for wasm (lispreader / .gz levels). Offline — no -sUSE_ZLIB port.
ZLIB_PREFIX="$PWD/zlib-prefix"
if [ -n "${ZLIB_SRC:-}" ]; then
  echo "==> zlib static (wasm32) → $ZLIB_PREFIX"
  rm -rf zlib-src build-zlib
  # Nix zlib.src is often a .tar.gz path; unpack or copy tree.
  if [ -d "$ZLIB_SRC" ]; then
    cp -a "$ZLIB_SRC" zlib-src
  else
    mkdir -p zlib-src
    tar -xf "$ZLIB_SRC" -C zlib-src --strip-components=1
  fi
  chmod -R u+w zlib-src
  # Prefer classic configure: zlib's CMake still add_library(SHARED), which
  # Emscripten rejects even with -DBUILD_SHARED_LIBS=OFF.
  (
    cd zlib-src
    if [ -x ./configure ] || [ -f ./configure ]; then
      emconfigure ./configure --static --prefix="$ZLIB_PREFIX"
      emmake make -j"${NIX_BUILD_CORES:-$(nproc)}"
      emmake make install
    else
      # Very old / odd tree: compile the few .c files by hand into libz.a
      echo "zlib: no configure — compiling sources with emcc"
      mkdir -p "$ZLIB_PREFIX/lib" "$ZLIB_PREFIX/include"
      objs=()
      for c in adler32 compress crc32 deflate gzclose gzlib gzread gzwrite \
               infback inffast inflate inftrees trees uncompr zutil; do
        if [ -f "${c}.c" ]; then
          emcc -O2 -c "${c}.c" -o "${c}.o"
          objs+=("${c}.o")
        fi
      done
      emar rcs "$ZLIB_PREFIX/lib/libz.a" "${objs[@]}"
      cp zlib.h zconf.h "$ZLIB_PREFIX/include/" 2>/dev/null || cp zlib.h "$ZLIB_PREFIX/include/"
    fi
  )
  ls -la "$ZLIB_PREFIX/lib" "$ZLIB_PREFIX/include" || true
else
  echo "WARNING: ZLIB_SRC unset — configure may fail on zlib.h"
  ZLIB_PREFIX=""
fi

APP_NAME="${APP_NAME:-supertux-milestone1}"
ENABLE_SOUND="${ENABLE_SOUND:-0}"
ENABLE_GLES2="${ENABLE_GLES2:-1}"

# ASYNCIFY: temporary so nested title/gameloop/worldmap while+SDL_Delay loops
# do not freeze the browser tab. Replace with a real frame callback later
# (see TODO.md Phase 5).
LINK_FLAGS=(
  "SHELL:-sALLOW_MEMORY_GROWTH=1"
  "SHELL:-sASYNCIFY=1"
  "SHELL:-sASYNCIFY_STACK_SIZE=1048576"
  "SHELL:-sFULL_ES2=1"
  "SHELL:-sMIN_WEBGL_VERSION=1"
  "SHELL:-sMAX_WEBGL_VERSION=2"
  "SHELL:-sFORCE_FILESYSTEM=1"
  "SHELL:-sEXIT_RUNTIME=0"
)

PRELOAD=()
if [ -n "${DATA_DIR:-}" ] && [ -d "$DATA_DIR" ]; then
  # Mount game assets at /data in the virtual FS; runtime datadir = "/data".
  PRELOAD+=("--preload-file" "${DATA_DIR}@/data")
  echo "==> preloading data/ → /data"
else
  echo "==> no DATA_DIR — building without assets (title will fail at runtime)"
fi

cmake_args=(
  -S "$SRC_DIR"
  -B build
  -DCMAKE_BUILD_TYPE=Release
  -DENABLE_SDL2=ON
  -DENABLE_OPENGL=ON
  -DENABLE_GLES2="$( [ "$ENABLE_GLES2" = 1 ] && echo ON || echo OFF )"
  -DENABLE_SOUND="$( [ "$ENABLE_SOUND" = 1 ] && echo ON || echo OFF )"
  -DDATA_PREFIX="/data"
  -DSDL2_ROOT="$SDL_WASM_LIBS"
  -DEMSCRIPTEN_LINK_FLAGS="${LINK_FLAGS[*]} ${PRELOAD[*]}"
)
if [ -n "$ZLIB_PREFIX" ] && [ -d "$ZLIB_PREFIX" ]; then
  cmake_args+=(-DZLIB_ROOT="$ZLIB_PREFIX")
fi

echo "==> emcmake configure ${APP_NAME}"
emcmake cmake "${cmake_args[@]}"

verbose=()
if [ "${CMAKE_VERBOSE:-0}" = 1 ]; then
  verbose=(--verbose)
fi

echo "==> cmake --build"
cmake --build build --parallel "${NIX_BUILD_CORES:-$(nproc)}" "${verbose[@]}"

# Emscripten names the outputs after the CMake target (supertux-milestone1).
out_base="build/${APP_NAME}"
for ext in html js wasm data; do
  if [ -f "${out_base}.${ext}" ]; then
    cp "${out_base}.${ext}" .
  fi
done
# Older emscripten sometimes writes .js next to a non-suffixed binary.
if [ ! -f "${APP_NAME}.html" ] && [ -f "build/${APP_NAME}" ]; then
  # May already be an html shell from SUFFIX.
  ls -la build/ || true
fi

ls -la "${APP_NAME}".* 2>/dev/null || ls -la build/
echo "==> wasm app build finished"
