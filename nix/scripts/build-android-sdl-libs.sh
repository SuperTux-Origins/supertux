#!/usr/bin/env bash
# Builds SDL2 (and optionally SDL2_mixer) native libraries via ndk-build,
# plus SDLActivity Java glue (javac/d8).
#
# Env from nix/android.nix:
#   ANDROID_HOME, BUILD_TOOLS_VERSION, COMPILE_PLATFORM, PACKAGE_PLATFORM
#   SDL_SRC            - SDL2 source tree
#   SDL_MIXER_SRC      - optional SDL2_mixer source tree
#   APPLICATION_MK, TOP_ANDROID_MK
set -euo pipefail

NDK="$ANDROID_HOME/ndk-bundle"
BT="$ANDROID_HOME/build-tools/$BUILD_TOOLS_VERSION"
COMPILE_JAR="$ANDROID_HOME/platforms/android-$COMPILE_PLATFORM/android.jar"

mkdir -p sdl-jni
cp -r "$SDL_SRC" sdl-jni/SDL
chmod -R u+w sdl-jni
cp "$APPLICATION_MK" sdl-jni/Application.mk
cp "$TOP_ANDROID_MK" sdl-jni/Android.mk

# Builds just SDL2 itself (module "SDL2").
"$NDK/ndk-build" \
  NDK_PROJECT_PATH="$PWD/sdl-jni" \
  APP_BUILD_SCRIPT="$PWD/sdl-jni/Android.mk" \
  NDK_APPLICATION_MK="$PWD/sdl-jni/Application.mk" \
  -j"$NIX_BUILD_CORES"

# Optional: SDL2_mixer linked against the SDL2 we just built.
# Disable heavy optional codecs that pull missing external modules on Android.
if [ -n "${SDL_MIXER_SRC:-}" ] && [ -d "$SDL_MIXER_SRC" ]; then
  echo "Building SDL2_mixer from $SDL_MIXER_SRC"
  mkdir -p mixer-jni/SDL mixer-jni/SDL2_mixer
  cp "$APPLICATION_MK" mixer-jni/Application.mk
  cp "$TOP_ANDROID_MK" mixer-jni/Android.mk

  # Prebuilt SDL2 for the mixer link step.
  cat > mixer-jni/SDL/Android.mk <<EOF
LOCAL_PATH := \$(call my-dir)
include \$(CLEAR_VARS)
LOCAL_MODULE := SDL2
LOCAL_SRC_FILES := $PWD/sdl-jni/libs/\$(TARGET_ARCH_ABI)/libSDL2.so
LOCAL_EXPORT_C_INCLUDES := $PWD/sdl-jni/SDL/include
include \$(PREBUILT_SHARED_LIBRARY)
EOF

  cp -a "$SDL_MIXER_SRC"/. mixer-jni/SDL2_mixer/
  chmod -R u+w mixer-jni

  # Prefer in-tree stb_vorbis OGG; avoid wavpack/gme/xmp (need extra deps).
  "$NDK/ndk-build" \
    NDK_PROJECT_PATH="$PWD/mixer-jni" \
    APP_BUILD_SCRIPT="$PWD/mixer-jni/Android.mk" \
    NDK_APPLICATION_MK="$PWD/mixer-jni/Application.mk" \
    SUPPORT_WAVPACK=false \
    SUPPORT_GME=false \
    SUPPORT_MOD_XMP=false \
    SUPPORT_OGG_STB=true \
    SUPPORT_OGG=false \
    -j"$NIX_BUILD_CORES"

  for abi_dir in mixer-jni/libs/*; do
    [ -d "$abi_dir" ] || continue
    abi=$(basename "$abi_dir")
    mkdir -p "sdl-jni/libs/$abi"
    cp -v "$abi_dir"/libSDL2_mixer.so "sdl-jni/libs/$abi/"
  done
  mkdir -p sdl-jni/SDL2_mixer_include
  if [ -d mixer-jni/SDL2_mixer/include ]; then
    cp -a mixer-jni/SDL2_mixer/include/. sdl-jni/SDL2_mixer_include/
  fi
fi

mkdir -p javasrc
cp -r sdl-jni/SDL/android-project/app/src/main/java/org javasrc/org
chmod -R u+w javasrc

mkdir -p classes
javac -encoding UTF-8 --release 8 -classpath "$COMPILE_JAR" -d classes \
  $(find javasrc -name '*.java')
"$BT/d8" --output classes --min-api "$PACKAGE_PLATFORM" $(find classes -name '*.class')
