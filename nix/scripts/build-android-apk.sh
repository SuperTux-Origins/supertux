#!/usr/bin/env bash
# Builds the SuperTux Milestone 1 APK, linking SDL2 as a prebuilt library.
# Expects environment variables from nix/android.nix:
#   ANDROID_HOME, BUILD_TOOLS_VERSION, PACKAGE_PLATFORM, TARGET_ABIS
#   APP_NAME, APP_DIR          - android/ packaging dir (manifest, res, jni/)
#   GAME_SRC_DIR               - path to C++ sources (repo src/)
#   APPLICATION_MK, TOP_ANDROID_MK, SDL_PREBUILT_MK, SDL_ANDROID_LIBS
#   KEYSTORE
#   STB_IMAGE_H                - upstream stb_image.h
#   GAME_DATA_DIR              - optional data/ tree packaged as assets
set -euo pipefail

NDK="$ANDROID_HOME/ndk-bundle"
BT="$ANDROID_HOME/build-tools/$BUILD_TOOLS_VERSION"
PACKAGE_JAR="$ANDROID_HOME/platforms/android-$PACKAGE_PLATFORM/android.jar"

if [ -z "${GAME_SRC_DIR:-}" ] || [ ! -d "$GAME_SRC_DIR" ]; then
  echo "error: GAME_SRC_DIR must point at the game C++ source tree" >&2
  exit 1
fi

mkdir -p src/jni/src src/jni/SDL
cp "$APPLICATION_MK" src/jni/Application.mk
cp "$TOP_ANDROID_MK" src/jni/Android.mk
cp "$APP_DIR/jni/Android.mk" src/jni/src/Android.mk
cp "$APP_DIR/AndroidManifest.xml" src/AndroidManifest.xml
cp -r "$APP_DIR/res" src/res

# Game C++ sources next to the module Android.mk.
cp -r "$GAME_SRC_DIR"/. src/jni/src/
# Drop the SDL1 backend — Android is SDL2-only.
rm -f src/jni/src/platform_sdl1.cpp

# IMG_* shim + headers (always writable copies in the build dir).
cp "$APP_DIR/jni/img_stb_min.c" src/jni/src/img_stb_min.c
cp "$APP_DIR/jni/SDL_image.h" src/jni/src/SDL_image.h
if [ -n "${STB_IMAGE_H:-}" ] && [ -f "$STB_IMAGE_H" ]; then
  cp "$STB_IMAGE_H" src/jni/src/stb_image.h
elif [ -f "$APP_DIR/jni/stb_image.h" ]; then
  cp "$APP_DIR/jni/stb_image.h" src/jni/src/stb_image.h
else
  echo "error: need STB_IMAGE_H or android/jni/stb_image.h (upstream stb)" >&2
  exit 1
fi

cp "$SDL_PREBUILT_MK" src/jni/SDL/Android.mk
cp -r "$SDL_ANDROID_LIBS/include" src/jni/SDL/include

# Optional game data → APK assets.
if [ -n "${GAME_DATA_DIR:-}" ] && [ -d "$GAME_DATA_DIR" ]; then
  mkdir -p src/assets
  cp -r "$GAME_DATA_DIR"/. src/assets/
fi

# Nix store copies are often 0444; make the tree writable before any edits.
chmod -R u+w src
cp "$KEYSTORE" debug.keystore

"$NDK/ndk-build" \
  NDK_PROJECT_PATH="$PWD/src" \
  APP_BUILD_SCRIPT="$PWD/src/jni/Android.mk" \
  NDK_APPLICATION_MK="$PWD/src/jni/Application.mk" \
  -j"${NIX_BUILD_CORES:-$(nproc)}"

mkdir -p out
AAPT_ASSETS=()
if [ -d src/assets ]; then
  AAPT_ASSETS=(-A src/assets)
fi

"$BT/aapt" package -f \
  -M src/AndroidManifest.xml \
  -S src/res \
  "${AAPT_ASSETS[@]}" \
  -I "$PACKAGE_JAR" \
  -F out/base.apk

cp "$SDL_ANDROID_LIBS/dex/classes.dex" out/classes.dex
for abi in $TARGET_ABIS; do
  mkdir -p out/lib/"$abi"
  cp src/libs/"$abi"/*.so out/lib/"$abi"/
done

( cd out && "$BT/aapt" add base.apk classes.dex )
( cd out && zip -r base.apk lib )

"$BT/zipalign" -f 4 out/base.apk out/aligned.apk

"$BT/apksigner" sign \
  --ks debug.keystore --ks-pass pass:android --key-pass pass:android \
  --out "out/$APP_NAME.apk" out/aligned.apk

"$BT/aapt" dump badging "out/$APP_NAME.apk"
