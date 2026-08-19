# SuperTux
# Copyright (C) 2021 Ingo Ruhnke <grumbel@gmail.com>
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
  description = "A 2D platform game featuring Tux the penguin";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs?ref=nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";

    tinycmmc.url = "github:grumbel/tinycmmc";
    tinycmmc.inputs.nixpkgs.follows = "nixpkgs";
    tinycmmc.inputs.flake-utils.follows = "flake-utils";

    sexpcpp.url = "github:lispparser/sexp-cpp";
    sexpcpp.inputs.nixpkgs.follows = "nixpkgs";
    sexpcpp.inputs.flake-utils.follows = "flake-utils";

    logmich.url = "github:logmich/logmich";
    logmich.inputs.nixpkgs.follows = "nixpkgs";

    curl-win32.url = "github:grumnix/curl-win32";
    curl-win32.inputs.nixpkgs.follows = "nixpkgs";
    curl-win32.inputs.tinycmmc.follows = "tinycmmc";

    physfs-win32.url = "github:grumnix/physfs-win32";
    physfs-win32.inputs.nixpkgs.follows = "nixpkgs";
    physfs-win32.inputs.tinycmmc.follows = "tinycmmc";

    SDL2-win32.url = "github:grumnix/SDL2-win32";
    SDL2-win32.inputs.nixpkgs.follows = "nixpkgs";

    SDL2_image-win32.url = "github:grumnix/SDL2_image-win32";
    SDL2_image-win32.inputs.nixpkgs.follows = "nixpkgs";

    freetype-win32.url = "github:grumnix/freetype-win32";
    freetype-win32.inputs.nixpkgs.follows = "nixpkgs";
    freetype-win32.inputs.tinycmmc.follows = "tinycmmc";

    SDL2_ttf-win32.url = "github:grumnix/SDL2_ttf-win32";
    SDL2_ttf-win32.inputs.nixpkgs.follows = "nixpkgs";
    SDL2_ttf-win32.inputs.tinycmmc.follows = "tinycmmc";

    strutcpp.url = "github:grumbel/strutcpp";
    strutcpp.inputs.nixpkgs.follows = "nixpkgs";

    miniswig.url = "github:WindstilleTeam/miniswig";
    miniswig.inputs.nixpkgs.follows = "nixpkgs";
    miniswig.inputs.tinycmmc.follows = "tinycmmc";

    xdgcpp.url = "github:grumbel/xdgcpp";
    xdgcpp.inputs.nixpkgs.follows = "nixpkgs";
    xdgcpp.inputs.flake-utils.follows = "flake-utils";

    wstsound.url = "github:WindstilleTeam/wstsound";
    wstsound.inputs.nixpkgs.follows = "nixpkgs";
    wstsound.inputs.flake-utils.follows = "flake-utils";
    wstsound.inputs.tinycmmc.follows = "tinycmmc";

    squirrel.url = "github:grumnix/squirrel";
    squirrel.inputs.nixpkgs.follows = "nixpkgs";
    squirrel.inputs.tinycmmc.follows = "tinycmmc";

    glew-win32.url = "github:grumnix/glew-win32";
    glew-win32.inputs.nixpkgs.follows = "nixpkgs";
    glew-win32.inputs.tinycmmc.follows = "tinycmmc";

    # PhysFS sources for EMSCRIPTEN / Android / R36S when system PhysFS is absent
    # and external/physfs submodule is not checked out.
    physfs-src = {
      url = "https://github.com/icculus/physfs/archive/refs/tags/release-3.2.0.tar.gz";
      flake = false;
    };

    # Android: SDL2 sources for ndk-build prebuilts
    sdl2-src = {
      url = "https://github.com/libsdl-org/SDL/releases/download/release-2.30.3/SDL2-2.30.3.tar.gz";
      flake = false;
    };
    sdl2-image-src = {
      url = "https://github.com/libsdl-org/SDL_image/releases/download/release-2.8.2/SDL2_image-2.8.2.tar.gz";
      flake = false;
    };

  };

  outputs = { self, nixpkgs, flake-utils,
              tinycmmc, sexpcpp, curl-win32, logmich,
              SDL2-win32, SDL2_image-win32, freetype-win32, physfs-win32, SDL2_ttf-win32,
              strutcpp, miniswig, xdgcpp, wstsound, squirrel, glew-win32, physfs-src, sdl2-src, sdl2-image-src }:

    tinycmmc.lib.eachSystemWithPkgs (pkgs:
      {
        packages = rec {
          default = supertux-origins;

          # Vendored C++ deps from external/ (patchable; no remote flake needed).
          logmich-pkg = pkgs.callPackage ./external/logmich/logmich.nix { };
          sexpcpp-pkg = (pkgs.callPackage ./external/sexpcpp/sexpcpp.nix { }).overrideAttrs (o: {
            doCheck = false;
            cmakeFlags = [ "-DBUILD_TESTS=OFF" "-DWARNINGS=OFF" "-DWERROR=OFF" ];
          });
          strutcpp-pkg = (pkgs.callPackage ./external/strutcpp/strutcpp.nix { }).overrideAttrs (o: {
            doCheck = false;
            cmakeFlags = [ "-DBUILD_TESTS=OFF" "-DWARNINGS=OFF" "-DWERROR=OFF" ];
          });
          priocpp-pkg = pkgs.callPackage ./external/priocpp/priocpp.nix {
            inherit self;
            logmich = logmich-pkg;
            sexpcpp = sexpcpp-pkg;
            withJsoncpp = false;
            withSexpcpp = true;
          };

          supertux-origins = pkgs.callPackage ./supertux-origins.nix {
            inherit self;

            SDL2_ttf = if pkgs.stdenv.hostPlatform.isWindows
                       then SDL2_ttf-win32.packages.${pkgs.stdenv.hostPlatform.system}.default
                       else pkgs.SDL2_ttf;

            sexpcpp = sexpcpp-pkg;
            squirrel = squirrel.packages.${pkgs.stdenv.hostPlatform.system}.default;
            tinycmmc = tinycmmc.packages.${pkgs.stdenv.hostPlatform.system}.default;
            strutcpp = strutcpp-pkg;
            miniswig = miniswig.packages.${pkgs.stdenv.hostPlatform.system}.default;
            wstsound = wstsound.packages.${pkgs.stdenv.hostPlatform.system}.default;
            priocpp = priocpp-pkg;
            logmich = logmich-pkg;

            physfs = if pkgs.stdenv.hostPlatform.isWindows
                     then physfs-win32.packages.${pkgs.stdenv.hostPlatform.system}.default
                     else pkgs.physfs;

            curl = if pkgs.stdenv.hostPlatform.isWindows
                   then curl-win32.packages.${pkgs.stdenv.hostPlatform.system}.default
                   else pkgs.curl;

            glew = if pkgs.stdenv.hostPlatform.isWindows
                   then glew-win32.packages.${pkgs.stdenv.hostPlatform.system}.default
                   else pkgs.glew;

            glm = (pkgs.glm.overrideAttrs (oldAttrs: { meta = {}; }));

            SDL2 = if pkgs.stdenv.hostPlatform.isWindows
                   then SDL2-win32.packages.${pkgs.stdenv.hostPlatform.system}.default
                   else pkgs.SDL2;

            SDL2_image = if pkgs.stdenv.hostPlatform.isWindows
                         then SDL2_image-win32.packages.${pkgs.stdenv.hostPlatform.system}.default
                         else pkgs.SDL2_image;

            xdgcpp = if !pkgs.stdenv.hostPlatform.isWindows
                     then xdgcpp.packages.${pkgs.stdenv.hostPlatform.system}.default
                     else null;

            mcfgthreads = pkgs.windows.mcfgthreads;
            gtest = pkgs.gtest;
          };

          supertux-origins-win32 = pkgs.runCommand "supertux-origins-win32" {} ''
            mkdir -p $out
            mkdir -p $out/data/

            cp --verbose --recursive ${supertux-origins}/bin/supertux-origins.exe $out/
            cp --verbose --recursive --dereference --no-preserve=all ${supertux-origins}/bin/*.dll $out/
            cp --verbose --recursive ${supertux-origins}/data/. $out/data/
          '';

          supertux-origins-win32-zip = pkgs.runCommand "supertux-origins-win32-zip" {} ''
            mkdir -p $out
            WORKDIR=$(mktemp -d)

            cp --no-preserve mode,ownership --verbose --recursive \
              ${supertux-origins-win32}/. "$WORKDIR"

            cd "$WORKDIR"
            ${nixpkgs.legacyPackages.x86_64-linux.zip}/bin/zip \
              -r \
              $out/SuperTux-${supertux-origins.version}-${pkgs.stdenv.hostPlatform.system}.zip \
              .
          '';

          # WebAssembly (Emscripten).  Currently marked broken until static
          # wasm builds of physfs / squirrel / wstsound / tinycmmc stack land;
          # CMake EMSCRIPTEN path and mk/emscripten/template.html.in are ready.
          # See nix/wasm.nix and PORTING.md.
          supertux-wasm = (import ./nix/wasm.nix {
            inherit pkgs self tinycmmc sexpcpp logmich strutcpp miniswig
                    wstsound squirrel physfs-src;
          }).supertux-wasm;

          # ---------------------------------------------------------------
          # Android (requires allowUnfree + android_sdk.accept_license)
          #   nix build .#supertux-android
          #   nix build .#android-sdl-libs
          # Still broken until jni links full game deps from external/.
          # ---------------------------------------------------------------
        } // (
          let
            androidPkgs = import nixpkgs {
              system = pkgs.stdenv.hostPlatform.system;
              config.allowUnfree = true;
              config.android_sdk.accept_license = true;
            };
            buildToolsVersion = "34.0.0";
            packagePlatform = "22";
            compilePlatform = "34";
            targetAbis = [ "armeabi-v7a" "arm64-v8a" ];
            androidSdk = (androidPkgs.androidenv.composeAndroidPackages {
              platformVersions = [ packagePlatform compilePlatform ];
              buildToolsVersions = [ buildToolsVersion ];
              includeNDK = true;
              ndkVersion = "26.1.10909125";
            }).androidsdk;
            android = import ./nix/android.nix {
              pkgs = androidPkgs;
              sdlSrc = sdl2-src;
              sdlVersion = "2.30.3";
              sdlMixerSrc = null;
              sdlMixerVersion = "2.8.0";
              libxmpSrc = null;
              inherit androidSdk buildToolsVersion packagePlatform compilePlatform targetAbis;
            };
            androidApkName = "supertux-origins.apk";
            stbImageH = androidPkgs.fetchurl {
              url = "https://raw.githubusercontent.com/nothings/stb/refs/heads/master/stb_image.h";
              sha256 = "sha256-WUwv411JSItDgtv67I+YNm3vyoGdkWrJW+zz519CALM=";
            };
            apk = android.mkApk {
              appName = "supertux-origins";
              appDir = ./mk/android/app;
              outApkName = androidApkName;
              keystore = ./mk/android/keystore/debug.keystore;
              gameSrcDir = ./src;
              gameExternalDir = ./external;
              glmIncludeDir = "${androidPkgs.glm}/include";
              gameDataDir = ./data;
              inherit stbImageH;
              gameVersion = "0.6.3-dev";
            };
          in {
            android-sdl-libs = android.sdlAndroidLibs;
            supertux-android = apk.overrideAttrs (old: {
              meta = (old.meta or {}) // {
                description = "SuperTux Origins Android APK (WIP)";
                broken = true;
              };
            });
          }
        ) // (
          # R36S / ArkOS — sysroot URL is still a localhost placeholder (PORTING.md).
          let
            r36s = import ./nix/r36s.nix {
              inherit (pkgs) lib stdenv stdenvNoCC fetchurl cmake pkg-config writeShellScript zip glm;
              pkgsCross = pkgs.pkgsCross;
            };
          in {
            arkos-sysroot = r36s.arkosSysroot.overrideAttrs (o: {
              meta = (o.meta or {}) // { broken = true; };
            });
            # Full game cross-build needs a published sysroot; keep broken.
            # supertux-r36s = r36s.mkSuperTuxR36s { ... };
          }
        );


        apps = {
          # adb install helper once APK builds
          # install-android-supertux = ...
        };
      }
    );
}
