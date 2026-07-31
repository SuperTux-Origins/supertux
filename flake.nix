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
  };

  outputs = { self, nixpkgs, tinycmmc, SDL-win32, SDL_mixer-win32, SDL_image-win32
            , sdl2-src, sdl2-image-src }:
    tinycmmc.lib.eachSystemWithPkgs (pkgs:
      let
        lib = nixpkgs.lib;
        versionBase = lib.strings.removeSuffix "\n" (builtins.readFile ./VERSION);
        gitRev = "${self.shortRev or self.dirtyShortRev or "dirty"}";
        version = "${versionBase}+g${gitRev}";

        # Android SDK is unfree + needs license accept; use a dedicated pkgs.
        androidPkgs = import nixpkgs {
          system = pkgs.system;
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
          inherit androidSdk buildToolsVersion packagePlatform compilePlatform targetAbis;
        };
        gitDate =
          if self ? lastModifiedDate then builtins.substring 0 8 self.lastModifiedDate
          else "00000000";
        androidApkName = "supertux-milestone1-${gitDate}-${gitRev}.apk";
        # Upstream stb_image.h (plain). Fetched at eval/build time; hash pinned.
        stbImageH = androidPkgs.fetchurl {
          url = "https://raw.githubusercontent.com/nothings/stb/refs/heads/master/stb_image.h";
          sha256 = "sha256-WUwv411JSItDgtv67I+YNm3vyoGdkWrJW+zz519CALM=";
        };

        mkSuperTux = { useSDL2 ? true, pname ? "supertux-milestone1" }:
          pkgs.stdenv.mkDerivation rec {
            inherit pname;
            inherit version;
            src = nixpkgs.lib.cleanSource ./.;
            enableParallelBuilding = true;

            # Keep debug symbols (and optionally split them to $debug).
            dontStrip = true;
            separateDebugInfo = true;

            nativeBuildInputs = [
              pkgs.buildPackages.cmake
              pkgs.buildPackages.pkg-config
            ] ++ nixpkgs.lib.optionals pkgs.stdenv.hostPlatform.isLinux [
              pkgs.addDriverRunpath
            ];

            cmakeFlags = [
              "-DCMAKE_BUILD_TYPE=RelWithDebInfo"
              "-DENABLE_SOUND=ON"
              "-DENABLE_OPENGL=ON"
              "-DENABLE_SDL2=${if useSDL2 then "ON" else "OFF"}"
              "-DDATA_PREFIX=${placeholder "out"}/share/supertux-milestone1"
              "-DPROJECT_VERSION_FULL=${version}"
            ];

            buildInputs =
              (if pkgs.stdenv.hostPlatform.isWindows && !useSDL2 then [
                SDL-win32.packages.${pkgs.system}.default
                SDL_image-win32.packages.${pkgs.system}.default
                SDL_mixer-win32.packages.${pkgs.system}.default
              ] else if useSDL2 then [
                pkgs.SDL2
                pkgs.SDL2_image
                pkgs.SDL2_mixer
                pkgs.libGL
              ] else [
                pkgs.SDL
                pkgs.SDL_image
                pkgs.SDL_mixer
                pkgs.libGL
                pkgs.libGLU
              ]) ++ [
                pkgs.zlib
                pkgs.libpng
                pkgs.libjpeg
                pkgs.libtiff
              ];

            postFixup = nixpkgs.lib.optionalString pkgs.stdenv.hostPlatform.isLinux ''
              addDriverRunpath $out/bin/supertux-milestone1
            '' + nixpkgs.lib.optionalString pkgs.stdenv.hostPlatform.isWindows ''
              mkdir -p $out/bin/
              find ${pkgs.windows.mcfgthreads} -iname "*.dll" -exec ln -sfv {} $out/bin/ \;
              find ${pkgs.stdenv.cc.cc} -iname "*.dll" -exec ln -sfv {} $out/bin/ \;
              ${if useSDL2 then "" else ''
              ln -sfv ${SDL-win32.packages.${pkgs.system}.default}/bin/*.dll $out/bin/
              ln -sfv ${SDL_image-win32.packages.${pkgs.system}.default}/bin/*.dll $out/bin/
              ln -sfv ${SDL_mixer-win32.packages.${pkgs.system}.default}/bin/*.dll $out/bin/
              ''}
            '';

            meta = with nixpkgs.lib; {
              description = "SuperTux Milestone 1 (${if useSDL2 then "SDL2" else "SDL 1.2"})";
              license = licenses.gpl2Plus;
              platforms = platforms.linux ++ platforms.windows;
              mainProgram = "supertux-milestone1";
            };
          };

        pkgSdl2 = mkSuperTux { useSDL2 = true;  pname = "supertux-milestone1-sdl2"; };
        pkgSdl1 = mkSuperTux { useSDL2 = false; pname = "supertux-milestone1-sdl1"; };
      in {
        packages = rec {
          default = supertux-milestone1-sdl2;
          supertux-milestone1 = supertux-milestone1-sdl2;
          supertux-milestone1-sdl2 = pkgSdl2;
          supertux-milestone1-sdl1 = pkgSdl1;

          supertux-milestone1-win32 = pkgs.runCommand "supertux-milestone1-win32" {} ''
            mkdir -p $out/data
            cp -vr ${pkgSdl1}/bin/supertux-milestone1.exe $out/ 2>/dev/null \
              || cp -vr ${pkgSdl1}/bin/supertux-milestone1 $out/
            cp -vLr ${pkgSdl1}/bin/*.dll $out/ 2>/dev/null || true
            if [ -d ${pkgSdl1}/share/supertux-milestone1 ]; then
              cp -vr ${pkgSdl1}/share/supertux-milestone1/. $out/data/
            fi
          '';

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
          };
        };

        apps = {
          default = {
            type = "app";
            program = "${pkgSdl2}/bin/supertux-milestone1";
            meta.description = "SuperTux Milestone 1 (SDL2)";
          };
          supertux-milestone1 = {
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
          install-android-supertux-milestone1 = android.mkInstallApp {
            pkg = android.mkApk {
              appName = "supertux-milestone1";
              appDir = ./android;
              outApkName = androidApkName;
              keystore = ./keystore/debug.keystore;
              gameSrcDir = ./src;
              gameDataDir = ./data;
              stbImageH = stbImageH;
            };
            apkFileName = androidApkName;
            description = "Install SuperTux Milestone 1 APK via adb";
          };
        };

        devShells = {
          default = pkgs.mkShell {
            packages = [
              pkgs.cmake pkgs.pkg-config
              pkgs.SDL2 pkgs.SDL2_image pkgs.SDL2_mixer
              pkgs.libGL pkgs.zlib pkgs.libpng pkgs.libjpeg
            ];
          };
          sdl2 = pkgs.mkShell {
            packages = [
              pkgs.cmake pkgs.pkg-config
              pkgs.SDL2 pkgs.SDL2_image pkgs.SDL2_mixer
              pkgs.libGL pkgs.zlib pkgs.libpng pkgs.libjpeg
            ];
          };
          sdl1 = pkgs.mkShell {
            packages = [
              pkgs.cmake pkgs.pkg-config
              pkgs.SDL pkgs.SDL_image pkgs.SDL_mixer
              pkgs.libGL pkgs.zlib pkgs.libpng pkgs.libjpeg
            ];
          };
        };
      }
    );
}
