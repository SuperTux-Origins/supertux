# Reusable Android/ndk-build pipeline for any SDL2 + C++ app. Nothing
# project-specific here beyond what's passed in — another project can
# import this directly with its own app directory, keystore, and
# platform/ABI choices.
#
# Usage:
#   android = import ./nix/android.nix {
#     inherit pkgs;
#     sdlSrc = <path to SDL2's extracted source tree>;   # e.g. a flake input
#     sdlVersion = "2.30.3";                              # just for derivation labeling
#     androidSdk = ...;            # androidenv.composeAndroidPackages { ... }.androidsdk
#     buildToolsVersion = "30.0.3";
#     packagePlatform = "22";      # baked into the APK's manifest/resources
#     compilePlatform = "33";      # javac classpath only, see comment below
#     targetAbis = [ "armeabi-v7a" "arm64-v8a" ];
#   };
#   android.mkApk { appName = "myapp"; appDir = ./apps/myapp; outApkName = "myapp.apk"; keystore = ./keystore/debug.keystore; }
{ pkgs
, sdlSrc
, sdlVersion
, androidSdk
, buildToolsVersion
, packagePlatform
, compilePlatform
, targetAbis
}:

let
  targetAbisStr = pkgs.lib.concatStringsSep " " targetAbis;

  # Generic ndk-build entry point: just recurses into whatever subdirs
  # have their own Android.mk (SDL2's prebuilt one, and the app's own).
  topAndroidMk = pkgs.writeTextFile {
    name = "Android.mk";
    text = "include $(call all-subdir-makefiles)\n";
  };

  applicationMk = pkgs.writeTextFile {
    name = "Application.mk";
    text = ''
      APP_STL := c++_shared
      APP_ABI := ${targetAbisStr}
      APP_PLATFORM := android-${packagePlatform}
    '';
  };

  # ---------------------------------------------------------------
  # SDL2 itself, built exactly once (per Nix store, cached across every
  # app built with this module, and across repeated app rebuilds): the
  # native libSDL2.so per ABI, its headers, and the compiled SDLActivity
  # Java glue (identical for every SDL2-based app — nothing app-specific
  # here). This is the expensive part of the whole pipeline (~150 C
  # source files x N ABIs), so pulling it out of mkApk means editing a
  # single app's main.cpp never triggers a full SDL2 recompile again.
  # ---------------------------------------------------------------
  sdlAndroidLibs = pkgs.stdenvNoCC.mkDerivation {
    pname = "sdl2-android-libs";
    version = sdlVersion;

    dontUnpack = true;
    nativeBuildInputs = [ androidSdk pkgs.jdk17 pkgs.gnumake ];

    env = {
      BUILD_TOOLS_VERSION = buildToolsVersion;
      COMPILE_PLATFORM = compilePlatform;
      PACKAGE_PLATFORM = packagePlatform;
      SDL_SRC = "${sdlSrc}";
      APPLICATION_MK = applicationMk;
      TOP_ANDROID_MK = topAndroidMk;
    };

    buildPhase = ''
      runHook preBuild
      export ANDROID_HOME=${androidSdk}/libexec/android-sdk
      bash ${./scripts/build-android-sdl-libs.sh}
      runHook postBuild
    '';

    installPhase = ''
      runHook preInstall
      TARGET_ABIS=${pkgs.lib.escapeShellArg targetAbisStr} \
        bash ${./scripts/install-android-sdl-libs.sh}
      runHook postInstall
    '';
  };

  # A drop-in replacement for the real SDL2 source dir, as far as an
  # app's own jni/Android.mk is concerned: same "SDL2" module name, same
  # ../SDL/include path — just backed by sdlAndroidLibs' prebuilt .so
  # instead of full source, via ndk-build's PREBUILT_SHARED_LIBRARY. No
  # changes needed in an app's own jni/Android.mk for this.
  sdlPrebuiltAndroidMk = pkgs.writeTextFile {
    name = "SDL2-prebuilt-Android.mk";
    text = ''
      LOCAL_PATH := $(call my-dir)
      include $(CLEAR_VARS)
      LOCAL_MODULE := SDL2
      LOCAL_SRC_FILES := ${sdlAndroidLibs}/lib/$(TARGET_ARCH_ABI)/libSDL2.so
      include $(PREBUILT_SHARED_LIBRARY)
    '';
  };

  # ---------------------------------------------------------------
  # Shared build pipeline for any SDL2-based app directory containing:
  #   AndroidManifest.xml, res/, jni/Android.mk (native module), main.cpp
  # Everything else (SDK/NDK setup, linking the prebuilt SDL2 layer,
  # aapt/zipalign/apksigner, ABI packaging) is identical across apps and
  # lives here once.
  # ---------------------------------------------------------------
  mkApk = {
    appName,
    appDir,
    outApkName,
    keystore,
    # SuperTux (and similar): C++ sources live outside appDir.
    gameSrcDir ? null,
    # Optional SDL2_image source tree (stb backend compiled into libmain).
    sdl2ImageSrc ? null,
    # Optional game data directory packaged as APK assets.
    gameDataDir ? null,
  }:
    pkgs.stdenvNoCC.mkDerivation {
      pname = appName;
      version = "1.0.0";

      dontUnpack = true;
      nativeBuildInputs = [ androidSdk pkgs.jdk17 pkgs.zip pkgs.gnumake ];

      env = {
        BUILD_TOOLS_VERSION = buildToolsVersion;
        PACKAGE_PLATFORM = packagePlatform;
        APP_NAME = appName;
        APP_DIR = "${appDir}";
        APPLICATION_MK = applicationMk;
        TOP_ANDROID_MK = topAndroidMk;
        SDL_PREBUILT_MK = sdlPrebuiltAndroidMk;
        SDL_ANDROID_LIBS = sdlAndroidLibs;
        KEYSTORE = "${keystore}";
      } // pkgs.lib.optionalAttrs (gameSrcDir != null) {
        GAME_SRC_DIR = "${gameSrcDir}";
      } // pkgs.lib.optionalAttrs (sdl2ImageSrc != null) {
        SDL2_IMAGE_SRC = "${sdl2ImageSrc}";
      } // pkgs.lib.optionalAttrs (gameDataDir != null) {
        GAME_DATA_DIR = "${gameDataDir}";
      };

      buildPhase = ''
        runHook preBuild
        export ANDROID_HOME=${androidSdk}/libexec/android-sdk
        TARGET_ABIS=${pkgs.lib.escapeShellArg targetAbisStr} \
          bash ${./scripts/build-android-apk.sh}
        runHook postBuild
      '';

      installPhase = ''
        mkdir -p $out
        cp out/${appName}.apk $out/${outApkName}
      '';
    };

  mkInstallApp = { pkg, apkFileName, description ? "Install ${apkFileName} to a connected Android device via adb" }: {
    type = "app";
    program = toString (pkgs.writeShellScript "adb-install-${apkFileName}" ''
      exec ${pkgs.android-tools}/bin/adb install -r ${pkg}/${apkFileName}
    '');
    meta.description = description;
  };
in {
  inherit sdlAndroidLibs mkApk mkInstallApp;
}
