#!/usr/bin/env bash
# Builds a SuperTux (Origins) APK, linking SDL2 as a prebuilt library.
# Adapted from Pingus mk/android/scripts/build-apk.sh.
# Required environment:
#   ANDROID_HOME, BUILD_TOOLS_VERSION, PACKAGE_PLATFORM, TARGET_ABIS
#   APP_NAME, APP_DIR          - android/ packaging dir (manifest, res, jni/)
#   GAME_SRC_DIR               - path to C++ sources (repo src/)
#   GAME_DATA_DIR              - required data/ tree packaged as assets
#   APPLICATION_MK, TOP_ANDROID_MK, SDL_PREBUILT_MK, SDL_ANDROID_LIBS
#   KEYSTORE, STB_IMAGE_H
#   SUPERTUX_VERSION / PINGUS_VERSION - full version string
set -euo pipefail


# Resolve NDK root: ndk-bundle (legacy) or ndk/<version> (current SDK layout).
resolve_ndk() {
  if [ -n "${ANDROID_NDK_HOME:-}" ] && [ -x "${ANDROID_NDK_HOME}/ndk-build" ]; then
    printf '%s' "$ANDROID_NDK_HOME"
    return
  fi
  if [ -z "${ANDROID_HOME:-}" ]; then
    echo "error: ANDROID_HOME is not set" >&2
    exit 1
  fi
  if [ -x "$ANDROID_HOME/ndk-bundle/ndk-build" ]; then
    printf '%s' "$ANDROID_HOME/ndk-bundle"
    return
  fi
  if [ -d "$ANDROID_HOME/ndk" ]; then
    # Prefer ANDROID_NDK_VERSION when set; else newest directory that has ndk-build.
    if [ -n "${ANDROID_NDK_VERSION:-}" ] && [ -x "$ANDROID_HOME/ndk/$ANDROID_NDK_VERSION/ndk-build" ]; then
      printf '%s' "$ANDROID_HOME/ndk/$ANDROID_NDK_VERSION"
      return
    fi
    newest=
    for d in "$ANDROID_HOME/ndk"/*; do
      [ -x "$d/ndk-build" ] || continue
      newest=$d
    done
    if [ -n "$newest" ]; then
      printf '%s' "$newest"
      return
    fi
  fi
  echo "error: no ndk-build under ANDROID_HOME=$ANDROID_HOME (tried ndk-bundle and ndk/*)" >&2
  exit 1
}

NDK="$(resolve_ndk)"
echo "==> NDK=$NDK"
BT="$ANDROID_HOME/build-tools/$BUILD_TOOLS_VERSION"
PACKAGE_JAR="$ANDROID_HOME/platforms/android-$PACKAGE_PLATFORM/android.jar"

if [ -z "${GAME_SRC_DIR:-}" ] || [ ! -d "$GAME_SRC_DIR" ]; then
  echo "error: GAME_SRC_DIR must point at the game C++ source tree" >&2
  exit 1
fi

if [ -z "${GAME_DATA_DIR:-}" ] || [ ! -d "$GAME_DATA_DIR" ]; then
  echo "error: GAME_DATA_DIR must point at the SuperTux data/ tree" >&2
  echo "       (expected images/, levels/, levelsets/, music/, … under that path)" >&2
  exit 1
fi

mkdir -p src/jni/src src/jni/SDL
cp "$APPLICATION_MK" src/jni/Application.mk
cp "$TOP_ANDROID_MK" src/jni/Android.mk
cp "$APP_DIR/jni/Android.mk" src/jni/src/Android.mk
# Companion files for the module Android.mk (LOCAL_PATH = jni/src).
if [ -f "$APP_DIR/jni/freetype_Android.mk" ]; then
  cp "$APP_DIR/jni/freetype_Android.mk" src/jni/src/freetype_Android.mk
fi
if [ -f "$APP_DIR/jni/sdl_ttf_stub.c" ]; then
  cp "$APP_DIR/jni/sdl_ttf_stub.c" src/jni/src/sdl_ttf_stub.c
fi
if [ -f "$APP_DIR/jni/SDL_ttf.h" ]; then
  cp "$APP_DIR/jni/SDL_ttf.h" src/jni/src/SDL_ttf.h
  cp "$APP_DIR/jni/SDL_ttf.h" src/jni/SDL_ttf.h 2>/dev/null || true
fi
# placeholder.cpp intentionally not staged — full game sources are used.
cp "$APP_DIR/AndroidManifest.xml" src/AndroidManifest.xml
cp -r "$APP_DIR/res" src/res

# Game C++ sources next to the module Android.mk.
cp -r "$GAME_SRC_DIR"/. src/jni/src/
chmod -R u+rwX src/jni/src
# Re-assert module Android.mk after the source tree copy so a stray file
# under GAME_SRC_DIR can never replace it (SDL2 prebuilts live in jni/SDL/).
cp "$APP_DIR/jni/Android.mk" src/jni/src/Android.mk
# Companion files for the module Android.mk (LOCAL_PATH = jni/src).
if [ -f "$APP_DIR/jni/freetype_Android.mk" ]; then
  cp "$APP_DIR/jni/freetype_Android.mk" src/jni/src/freetype_Android.mk
fi
if [ -f "$APP_DIR/jni/sdl_ttf_stub.c" ]; then
  cp "$APP_DIR/jni/sdl_ttf_stub.c" src/jni/src/sdl_ttf_stub.c
fi
if [ -f "$APP_DIR/jni/SDL_ttf.h" ]; then
  cp "$APP_DIR/jni/SDL_ttf.h" src/jni/src/SDL_ttf.h
  cp "$APP_DIR/jni/SDL_ttf.h" src/jni/SDL_ttf.h 2>/dev/null || true
fi
# Generated/config headers (cmake would write these into the build dir).
for hdr in config.h version.h SDL_image.h; do
  if [ -f "$APP_DIR/jni/$hdr" ]; then
    cp "$APP_DIR/jni/$hdr" "src/jni/src/$hdr"
    echo "==> staged $hdr into jni/src/"
  else
    echo "warning: missing $APP_DIR/jni/$hdr" >&2
  fi
done
# NDK format_error key functions missing from libc++_shared — compile in stub.
if [ -f "$APP_DIR/jni/format_error_stub.cpp" ]; then
  cp "$APP_DIR/jni/format_error_stub.cpp" src/jni/src/format_error_stub.cpp
  echo "==> staged format_error_stub.cpp into jni/src/"
fi
# NDK also searches LOCAL_PATH parent includes; keep a copy under jni/
mkdir -p src/jni
for hdr in config.h version.h SDL_image.h; do
  if [ -f "src/jni/src/$hdr" ]; then
    cp "src/jni/src/$hdr" "src/jni/$hdr"
  fi
done

# SDL2_ttf + FreeType: real font path (TTF_OpenFontRW via PhysFS).
TTF_SRC="${SDL2_TTF_SOURCE_DIR:-}"
FT_SRC="${FREETYPE_SOURCE_DIR:-}"
HAVE_REAL_TTF=0
if [ -n "$TTF_SRC" ] && [ -d "$TTF_SRC" ] && [ -n "$FT_SRC" ] && [ -d "$FT_SRC" ]; then
  if [ -f "$TTF_SRC/SDL_ttf.h" ] && [ -f "$TTF_SRC/SDL_ttf.c" ] \
     && [ -f "$FT_SRC/include/ft2build.h" ]; then
    # LOCAL_PATH for module is jni/src — stage FreeType and SDL_ttf there.
    mkdir -p src/jni/src/freetype
    cp -a "$FT_SRC/include" src/jni/src/freetype/
    cp -a "$FT_SRC/src" src/jni/src/freetype/
    if [ ! -f src/jni/src/freetype/include/ft2build.h ]; then
      if [ -f src/jni/src/freetype/include/freetype2/ft2build.h ]; then
        ln -sf freetype2/ft2build.h src/jni/src/freetype/include/ft2build.h
      fi
    fi
    cp -a "$TTF_SRC/SDL_ttf.h" src/jni/src/
    cp -a "$TTF_SRC/SDL_ttf.h" src/jni/ 2>/dev/null || true
    cp -a "$TTF_SRC/SDL_ttf.c" src/jni/src/SDL_ttf.c
    chmod -R u+rwX src/jni/src/freetype src/jni/src/SDL_ttf.c
    # Drop stub so only real SDL_ttf is compiled when FreeType is present.
    rm -f src/jni/src/sdl_ttf_stub.c
    HAVE_REAL_TTF=1
    echo "==> staged FreeType from $FT_SRC and SDL_ttf.c from $TTF_SRC"
  fi
fi
if [ "$HAVE_REAL_TTF" != 1 ]; then
  echo "warning: FreeType+SDL_ttf sources incomplete — using stub (fonts will fail)" >&2
  if [ -n "$TTF_SRC" ] && [ -f "$TTF_SRC/SDL_ttf.h" ]; then
    cp -a "$TTF_SRC/SDL_ttf.h" src/jni/src/
    cp -a "$TTF_SRC/SDL_ttf.h" src/jni/ 2>/dev/null || true
  fi
  cat > src/jni/src/sdl_ttf_stub.c <<'STUB'
#include "SDL_ttf.h"
#include <stdio.h>
int TTF_Init(void) { return 0; }
void TTF_Quit(void) {}
const SDL_version *TTF_Linked_Version(void) {
  static SDL_version v = {2, 0, 0};
  return &v;
}
TTF_Font *TTF_OpenFont(const char *file, int ptsize) { (void)file; (void)ptsize; return NULL; }
TTF_Font *TTF_OpenFontIndex(const char *file, int ptsize, long index) {
  (void)file; (void)ptsize; (void)index; return NULL;
}
TTF_Font *TTF_OpenFontRW(SDL_RWops *src, int freesrc, int ptsize) {
  if (src && freesrc) SDL_RWclose(src);
  (void)ptsize; return NULL;
}
void TTF_CloseFont(TTF_Font *font) { (void)font; }
int TTF_GetFontStyle(const TTF_Font *font) { (void)font; return 0; }
void TTF_SetFontStyle(TTF_Font *font, int style) { (void)font; (void)style; }
int TTF_FontHeight(const TTF_Font *font) { (void)font; return 0; }
int TTF_FontAscent(const TTF_Font *font) { (void)font; return 0; }
int TTF_FontDescent(const TTF_Font *font) { (void)font; return 0; }
int TTF_FontLineSkip(const TTF_Font *font) { (void)font; return 0; }
int TTF_SizeUTF8(TTF_Font *font, const char *text, int *w, int *h) {
  (void)font; (void)text; if (w) *w = 0; if (h) *h = 0; return 0;
}
SDL_Surface *TTF_RenderUTF8_Blended(TTF_Font *font, const char *text, SDL_Color fg) {
  (void)font; (void)text; (void)fg; return NULL;
}
SDL_Surface *TTF_RenderUTF8_Solid(TTF_Font *font, const char *text, SDL_Color fg) {
  (void)font; (void)text; (void)fg; return NULL;
}
const char *TTF_GetError(void) { return "SDL_ttf stub (FreeType not linked)"; }
STUB
  echo "==> wrote sdl_ttf_stub.c"
fi

# Stage monorepo external/ headers + sources.
# Under Nix, GAME_SRC_DIR is a filtered ./src store path — parent is NOT the
# repo. Pass GAME_EXTERNAL_DIR (flake: ./external) and optional GLM_INCLUDE_DIR.
EXTERNAL_DIR="${GAME_EXTERNAL_DIR:-}"
if [ -z "$EXTERNAL_DIR" ]; then
  REPO_ROOT="$(cd "$GAME_SRC_DIR/.." && pwd)"
  if [ -d "$REPO_ROOT/external" ]; then
    EXTERNAL_DIR="$REPO_ROOT/external"
  fi
fi
if [ -z "$EXTERNAL_DIR" ] || [ ! -d "$EXTERNAL_DIR" ]; then
  echo "error: GAME_EXTERNAL_DIR must point at the repo external/ tree" >&2
  echo "       (contains geomcpp/, priocpp/, sexpcpp/, logmich/, …)" >&2
  exit 1
fi

mkdir -p src/jni/external_includes
# Nix store trees are often 0555/0444. cp -a preserves that and the next
# package cannot create e.g. external_includes/geom → Permission denied.
# Copy then force owner-writable on the staging tree.
# Header-only / public includes (layout: include/<ns>/… → external_includes/<ns>/…)
for name in argpp geomcpp logmich priocpp strutcpp sexpcpp tinygettext; do
  inc="$EXTERNAL_DIR/$name/include"
  if [ -d "$inc" ]; then
    cp -a "$inc"/. src/jni/external_includes/
    chmod -R u+rwX src/jni/external_includes
  else
    echo "error: missing $inc" >&2
    exit 1
  fi
done
# glm is header-only (geom depends on it).
if [ -n "${GLM_INCLUDE_DIR:-}" ] && [ -d "$GLM_INCLUDE_DIR" ]; then
  # Expect GLM_INCLUDE_DIR to contain glm/… (nixpkgs glm) or be the glm/ dir itself.
  if [ -d "$GLM_INCLUDE_DIR/glm" ]; then
    cp -a "$GLM_INCLUDE_DIR/glm" src/jni/external_includes/
  elif [ "$(basename "$GLM_INCLUDE_DIR")" = "glm" ]; then
    cp -a "$GLM_INCLUDE_DIR" src/jni/external_includes/
  else
    echo "error: GLM_INCLUDE_DIR=$GLM_INCLUDE_DIR does not look like glm headers" >&2
    exit 1
  fi
  chmod -R u+rwX src/jni/external_includes
  echo "==> staged glm headers from $GLM_INCLUDE_DIR"
else
  echo "error: GLM_INCLUDE_DIR is required for Android (geom → glm)" >&2
  exit 1
fi
echo "==> staged external headers into jni/external_includes"
# tinygettext ships a Windows dirent.h; if it sits on -I, <dirent.h> resolves
# to it even on Android. Always drop the Windows shim from the stage tree.
rm -f src/jni/external_includes/tinygettext/dirent.h

# Compile external .cpp into libmain (ndk-build RWILDCARD under jni/src/).
# Skip tests/benchmarks; skip priocpp JSON (no jsoncpp on Android).
mkdir -p src/jni/src/deps
stage_lib_src() {
  local name="$1"
  local srcdir="$EXTERNAL_DIR/$name/src"
  if [ ! -d "$srcdir" ]; then
    echo "warning: no sources for $name ($srcdir)" >&2
    return 0
  fi
  mkdir -p "src/jni/src/deps/$name"
  # top-level sources + private headers (float.hpp, prettyprinter.hpp, …)
  find "$srcdir" -maxdepth 1 -name '*.cpp' -exec cp -a {} "src/jni/src/deps/$name/" \;
  find "$srcdir" -maxdepth 1 \( -name '*.hpp' -o -name '*.h' \) -exec cp -a {} "src/jni/src/deps/$name/" \;
  chmod -R u+rwX "src/jni/src/deps/$name"
}
stage_lib_src argpp
stage_lib_src logmich
stage_lib_src sexpcpp
stage_lib_src strutcpp
stage_lib_src priocpp
stage_lib_src tinygettext

# --- Upstream sources that are packaging-only under external/ ---
# Squirrel (scripting): flake input squirrel-src or SQUIRREL_SOURCE_DIR
# Nix store trees are 0555/0444 — chmod after every cp -a before writing more.
SQUIRREL_SRC="${SQUIRREL_SOURCE_DIR:-}"
if [ -n "$SQUIRREL_SRC" ] && [ -d "$SQUIRREL_SRC" ]; then
  mkdir -p src/jni/src/deps/squirrel
  for sub in squirrel sqstdlib include; do
    if [ -d "$SQUIRREL_SRC/$sub" ]; then
      cp -a "$SQUIRREL_SRC/$sub" src/jni/src/deps/squirrel/
    fi
  done
  chmod -R u+rwX src/jni/src/deps/squirrel
  mkdir -p src/jni/src/deps/squirrel/include
  for h in squirrel.h sqconfig.h sqstdblob.h sqstdio.h sqstdmath.h sqstdstring.h sqstdsystem.h sqstdaux.h; do
    if [ -f "$SQUIRREL_SRC/$h" ]; then
      cp -a "$SQUIRREL_SRC/$h" src/jni/src/deps/squirrel/include/
    elif [ -f "$SQUIRREL_SRC/include/$h" ]; then
      cp -a "$SQUIRREL_SRC/include/$h" src/jni/src/deps/squirrel/include/
    fi
  done
  chmod -R u+rwX src/jni/src/deps/squirrel
  echo "==> staged squirrel from $SQUIRREL_SRC"
else
  echo "warning: SQUIRREL_SOURCE_DIR not set — scripting will fail to link" >&2
fi

# PhysFS
PHYSFS_SRC="${PHYSFS_SOURCE_DIR:-}"
if [ -n "$PHYSFS_SRC" ] && [ -d "$PHYSFS_SRC" ]; then
  mkdir -p src/jni/src/deps/physfs
  if [ -d "$PHYSFS_SRC/src" ]; then
    cp -a "$PHYSFS_SRC/src" src/jni/src/deps/physfs/
  else
    find "$PHYSFS_SRC" -maxdepth 1 -name '*.c' -exec cp -a {} src/jni/src/deps/physfs/ \;
  fi
  chmod -R u+rwX src/jni/src/deps/physfs
  # public header into writable external_includes (already chmod'd earlier)
  if [ -f "$PHYSFS_SRC/src/physfs.h" ]; then
    mkdir -p src/jni/external_includes
    cp -a "$PHYSFS_SRC/src/physfs.h" src/jni/external_includes/
  elif [ -f "$PHYSFS_SRC/physfs.h" ]; then
    mkdir -p src/jni/external_includes
    cp -a "$PHYSFS_SRC/physfs.h" src/jni/external_includes/
  fi
  chmod -R u+rwX src/jni/external_includes 2>/dev/null || true
  chmod -R u+rwX src/jni/src/deps/physfs
  echo "==> staged physfs from $PHYSFS_SRC"
else
  echo "warning: PHYSFS_SOURCE_DIR not set — PhysFS will fail to link" >&2
fi

# obstack (C helper used by SuperTux)
if [ -d "$EXTERNAL_DIR/obstack" ]; then
  mkdir -p src/jni/src/deps/obstack
  find "$EXTERNAL_DIR/obstack" -name '*.c' -o -name '*.h' | while read -r f; do
    cp -a "$f" src/jni/src/deps/obstack/
  done
  chmod -R u+rwX src/jni/src/deps/obstack
  echo "==> staged obstack"
fi

# SavePNG stub (no libpng on NDK path for now)
mkdir -p src/jni/src/deps/SDL_SavePNG
cat > src/jni/src/deps/SDL_SavePNG/savepng_stub.c << 'STUBEOF'
#include <SDL.h>
int SDL_SavePNG_RW(SDL_Surface *surface, SDL_RWops *dst, int freedst) {
  (void)surface;
  if (dst && freedst) SDL_RWclose(dst);
  SDL_SetError("SDL_SavePNG stub: libpng not linked on Android");
  return -1;
}
SDL_Surface *SDL_PNGFormatAlpha(SDL_Surface *src) { (void)src; return NULL; }
STUBEOF
if [ -f "$EXTERNAL_DIR/SDL_SavePNG/savepng.h" ]; then
  cp -a "$EXTERNAL_DIR/SDL_SavePNG/savepng.h" src/jni/src/deps/SDL_SavePNG/
fi
echo "==> staged SavePNG stub"

# Drop JSON backends (PRIO_USE_JSONCPP is off).
rm -f src/jni/src/deps/priocpp/json_*.cpp \
      src/jni/src/deps/priocpp/jsonpretty_*.cpp
# strut layout.cpp needs a missing polygon.hpp; Pingus does not use Layout.
rm -f src/jni/src/deps/strutcpp/layout.cpp
echo "==> staged external sources into jni/src/deps/"

# wstsound (wav + modplug only; match EMSCRIPTEN/ANDROID CMake defaults)
if [ -d "$EXTERNAL_DIR/wstsound/src" ]; then
  mkdir -p src/jni/src/deps/wstsound
  find "$EXTERNAL_DIR/wstsound/src" -maxdepth 1 -name '*.cpp' -exec cp -a {} src/jni/src/deps/wstsound/ \;
  find "$EXTERNAL_DIR/wstsound/src" -maxdepth 1 \( -name '*.hpp' -o -name '*.h' \) -exec cp -a {} src/jni/src/deps/wstsound/ \;
  # Drop codecs / EFX not used on Android.
  # Keep ogg_sound_file.cpp: SuperTux ships .ogg music and Android.mk enables
  # WSTSOUND_WITH_VORBIS + links vorbis when the prebuilts are staged. The
  # ndk-build filter still drops the TU if SUPERTUX_HAVE_VORBIS is unset.
  rm -f src/jni/src/deps/wstsound/opus_sound_file.cpp \
        src/jni/src/deps/wstsound/mp3_sound_file.cpp \
        src/jni/src/deps/wstsound/effect.cpp \
        src/jni/src/deps/wstsound/effect_slot.cpp \
        src/jni/src/deps/wstsound/filter.cpp \
        src/jni/src/deps/wstsound/procedural_sound_file.cpp
  # Public headers: include/wstsound/*.hpp
  if [ -d "$EXTERNAL_DIR/wstsound/include/wstsound" ]; then
    mkdir -p src/jni/external_includes/wstsound
    cp -a "$EXTERNAL_DIR/wstsound/include/wstsound/." src/jni/external_includes/wstsound/
  fi
  chmod -R u+rwX src/jni/src/deps/wstsound src/jni/external_includes/wstsound 2>/dev/null || true
  echo "==> staged wstsound (wav+modplug+ogg) into jni/src/deps/wstsound"
fi

# Prebuilt OpenAL Soft + libmodplug (from nix audioAndroidLibs)
ENABLE_ANDROID_SOUND=0
if [ -n "${AUDIO_ANDROID_LIBS:-}" ] && [ -d "$AUDIO_ANDROID_LIBS" ]; then
  mkdir -p src/jni/audio
  # Layout: jni/audio/<abi>/lib/*.a and shared jni/audio/include/
  # Headers are identical per ABI; copy once and chmod writable (nix store is 0444).
  headers_done=0
  for abi_dir in "$AUDIO_ANDROID_LIBS"/*; do
    [ -d "$abi_dir" ] || continue
    abi=$(basename "$abi_dir")
    case "$abi" in
      armeabi-v7a|arm64-v8a|x86|x86_64)
        mkdir -p "src/jni/audio/$abi/lib"
        cp -a "$abi_dir"/lib/*.a "src/jni/audio/$abi/lib/" 2>/dev/null || true
        chmod -R u+w "src/jni/audio/$abi/lib" 2>/dev/null || true
        if [ "$headers_done" -eq 0 ] && [ -d "$abi_dir/include" ]; then
          mkdir -p src/jni/audio/include
          cp -a "$abi_dir/include/." src/jni/audio/include/
          chmod -R u+rwX src/jni/audio/include
          headers_done=1
        fi
        ;;
    esac
  done
  if ls src/jni/audio/*/lib/libopenal.a >/dev/null 2>&1 &&      ls src/jni/audio/*/lib/libmodplug.a >/dev/null 2>&1; then
    ENABLE_ANDROID_SOUND=1
    echo "==> staged OpenAL Soft + libmodplug prebuilts (sound enabled)"
  else
    echo "warning: AUDIO_ANDROID_LIBS set but openal/modplug .a missing — sound disabled" >&2
    find src/jni/audio -type f 2>/dev/null | head -20 >&2 || true
  fi
else
  echo "==> AUDIO_ANDROID_LIBS not set — building with SUPERTUX_NO_SOUND"
fi
export ENABLE_ANDROID_SOUND

# SuperTux does not use libsigc++ (Pingus polyfill omitted).

# Optional IMG_* / stb shim (Pingus-style). SuperTux links SDL2_image when
# prebuilt; only stage if the files exist under APP_DIR/jni.
if [ -f "$APP_DIR/jni/img_stb_min.c" ]; then
  cp "$APP_DIR/jni/img_stb_min.c" src/jni/src/img_stb_min.c
  echo "==> staged img_stb_min.c"
fi
if [ -f "$APP_DIR/jni/android_SDL_image.h" ]; then
  cp "$APP_DIR/jni/android_SDL_image.h" src/jni/src/SDL_image.h
fi
if [ -n "${STB_IMAGE_H:-}" ] && [ -f "$STB_IMAGE_H" ]; then
  cp "$STB_IMAGE_H" src/jni/src/stb_image.h
elif [ -f "$APP_DIR/jni/stb_image.h" ]; then
  cp "$APP_DIR/jni/stb_image.h" src/jni/src/stb_image.h
else
  echo "==> no stb_image.h staged (SDL2_image prebuilt expected)"
fi

cp "$SDL_PREBUILT_MK" src/jni/SDL/Android.mk
cp -r "$SDL_ANDROID_LIBS/include" src/jni/SDL/include
# SDL2_mixer is optional for SuperTux (sound is OpenAL + wstsound/modplug).
if [ -f src/jni/SDL/include/SDL_mixer.h ] || [ -f src/jni/SDL/include/SDL2/SDL_mixer.h ]; then
  echo "SDL2_mixer headers present"
else
  echo "==> no SDL_mixer.h (expected when sdlMixerSrc is null; OpenAL path used)"
fi

# Game data → APK assets/ (AssetManager root).
mkdir -p src/assets
cp -a "$GAME_DATA_DIR"/. src/assets/
# Nix store files are often 0444; aapt/zip need readable tree we can scan.
chmod -R u+rwX src

ASSET_COUNT=$(find src/assets -type f | wc -l)
ASSET_SIZE=$(du -sh src/assets | awk '{print $1}')
echo "Packaging $ASSET_COUNT asset files ($ASSET_SIZE) from $GAME_DATA_DIR"

# Flat list of asset paths so native code can "opendir" without AAssetManager_list.
# Paths are relative to AssetManager root (same as SDL_RWFromFile).
( cd src/assets && find . -type f ! -name 'android-asset-index.txt' | sed 's|^\./||' | sort > android-asset-index.txt )
INDEX_COUNT=$(wc -l < src/assets/android-asset-index.txt | tr -d ' ')
echo "Wrote android-asset-index.txt ($INDEX_COUNT paths)"

if [ "$ASSET_COUNT" -lt 10 ]; then
  echo "error: asset tree looks empty (found $ASSET_COUNT files)" >&2
  ls -la src/assets >&2 || true
  exit 1
fi
# Probe well-known SuperTux data paths (not SuperTux Milestone 1 leftovers).
for probe in fonts levels music sounds images
do
  if [ ! -e "src/assets/$probe" ]; then
    echo "error: missing src/assets/$probe" >&2
    echo "       is GAME_DATA_DIR a full SuperTux data/ tree?" >&2
    exit 1
  fi
done

# Nested zip so PhysFS can mount assets/data.zip as the data root (paths
# images/, levels/, … without an assets/ prefix). See PHYSFS_AndroidInit
# handling in PhysfsSubsystem::find_datadir().
echo "==> packing assets/data.zip for PhysFS"
rm -f src/assets/data.zip
( cd src/assets && zip -r -0 data.zip . -x 'data.zip' -x 'android-asset-index.txt' )
echo "assets/data.zip size: $(du -h src/assets/data.zip | awk '{print $1}')"
# Avoid double-shipping: APK keeps assets/data.zip (+ index) only. PhysFS mounts
# the zip as the data root; loose duplicates only bloated the package.
echo "==> pruning loose assets (keep data.zip + index only)"
find src/assets -mindepth 1 -maxdepth 1 ! -name 'data.zip' ! -name 'android-asset-index.txt' -exec rm -rf {} +

cp "$KEYSTORE" debug.keystore

# Bake VERSION+g<rev> into the game version macro (see jni/Android.mk).
# nix/android.mkApk exports SUPERTUX_VERSION; accept PINGUS_VERSION as alias.
SUPERTUX_VERSION="${SUPERTUX_VERSION:-${PINGUS_VERSION:-0.6.3-dev}}"
PINGUS_VERSION="$SUPERTUX_VERSION"
echo "==> SUPERTUX_VERSION=$SUPERTUX_VERSION"
# Count only — never pipe find into head under set -o pipefail (SIGPIPE aborts).
echo "==> staged C/C++ sources: $(find src/jni/src \( -name '*.cpp' -o -name '*.c' \) | wc -l)"

"$NDK/ndk-build" \
  NDK_PROJECT_PATH="$PWD/src" \
  APP_BUILD_SCRIPT="$PWD/src/jni/Android.mk" \
  
# Diagnostics: module LOCAL_PATH is jni/src — TTF/FreeType must live there.
echo "==> TTF/FreeType stage check (jni/src):"
ls -la src/jni/src/SDL_ttf.c src/jni/src/sdl_ttf_stub.c src/jni/src/freetype/include/ft2build.h \
  src/jni/src/freetype_Android.mk 2>&1 | sed 's/^/  /' || true

NDK_APPLICATION_MK="$PWD/src/jni/Application.mk" \
  SUPERTUX_VERSION="$SUPERTUX_VERSION" \
  PINGUS_VERSION="$SUPERTUX_VERSION" \
  ENABLE_ANDROID_SOUND="${ENABLE_ANDROID_SOUND:-0}" \
  -j"${NIX_BUILD_CORES:-${JOBS:-$(nproc)}}"

mkdir -p out

# Package resources + manifest. Assets are added via zip below: old aapt's
# -A path has been unreliable with large trees in this pipeline.
"$BT/aapt" package -f \
  -M src/AndroidManifest.xml \
  -S src/res \
  -I "$PACKAGE_JAR" \
  -F out/base.apk

cp "$SDL_ANDROID_LIBS/dex/classes.dex" out/classes.dex
for abi in $TARGET_ABIS; do
  mkdir -p out/lib/"$abi"
  cp src/libs/"$abi"/*.so out/lib/"$abi"/
done

( cd out && "$BT/aapt" add base.apk classes.dex )
( cd out && zip -r base.apk lib )

# Inject assets/ into the APK (same layout AssetManager expects).
# Run from src/ so paths inside the zip are assets/...
( cd src && zip -r -9 ../out/base.apk assets )
echo "APK contents (assets sample):"
#unzip -l out/base.apk | grep -E 'assets/(images|levels)/' | head -20
#ASSET_IN_APK=$(unzip -l out/base.apk | grep -c ' assets/' || true)
#echo "Asset entries in APK: $ASSET_IN_APK"
#if [ "${ASSET_IN_APK:-0}" -lt 10 ]; then
#  echo "error: APK still has almost no assets after zip inject" >&2
#  exit 1
#fi

"$BT/zipalign" -f 4 out/base.apk out/aligned.apk

"$BT/apksigner" sign \
  --ks debug.keystore --ks-pass pass:android --key-pass pass:android \
  --out "out/$APP_NAME.apk" out/aligned.apk

echo "Final APK size: $(du -h "out/$APP_NAME.apk" | awk '{print $1}')"
"$BT/aapt" dump badging "out/$APP_NAME.apk"
