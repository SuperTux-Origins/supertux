#!/usr/bin/env bash
# Builds SDL2 (+ optional SDL2_image, SDL2_mixer, libxmp) as static libraries
# for wasm32 via emcmake/emmake. Expects (set by nix/wasm.nix):
#   SDL_SRC          - extracted SDL2 source tree
#   SDL_IMAGE_SRC    - optional extracted SDL2_image source tree
#   SDL_MIXER_SRC    - optional extracted SDL2_mixer source tree
#   LIBXMP_SRC       - optional libxmp source (MOD/XM for Milestone 1 music)
#   NIX_BUILD_CORES  - parallel jobs
#
# Installs into $PWD/prefix so dependents can find SDL2 via explicit
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

# Paths for SDL2_image / SDL2_mixer FindPrivateSDL2 / sdl_find_sdl2
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

# --- SDL2_mixer + libxmp (MOD/XM for Milestone 1 music) ---
# Mirror Android: vendor libxmp into SDL2_mixer/external/libxmp and build with
# SDL2MIXER_VENDORED + MOD_XMP so the static mixer actually decodes .mod/.xm.
# A system-style find_package often silently drops MOD under emcmake.
if [ -n "${SDL_MIXER_SRC:-}" ]; then
  if [ -z "${LIBXMP_SRC:-}" ]; then
    echo "error: SDL_MIXER_SRC set but LIBXMP_SRC unset — MOD music will not work" >&2
    exit 1
  fi

  echo "==> SDL2_mixer static (wasm32) with vendored libxmp"
  cp -a "$SDL_MIXER_SRC" SDL2_mixer-src
  chmod -R u+w SDL2_mixer-src

  # Vendor libxmp where SDL2_mixer CMake expects it.
  rm -rf SDL2_mixer-src/external/libxmp
  mkdir -p SDL2_mixer-src/external
  cp -a "$LIBXMP_SRC" SDL2_mixer-src/external/libxmp
  chmod -R u+w SDL2_mixer-src/external/libxmp

  # libxmp 4.6 still declares cmake_minimum_required < 3.5; modern CMake rejects it.
  if [ -f SDL2_mixer-src/external/libxmp/CMakeLists.txt ]; then
    sed -i -E 's/cmake_minimum_required\s*\(\s*VERSION\s+[0-9.]+/cmake_minimum_required(VERSION 3.16/' \
      SDL2_mixer-src/external/libxmp/CMakeLists.txt || true
  fi

  # Also install a standalone libxmp.a into PREFIX for the game link line
  # (some mixer builds only reference symbols, not merge the archive).
  echo "==> libxmp static → $PREFIX (standalone + vendored)"
  mkdir -p build-libxmp
  (
    cd build-libxmp
    set +e
    emcmake cmake ../SDL2_mixer-src/external/libxmp \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX="$PREFIX" \
      -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
      -DBUILD_SHARED=OFF \
      -DBUILD_STATIC=ON \
      -DBUILD_SHARED_LIBS=OFF \
      -DLIBXMP_DISABLE_DEPACKERS=ON \
      -DLIBXMP_DISABLE_PROWIZARD=ON \
      -DCMAKE_C_FLAGS="-O2 -DHAVE_ROUND"
    xmp_cfg=$?
    set -e
    if [ "$xmp_cfg" -eq 0 ]; then
      emmake make -j"${NIX_BUILD_CORES:-$(nproc)}"
      emmake make install || true
    fi
  )
  if [ ! -f "$PREFIX/lib/libxmp.a" ]; then
    find build-libxmp SDL2_mixer-src/external/libxmp -name 'libxmp*.a' 2>/dev/null | head -20
    find . -name 'libxmp.a' -exec cp -v {} "$PREFIX/lib/libxmp.a" \; || true
    find . -name 'libxmp_static.a' -exec cp -v {} "$PREFIX/lib/libxmp.a" \; || true
  fi
  if [ ! -f "$PREFIX/lib/libxmp.a" ]; then
    # Autotools fallback — must declare wasm host or configure tries to run a.out.
    (
      cd SDL2_mixer-src/external/libxmp
      if [ -f configure ] || [ -f configure.ac ]; then
        [ -f configure ] || autoreconf -fi || true
        emconfigure ./configure \
          --host=wasm32-unknown-emscripten \
          --build="$(cc -dumpmachine 2>/dev/null || echo x86_64-pc-linux-gnu)" \
          --prefix="$PREFIX" \
          --enable-static \
          --disable-shared \
          --disable-depackers \
          --disable-prowizard \
          CFLAGS="-O2 -DHAVE_ROUND"
        emmake make -j"${NIX_BUILD_CORES:-$(nproc)}"
        emmake make install || true
      fi
    )
  fi
  if [ ! -f "$PREFIX/lib/libxmp.a" ]; then
    echo "error: libxmp.a not produced — cannot enable MOD music" >&2
    exit 1
  fi
  mkdir -p "$PREFIX/lib/pkgconfig" "$PREFIX/include"
  # Ensure xmp.h is visible
  if [ ! -f "$PREFIX/include/xmp.h" ]; then
    find SDL2_mixer-src/external/libxmp -name 'xmp.h' -exec cp -v {} "$PREFIX/include/" \; || true
  fi
  cat > "$PREFIX/lib/pkgconfig/libxmp.pc" <<EOF
