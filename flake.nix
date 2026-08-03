# SuperTux - Milestone 1
# Copyright (C) 2022 Ingo Ruhnke <grumbel@gmail.com>
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
# LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
# OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
# WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

{
  description = "A 2D platform game featuring Tux the penguin (Milestone 1)";

  inputs = rec {
    nixpkgs.url = "github:NixOS/nixpkgs?ref=nixos-26.05";

    tinycmmc.url = "github:grumbel/tinycmmc";
    tinycmmc.inputs.nixpkgs.follows = "nixpkgs";

    SDL-win32.url = "github:grumnix/SDL-win32";
    SDL-win32.inputs.nixpkgs.follows = "nixpkgs";
    SDL-win32.inputs.tinycmmc.follows = "tinycmmc";

    SDL_mixer-win32.url = "github:grumnix/SDL_mixer-win32";
    SDL_mixer-win32.inputs.nixpkgs.follows = "nixpkgs";
    SDL_mixer-win32.inputs.tinycmmc.follows = "tinycmmc";
    SDL_mixer-win32.inputs.SDL-win32.follows = "SDL-win32";

    SDL_image-win32.url = "github:grumnix/SDL_image-win32";
    SDL_image-win32.inputs.nixpkgs.follows = "nixpkgs";
    SDL_image-win32.inputs.tinycmmc.follows = "tinycmmc";
    SDL_image-win32.inputs.SDL-win32.follows = "SDL-win32";

    # SDL2 source for Android ndk-build (same pattern as helloworld-fireos).
    sdl2-src = {
      url = "https://github.com/libsdl-org/SDL/releases/download/release-2.30.3/SDL2-2.30.3.tar.gz";
      flake = false;
    };
    sdl2-image-src = {
      url = "https://github.com/libsdl-org/SDL_image/releases/download/release-2.8.2/SDL2_image-2.8.2.tar.gz";
      flake = false;
    };
    # SDL2_mixer for Android audio (OGG via stb_vorbis in-tree; no system libogg).
    sdl2-mixer-src = {
      url = "https://github.com/libsdl-org/SDL_mixer/releases/download/release-2.8.0/SDL2_mixer-2.8.0.tar.gz";
      flake = false;
    };
    # libxmp for MOD/XM music (salcon.mod, theme.mod, credits.xm, …).
    libxmp-src = {
      url = "https://github.com/libxmp/libxmp/releases/download/libxmp-4.6.0/libxmp-4.6.0.tar.gz";
      flake = false;
    };
  };

  outputs = { self, nixpkgs, tinycmmc, SDL-win32, SDL_mixer-win32, SDL_image-win32
            , sdl2-src, sdl2-image-src, sdl2-mixer-src, libxmp-src }:
    tinycmmc.lib.eachSystemWithPkgs (pkgs:
      let
        lib = nixpkgs.lib;
        versionBase = lib.strings.removeSuffix "\n" (builtins.readFile ./VERSION);
        gitRev = "${self.shortRev or self.dirtyShortRev or "dirty"}";
        version = "${versionBase}+g${gitRev}";

        # Android SDK is unfree + needs license accept; use a dedicated pkgs.
        androidPkgs = import nixpkgs {
          system = pkgs.stdenv.hostPlatform.system;
          config.allowUnfree = true;
          config.android_sdk.accept_license = true;
        };
        buildToolsVersion = "30.0.3";
        packagePlatform = "22";
        compilePlatform = "33";
        ndkVersion = "23.1.7779620";
        targetAbis = [ "armeabi-v7a" "arm64-v8a" ];
        androidSdk = (androidPkgs.androidenv.composeAndroidPackages {
          platformVersions = [ packagePlatform compilePlatform ];
          buildToolsVersions = [ buildToolsVersion ];
          includeNDK = true;
          inherit ndkVersion;
          includeEmulator = false;
          includeSources = false;
        }).androidsdk;
        android = import ./nix/android.nix {
          pkgs = androidPkgs;
          sdlSrc = sdl2-src;
          sdlVersion = "2.30.3";
          sdlMixerSrc = sdl2-mixer-src;
          sdlMixerVersion = "2.8.0";
          libxmpSrc = libxmp-src;
          inherit androidSdk buildToolsVersion packagePlatform compilePlatform targetAbis;
        };

        # WebAssembly (Emscripten) — same SDL2 / mixer / libxmp inputs as Android.
        # Libs split: wasm-sdl2, wasm-sdl2-image, wasm-sdl2-mixer (+ joined wasm-sdl-libs).
        wasm = import ./nix/wasm.nix {
          inherit pkgs;
          sdlSrc = sdl2-src;
          sdlImageSrc = sdl2-image-src;
          sdlMixerSrc = sdl2-mixer-src;
          libxmpSrc = libxmp-src;
          sdlVersion = "2.30.3";
        };
        # data/ may be missing in thin agent trees; mkApp tolerates null.
        wasmDataDir =
          if builtins.pathExists ./data then ./data else null;

        gitDate =
          if self ? lastModifiedDate then builtins.substring 0 8 self.lastModifiedDate
          else "00000000";
        androidApkName = "supertux-milestone1-${gitDate}-${gitRev}.apk";
        # Upstream stb_image.h (plain). Fetched at eval/build time; hash pinned.
        stbImageH = androidPkgs.fetchurl {
          url = "https://raw.githubusercontent.com/nothings/stb/refs/heads/master/stb_image.h";
          sha256 = "sha256-WUwv411JSItDgtv67I+YNm3vyoGdkWrJW+zz519CALM=";
        };

        # targetPkgs: native pkgs for Linux builds, or pkgs.pkgsCross.mingw32 for Win32.
        # SDL-*-win32 flake packages are keyed by the *build* system (e.g. x86_64-linux),
        # not the Windows hostPlatform string.
        mkSuperTux = {
          useSDL2 ? true,
          useGLES2 ? false,
          pname ? "supertux-milestone1",
          targetPkgs ? pkgs,
        }:
          let
            p = targetPkgs;
            # GLES2 implies SDL2 + OpenGL path (shader renderer, no libGLU).
            effectiveSDL2 = useSDL2 || useGLES2;
            isWin = p.stdenv.hostPlatform.isWindows;
            # Flake inputs that ship MinGW SDL1.2 DLLs + import libs.
            sdlWin = SDL-win32.packages.${pkgs.system}.default;
            sdlImageWin = SDL_image-win32.packages.${pkgs.system}.default;
            sdlMixerWin = SDL_mixer-win32.packages.${pkgs.system}.default;
          in
          p.stdenv.mkDerivation rec {
            inherit pname;
            inherit version;
            src = nixpkgs.lib.cleanSource ./.;
            enableParallelBuilding = true;

            # Keep debug symbols on native Linux; MinGW strip/split is flaky.
            dontStrip = true;
            separateDebugInfo = !isWin;

            nativeBuildInputs = [
              p.buildPackages.cmake
              p.buildPackages.pkg-config
            ] ++ nixpkgs.lib.optionals p.stdenv.hostPlatform.isLinux [
              p.addDriverRunpath
            ];

            cmakeFlags = [
              "-DCMAKE_BUILD_TYPE=RelWithDebInfo"
              "-DENABLE_SOUND=ON"
              "-DENABLE_OPENGL=${if isWin then "OFF" else "ON"}"
              "-DENABLE_GLES2=${if useGLES2 then "ON" else "OFF"}"
              "-DENABLE_SDL2=${if effectiveSDL2 then "ON" else "OFF"}"
              "-DDATA_PREFIX=${placeholder "out"}/share/supertux-milestone1"
              "-DPROJECT_VERSION_FULL=${version}"
            ];

            buildInputs =
              (if isWin && !effectiveSDL2 then [
                sdlWin
                sdlImageWin
                sdlMixerWin
              ] else if effectiveSDL2 then [
                p.SDL2
                p.SDL2_image
                p.SDL2_mixer
              ] ++ (if useGLES2 then [
                p.libGL
                p.libglvnd
              ] else [
                p.libGL
              ]) else [
                p.SDL
                p.SDL_image
                p.SDL_mixer
                p.libGL
                p.libGLU
              ]) ++ [
                p.zlib
              ] ++ nixpkgs.lib.optionals (!isWin) [
                p.libpng
                p.libjpeg
                p.libtiff
              ];

            postFixup = nixpkgs.lib.optionalString p.stdenv.hostPlatform.isLinux ''
              addDriverRunpath $out/bin/supertux-milestone1
            '' + nixpkgs.lib.optionalString isWin ''
              mkdir -p $out/bin/
              # Runtime DLLs next to the .exe (Windows loader search path).
              find ${p.windows.mcfgthreads} -iname "*.dll" -exec ln -sfv {} $out/bin/ \; || true
              find ${p.stdenv.cc.cc} -iname "*.dll" -exec ln -sfv {} $out/bin/ \; || true
              ${if effectiveSDL2 then "" else ''
              ln -sfv ${sdlWin}/bin/*.dll $out/bin/ || true
              ln -sfv ${sdlImageWin}/bin/*.dll $out/bin/ || true
              ln -sfv ${sdlMixerWin}/bin/*.dll $out/bin/ || true
              ''}
            '';

            meta = with nixpkgs.lib; {
              description = "SuperTux Milestone 1 (${
                if isWin then "Win32 SDL1.2 cross"
                else if useGLES2 then "SDL2 + GLES2"
                else if effectiveSDL2 then "SDL2"
                else "SDL 1.2"
              })";
              license = licenses.gpl2Plus;
              platforms = if isWin then platforms.windows else platforms.linux;
              mainProgram = "supertux-milestone1";
            };
          };

        pkgSdl2 = mkSuperTux { useSDL2 = true;  pname = "supertux-milestone1-sdl2"; };
        pkgSdl1 = mkSuperTux { useSDL2 = false; pname = "supertux-milestone1-sdl1"; };
        pkgSdl2Gles2 = mkSuperTux {
          useSDL2 = true;
          useGLES2 = true;
          pname = "supertux-milestone1-sdl2-gles2";
        };
        # Real MinGW cross build (SDL1.2 + grumnix win32 SDL inputs). Not a
        # repack of the native Linux binary.
        pkgWin32 = mkSuperTux {
          useSDL2 = false;
          useGLES2 = false;
          pname = "supertux-milestone1-win32";
          targetPkgs = pkgs.pkgsCross.mingw32;
        };
      in rec {
        packages = {
          default = pkgSdl2;
          supertux-milestone1-sdl2 = pkgSdl2;
          supertux-milestone1-sdl1 = pkgSdl1;
          supertux-milestone1-sdl2-gles2 = pkgSdl2Gles2;

          # MinGW32 cross (SDL 1.2). See pkgWin32 / mkSuperTux targetPkgs.
          supertux-milestone1-win32 = pkgWin32;

          # Android APK (Fire OS 5 / API 22 baseline). Requires Linux + SDK.
          android-sdl-libs = android.sdlAndroidLibs;
          supertux-milestone1-android = android.mkApk {
            appName = "supertux-milestone1";
            appDir = ./android;
            outApkName = androidApkName;
            keystore = ./keystore/debug.keystore;
            gameSrcDir = ./src;
            gameDataDir = ./data;
            stbImageH = stbImageH;
            gameVersion = version;
          };

          # WebAssembly (Emscripten + SDL2 + GLES2/WebGL).
          # Split so SDL2 / image / mixer rebuild independently; joined for the app.
          wasm-sdl2 = wasm.sdl2WasmLibs;
          wasm-sdl2-image = wasm.sdl2Image;
          wasm-sdl2-mixer = wasm.sdl2Mixer;
          wasm-sdl-libs = wasm.sdlWasmLibs;  # symlinkJoin of the above
          wasm-zlib-libs = wasm.zlibWasmLibs;
          supertux-milestone1-wasm = wasm.mkApp {
            appName = "supertux-milestone1";
            srcDir = ./.;
            dataDir = wasmDataDir;
            enableSound = true;
            enableGles2 = true;
            enableAsyncify = false;  # app_loop + st_frame_delay; true if a path freezes
            versionFull = version;
            gitRev = gitRev;
            sourceUrl = "https://github.com/SuperTux-Origins/supertux-milestone1";
          };
        };

        # `nix flake check` builds every derivation listed here.
        checks = {
          inherit (packages)
            supertux-milestone1-sdl1
            supertux-milestone1-sdl2
            supertux-milestone1-sdl2-gles2
            supertux-milestone1-win32
            android-sdl-libs
            supertux-milestone1-android;
        };

        apps = {
          default = {
            type = "app";
            program = "${pkgSdl2}/bin/supertux-milestone1";
            meta.description = "SuperTux Milestone 1 (SDL2)";
          };
          supertux-milestone1-sdl2 = {
            type = "app";
            program = "${pkgSdl2}/bin/supertux-milestone1";
            meta.description = "SuperTux Milestone 1 (SDL2)";
          };
          supertux-milestone1-sdl1 = {
            type = "app";
            program = "${pkgSdl1}/bin/supertux-milestone1";
            meta.description = "SuperTux Milestone 1 (SDL 1.2)";
          };
          supertux-milestone1-sdl2-gles2 = {
            type = "app";
            program = "${pkgSdl2Gles2}/bin/supertux-milestone1";
            meta.description = "SuperTux Milestone 1 (SDL2 + OpenGL ES 2.0)";
          };
          install-android-supertux-milestone1 = android.mkInstallApp {
            pkg = android.mkApk {
              appName = "supertux-milestone1";
              appDir = ./android;
              outApkName = androidApkName;
              keystore = ./keystore/debug.keystore;
              gameSrcDir = ./src;
              gameDataDir = ./data;
              stbImageH = stbImageH;
              gameVersion = version;
            };
            apkFileName = androidApkName;
            description = "Install SuperTux Milestone 1 APK via adb";
          };

          # Serve wasm build over local HTTP (required for .wasm fetch).
          supertux-milestone1-wasm = wasm.mkOpenBrowserApp {
            pkg = packages.supertux-milestone1-wasm;
            appName = "supertux-milestone1";
            description = "Serve SuperTux Milestone 1 (wasm) over HTTP and open browser";
          };
        };

        # Shell names match package attribute names.
        devShells = {
          default = pkgs.mkShell {
            packages = [
              pkgs.cmake pkgs.pkg-config
              pkgs.SDL2 pkgs.SDL2_image pkgs.SDL2_mixer
              pkgs.libGL pkgs.zlib pkgs.libpng pkgs.libjpeg
            ];
          };
          supertux-milestone1-sdl2 = pkgs.mkShell {
            packages = [
              pkgs.cmake pkgs.pkg-config
              pkgs.SDL2 pkgs.SDL2_image pkgs.SDL2_mixer
              pkgs.libGL pkgs.zlib pkgs.libpng pkgs.libjpeg
            ];
          };
          supertux-milestone1-sdl1 = pkgs.mkShell {
            packages = [
              pkgs.cmake pkgs.pkg-config
              pkgs.SDL pkgs.SDL_image pkgs.SDL_mixer
              pkgs.libGL pkgs.zlib pkgs.libpng pkgs.libjpeg
            ];
          };
          supertux-milestone1-sdl2-gles2 = pkgs.mkShell {
            packages = [
              pkgs.cmake pkgs.pkg-config
              pkgs.SDL2 pkgs.SDL2_image pkgs.SDL2_mixer
              pkgs.libGL pkgs.libGLU pkgs.zlib pkgs.libpng pkgs.libjpeg
            ];
          };
        };
      }
    );
}
