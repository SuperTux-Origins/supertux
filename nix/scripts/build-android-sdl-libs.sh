#!/usr/bin/env bash
# Builds SDL2's native libraries (via ndk-build) and Java glue (via
# javac/d8) for Android. Expects these environment variables set by the
# calling derivation:
#   ANDROID_HOME, BUILD_TOOLS_VERSION, COMPILE_PLATFORM, PACKAGE_PLATFORM
#   SDL_SRC            - path to SDL2's extracted source tree
#   APPLICATION_MK     - path to a generated Application.mk
#   TOP_ANDROID_MK      - path to a generated top-level Android.mk
set -euo pipefail

NDK="$ANDROID_HOME/ndk-bundle"
BT="$ANDROID_HOME/build-tools/$BUILD_TOOLS_VERSION"
COMPILE_JAR="$ANDROID_HOME/platforms/android-$COMPILE_PLATFORM/android.jar"

mkdir -p sdl-jni
cp -r "$SDL_SRC" sdl-jni/SDL
chmod -R u+w sdl-jni
cp "$APPLICATION_MK" sdl-jni/Application.mk
cp "$TOP_ANDROID_MK" sdl-jni/Android.mk

# Builds just SDL2 itself (module "SDL2") — no app module present, so
# there's nothing here for an app's own main.cpp to interfere with or
# trigger a rebuild of.
"$NDK/ndk-build" \
  NDK_PROJECT_PATH="$PWD/sdl-jni" \
  APP_BUILD_SCRIPT="$PWD/sdl-jni/Android.mk" \
  NDK_APPLICATION_MK="$PWD/sdl-jni/Application.mk" \
  -j"$NIX_BUILD_CORES"

mkdir -p javasrc
cp -r sdl-jni/SDL/android-project/app/src/main/java/org javasrc/org
chmod -R u+w javasrc

mkdir -p classes
javac -encoding UTF-8 --release 8 -classpath "$COMPILE_JAR" -d classes \
  $(find javasrc -name '*.java')
"$BT/d8" --output classes --min-api "$PACKAGE_PLATFORM" $(find classes -name '*.class')