prefix=$PREFIX
exec_prefix=\${prefix}
libdir=\${prefix}/lib
includedir=\${prefix}/include
Name: libxmp
Description: libxmp (wasm static)
Version: 4.6.0
Libs: -L\${libdir} -lxmp
Cflags: -I\${includedir}
EOF
  # Alias some finders use
  cp -f "$PREFIX/lib/pkgconfig/libxmp.pc" "$PREFIX/lib/pkgconfig/xmp.pc"
  echo "==> libxmp ready: $(ls -la "$PREFIX/lib/libxmp.a")"

  mkdir -p build-sdl2-mixer
  cd build-sdl2-mixer

  export PKG_CONFIG_PATH="$PREFIX/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"

  mixer_args=(
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_INSTALL_PREFIX="$PREFIX"
    -DCMAKE_PREFIX_PATH="$PREFIX"
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5
    -DSDL2_DIR="$PREFIX/lib/cmake/SDL2"
    -DSDL2_LIBRARY="$SDL2_LIB"
    -DSDL2_INCLUDE_DIR="$SDL2_INC"
    -DPrivateSDL2_LIBRARY="$SDL2_LIB"
    -DPrivateSDL2_INCLUDE_DIR="$SDL2_INC"
    -DBUILD_SHARED_LIBS=OFF
    -DSDL2MIXER_SAMPLES=OFF
    -DSDL2MIXER_CMD=OFF
    -DSDL2MIXER_VENDORED=ON
    -DSDL2MIXER_FLAC=OFF
    -DSDL2MIXER_GME=OFF
    -DSDL2MIXER_MOD=ON
    -DSDL2MIXER_MOD_XMP=ON
    -DSDL2MIXER_MOD_MODPLUG=OFF
    -DSDL2MIXER_MP3=OFF
    -DSDL2MIXER_MIDI=OFF
    -DSDL2MIXER_OPUS=OFF
    -DSDL2MIXER_WAVPACK=OFF
    -DSDL2MIXER_VORBIS=STB
    -DSDL2MIXER_DEPS_SHARED=OFF
  )

  set +e
  emcmake cmake ../SDL2_mixer-src "${mixer_args[@]}"
  mix_cfg=$?
  set -e
  if [ "$mix_cfg" -ne 0 ]; then
    echo "retry SDL2_mixer cmake with explicit xmp paths..."
    rm -rf ./*
    emcmake cmake ../SDL2_mixer-src \
      "${mixer_args[@]}" \
      -Dxmp_LIBRARY="$PREFIX/lib/libxmp.a" \
      -Dxmp_INCLUDE_PATH="$PREFIX/include" \
      -Dlibxmp_LIBRARY="$PREFIX/lib/libxmp.a" \
      -Dlibxmp_INCLUDE_DIR="$PREFIX/include"
  fi
  emmake make -j"${NIX_BUILD_CORES:-$(nproc)}"
  emmake make install || true

  if [ ! -f "$PREFIX/lib/libSDL2_mixer.a" ]; then
    find . -name 'libSDL2_mixer.a' -exec cp -v {} "$PREFIX/lib/" \; || true
  fi
  if [ ! -f "$PREFIX/lib/libSDL2_mixer.a" ]; then
    echo "error: libSDL2_mixer.a not produced" >&2
    exit 1
  fi

  # Headers
  if [ ! -f "$PREFIX/include/SDL_mixer.h" ]; then
    if [ -f ../SDL2_mixer-src/include/SDL_mixer.h ]; then
      cp ../SDL2_mixer-src/include/SDL_mixer.h "$PREFIX/include/"
    elif [ -f ../SDL2_mixer-src/SDL_mixer.h ]; then
      cp ../SDL2_mixer-src/SDL_mixer.h "$PREFIX/include/"
    fi
  fi
  if [ -f "$PREFIX/include/SDL_mixer.h" ] && [ ! -f "$PREFIX/include/SDL2/SDL_mixer.h" ]; then
    mkdir -p "$PREFIX/include/SDL2"
    cp "$PREFIX/include/SDL_mixer.h" "$PREFIX/include/SDL2/"
  fi

  # Confirm the static mixer actually references xmp (nm may be llvm-nm).
  if command -v llvm-nm >/dev/null 2>&1 || command -v nm >/dev/null 2>&1; then
    NM=$(command -v llvm-nm || command -v nm)
    if $NM "$PREFIX/lib/libSDL2_mixer.a" 2>/dev/null | grep -qi 'xmp_'; then
      echo "==> SDL2_mixer archive references xmp symbols (good)"
    else
      echo "warning: no xmp symbols found in libSDL2_mixer.a — game must link libxmp.a" >&2
    fi
  fi

  cd ..
  echo "==> SDL2_mixer ready: $(ls -la "$PREFIX/lib/libSDL2_mixer.a")"
  ls -la "$PREFIX/lib"/libxmp* "$PREFIX/lib"/libSDL2_mixer* 2>/dev/null || true
fi

echo "==> wasm SDL libs build finished (prefix=$PREFIX)"
ls -la "$PREFIX/lib" || true
ls -la "$PREFIX/include" | head -30 || true
