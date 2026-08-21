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

    tinycmmc.url = "path:./external/tinycmmc";
    tinycmmc.inputs.nixpkgs.follows = "nixpkgs";
    tinycmmc.inputs.flake-utils.follows = "flake-utils";

    physfs-win32.url = "github:grumnix/physfs-win32";
    physfs-win32.inputs.nixpkgs.follows = "nixpkgs";
    physfs-win32.inputs.tinycmmc.follows = "tinycmmc";

    SDL2-win32.url = "github:grumnix/SDL2-win32";
    SDL2-win32.inputs.nixpkgs.follows = "nixpkgs";

    SDL2_image-win32.url = "github:grumnix/SDL2_image-win32";
    SDL2_image-win32.inputs.nixpkgs.follows = "nixpkgs";

    SDL2_ttf-win32.url = "github:grumnix/SDL2_ttf-win32";
    SDL2_ttf-win32.inputs.nixpkgs.follows = "nixpkgs";
    SDL2_ttf-win32.inputs.tinycmmc.follows = "tinycmmc";

    # Prebuilt/cross MinGW OpenAL Soft + libmodplug (Pingus pattern; avoid
    # pkgsCross.openal → ffmpeg on Windows).
    openal-soft-win32.url = "git+https://github.com/grumnix/openal-soft-win32.git";
    openal-soft-win32.inputs.nixpkgs.follows = "nixpkgs";
    openal-soft-win32.inputs.flake-utils.follows = "flake-utils";

    libmodplug-win32.url = "git+https://github.com/grumnix/libmodplug-win32.git";
    libmodplug-win32.inputs.nixpkgs.follows = "nixpkgs";
    libmodplug-win32.inputs.flake-utils.follows = "flake-utils";

    miniswig.url = "github:WindstilleTeam/miniswig";
    miniswig.inputs.nixpkgs.follows = "nixpkgs";
    miniswig.inputs.tinycmmc.follows = "tinycmmc";

    xdgcpp.url = "github:grumbel/xdgcpp";
    xdgcpp.inputs.nixpkgs.follows = "nixpkgs";
    xdgcpp.inputs.flake-utils.follows = "flake-utils";

    squirrel.url = "github:grumnix/squirrel";
    squirrel.inputs.nixpkgs.follows = "nixpkgs";
    squirrel.inputs.tinycmmc.follows = "tinycmmc";

    # PhysFS sources for EMSCRIPTEN / Android / R36S when system PhysFS is absent
    # and external/physfs submodule is not checked out.
    physfs-src = {
      url = "https://github.com/icculus/physfs/archive/refs/tags/release-3.2.0.tar.gz";
      flake = false;
    };

    # Squirrel language sources (external/squirrel is only a packaging flake).
    squirrel-src = {
      url = "github:albertodemichelis/squirrel?rev=f77074bdd6152d230609146a3d424c6f49e3770f";
      flake = false;
    };

    # SDL2_ttf sources for R36S / Android when system SDL2_ttf is absent.
    sdl2-ttf-src = {
      url = "https://github.com/libsdl-org/SDL_ttf/archive/refs/tags/release-2.22.0.tar.gz";
      flake = false;
    };

    # FreeType for Android SDL2_ttf (and offline targets without system FT).
    freetype-src = {
      url = "https://download-mirror.savannah.gnu.org/releases/freetype/freetype-2.13.2.tar.xz";
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
              tinycmmc,
              SDL2-win32, SDL2_image-win32, physfs-win32, SDL2_ttf-win32,
              openal-soft-win32, libmodplug-win32,
              miniswig, xdgcpp, squirrel, physfs-src, squirrel-src, sdl2-ttf-src, freetype-src, sdl2-src, sdl2-image-src }:

    # Linux only — no Darwin (nixpkgs 26.11 dropped x86_64-darwin; we do not
    # target macOS). Windows is a *cross* target via pkgsCross.mingwW64, not a
    # flake system.
    flake-utils.lib.eachSystem [ "x86_64-linux" "aarch64-linux" ] (system:
      let
        pkgs = import nixpkgs {
          inherit system;
          config.allowUnfree = true;
        };
        lib = pkgs.lib;
        # VERSION file is the only source of truth (see PORTING.md / VERSION).
        versionBase = lib.strings.removeSuffix "\n" (builtins.readFile ./VERSION);
        gitRev = self.shortRev or self.dirtyShortRev or "dirty";
        # Dev builds: 0.6.4-dev.<revCount>+g<hash>[-dirty]
        # Releases (no -dev): plain VERSION contents.
        version =
          if lib.hasInfix "-dev" versionBase then
            "${versionBase}.${toString (self.revCount or 0)}+g${gitRev}"
          else
            versionBase;
        isWin = pkgs.stdenv.hostPlatform.isWindows;
        # Wine apps only make sense when *evaluating* a Linux flake system.
        # Use buildPackages for the wrapper script so we never pull host bash
        # from a Windows/MinGW pkgs (error: bash not available on x86_64-windows).
        wineAppsEnabled =
          pkgs.stdenv.buildPlatform.isLinux
          && pkgs.stdenv.hostPlatform.isLinux
          && !isWin
          # wineWow64Packages is x86_64-oriented; skip on aarch64 flake systems.
          && system == "x86_64-linux";
        mkWineApp = pkg: name: description:
          # Caller must gate with wineAppsEnabled; still guard the script body.
          assert wineAppsEnabled;
          {
            type = "app";
            program = toString (pkgs.buildPackages.writeShellScript name ''
              set -euo pipefail
              export WINEPREFIX=$(mktemp -d)
              export WINEARCH=win64
              export WINEDLLOVERRIDES="mscoree,mshtml="
              # Prefer bundled SDL2 DLLs next to the exe over Wine's.
              export WINEDLLOVERRIDES="SDL2,SDL2_image,SDL2_ttf=n,$WINEDLLOVERRIDES"
              trap 'rm -rf "$WINEPREFIX"' EXIT
              ${pkgs.wineWow64Packages.stable}/bin/wineboot --init >/dev/null 2>&1 || true
              # Flat package: exe + dlls at root. Store mingw package: bin/*.exe.
              if [ -d ${pkg}/bin ]; then
                cd ${pkg}/bin
              else
                cd ${pkg}
              fi
              exe=
              for c in supertux-origins.exe SuperTux.exe *.exe; do
                if [ -f "$c" ]; then exe="$c"; break; fi
              done
              if [ -z "$exe" ]; then
                echo "error: no .exe found under ${pkg}" >&2
                find ${pkg} -maxdepth 3 -type f >&2 || true
                exit 1
              fi
              exec ${pkgs.wineWow64Packages.stable}/bin/wine "./$exe" "$@"
            '');
            meta = {
              description = description;
              platforms = lib.platforms.linux;
            };
          };

        # Helper for MinGW prebuilt flakes (not a package output).

        # Windows (MinGW-w64) — patterned after Pingus mkPingus:
        #   SDL2* from grumnix *-win32 flakes as packages.${system}.SDL2-win64
        #   C++ deps ideally built with pkgsCross.mingwW64 (not host Linux libs)
        #
        # Nix gotcha: `a or b or throw "x"` is parsed as `(a or b or throw) "x"`
        # (function application binds tighter than `or`). Always parenthesize throw.
        # grumnix *-win32 flakes (via tinycmmc eachWin32SystemWithPkgs) publish
        # under packages.x86_64-windows / packages.i686-windows — NOT under the
        # Linux builder system. Prefer the ABI matching pkgsCross.mingwW64.
        pickWinFlakePkg = flake: names: abi:
          let
            all = flake.packages or {};
            systems = builtins.attrNames all;
            # abi: "win64" → x86_64-windows, "win32" → i686-windows
            prefer =
              if abi == "win32" then [
                "i686-windows"
                "x86_64-windows"
                system
                "x86_64-linux"
              ] else [
                "x86_64-windows"
                "i686-windows"
                system
                "x86_64-linux"
              ];
            matches = builtins.filter (s: builtins.elem s systems) prefer;
            sys =
              if matches != [] then builtins.head matches
              else if systems != [] then builtins.head systems
              else throw "pickWinFlakePkg: flake has no packages.*";
            set = all.${sys};
            tryName = n: if builtins.hasAttr n set then set.${n} else null;
            found = builtins.filter (x: x != null) (map tryName names);
          in
            if found != [] then builtins.head found
            else throw ("pickWinFlakePkg: none of ${builtins.toString names} under packages.${sys} (have: ${builtins.toString systems})");


      in
      rec {
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
          # Vendored wstsound (avoid flake input with .gitattributes export-subst).
          wstsound-pkg = (pkgs.callPackage ./external/wstsound/wstsound.nix {
            mcfgthreads = null;
            gtest = null;
          }).overrideAttrs (o: {
            doCheck = false;
            cmakeFlags = [
              "-DWARNINGS=OFF" "-DWERROR=OFF"
              "-DBUILD_TESTS=OFF" "-DBUILD_EXTRA=OFF"
            ];
          });
          priocpp-pkg = pkgs.callPackage ./external/priocpp/priocpp.nix {
            inherit self;
            logmich = logmich-pkg;
            sexpcpp = sexpcpp-pkg;
            withJsoncpp = false;
            withSexpcpp = true;
          };

          # Linux native packages only. Windows is *always* pkgsCross.mingwW64
          # below — never evaluate packages.\${system} with hostPlatform=windows
          # (that confused i686/x86_64-windows flake outputs in older recipes).
          supertux-origins = pkgs.callPackage ./supertux-origins.nix {
            inherit self;
            versionFull = version;
            SDL2 = pkgs.SDL2;
            SDL2_image = pkgs.SDL2_image;
            SDL2_ttf = pkgs.SDL2_ttf;
            sexpcpp = sexpcpp-pkg;
            squirrel = squirrel.packages.${system}.default;
            tinycmmc = tinycmmc.packages.${system}.default;
            strutcpp = strutcpp-pkg;
            miniswig = miniswig.packages.${system}.default;
            wstsound = wstsound-pkg;
            priocpp = priocpp-pkg;
            logmich = logmich-pkg;
            physfs = pkgs.physfs;
            glm = (pkgs.glm.overrideAttrs (oldAttrs: { meta = {}; }));
            xdgcpp = xdgcpp.packages.${system}.default;
            mcfgthreads = null;
            gtest = pkgs.gtest;
          };

          # Desktop GLES2 validation (compile-time USE_OPENGLES2). Not a
          # --renderer switch — Origins has no runtime VIDEO_GLES enum.
          supertux-origins-gles2 = pkgs.callPackage ./supertux-origins.nix {
            inherit self;
            versionFull = version;
            SDL2 = pkgs.SDL2;
            SDL2_image = pkgs.SDL2_image;
            SDL2_ttf = pkgs.SDL2_ttf;
            sexpcpp = sexpcpp-pkg;
            squirrel = squirrel.packages.${system}.default;
            tinycmmc = tinycmmc.packages.${system}.default;
            strutcpp = strutcpp-pkg;
            miniswig = miniswig.packages.${system}.default;
            wstsound = wstsound-pkg;
            priocpp = priocpp-pkg;
            logmich = logmich-pkg;
            physfs = pkgs.physfs;
            glm = (pkgs.glm.overrideAttrs (oldAttrs: { meta = {}; }));
            xdgcpp = xdgcpp.packages.${system}.default;
            mcfgthreads = null;
            gtest = pkgs.gtest;
            useGLES2 = true;
            libGL = pkgs.libGL;
          };
          supertux-origins-mingw64 =
            if pkgs.stdenv.hostPlatform.isWindows then
              supertux-origins
            else
              let
                pkgsW = pkgs.pkgsCross.mingwW64;
                winSuffix = "win64";
                # Pingus: SDL2-* from grumnix on *builder* system, not target.
                sdl2w = pickWinFlakePkg SDL2-win32 [ "SDL2-win64" "default" ] "win64";
                sdl2imgw = pickWinFlakePkg SDL2_image-win32 [ "SDL2_image-win64" "default" ] "win64";
                sdl2ttfw = pickWinFlakePkg SDL2_ttf-win32 [ "SDL2_ttf" "default" ] "win64";
                physfsw = pickWinFlakePkg physfs-win32 [ "default" "physfs" ] "win64";
                openalw = pickWinFlakePkg openal-soft-win32 [ "openal-soft-win64" "default" ] "win64";
                modplugw = pickWinFlakePkg libmodplug-win32 [ "libmodplug-win64" "default" ] "win64";

                # Build C++ deps with the *cross* stdenv (not host Linux .so).
                # Force Linux cmake into nativeBuildInputs via callPackage args.
                bpCmake = pkgs.buildPackages.cmake;
                logmichW = pkgsW.callPackage ./external/logmich/logmich.nix {
                  cmake = bpCmake;
                };
                sexpcppW = (pkgsW.callPackage ./external/sexpcpp/sexpcpp.nix {
                  cmake = bpCmake;
                }).overrideAttrs (o: {
                  doCheck = false;
                  cmakeFlags = [ "-DBUILD_TESTS=OFF" "-DWARNINGS=OFF" "-DWERROR=OFF" ];
                });
                strutcppW = (pkgsW.callPackage ./external/strutcpp/strutcpp.nix {
                  cmake = bpCmake;
                }).overrideAttrs (o: {
                  doCheck = false;
                  cmakeFlags = [ "-DBUILD_TESTS=OFF" "-DWARNINGS=OFF" "-DWERROR=OFF" ];
                });
                priocppW = pkgsW.callPackage ./external/priocpp/priocpp.nix {
                  inherit self;
                  cmake = bpCmake;
                  logmich = logmichW;
                  sexpcpp = sexpcppW;
                  withJsoncpp = false;
                  withSexpcpp = true;
                };
                tinycmmcW = tinycmmc.packages.${system}.default; # cmake modules; host ok
                wstsoundW = (pkgsW.callPackage ./external/wstsound/wstsound.nix {
                  cmake = bpCmake;
                  pkg-config = pkgs.buildPackages.pkg-config;
                  mcfgthreads = pkgsW.windows.mcfgthreads;
                  openal = openalw;
                  libmodplug = modplugw;
                  gtest = null;
                }).overrideAttrs (o: {
                  doCheck = false;
                  cmakeFlags = [
                    "-DWARNINGS=OFF" "-DWERROR=OFF"
                    "-DBUILD_TESTS=OFF" "-DBUILD_EXTRA=OFF"
                  ];
                });
              in
              (pkgsW.callPackage ./supertux-origins.nix {
                inherit self;
                versionFull = version;
                # Build-time tools must come from buildPackages (Linux), never the
                # MinGW target package set — otherwise evaluation can demand
                # bash for hostPlatform=x86_64-windows.
                cmake = pkgs.buildPackages.cmake;
                pkg-config = pkgs.buildPackages.pkg-config;
                SDL2 = sdl2w;
                SDL2_image = sdl2imgw;
                SDL2_ttf = sdl2ttfw;
                physfs = physfsw;
                sexpcpp = sexpcppW;
                # Host squirrel lacks IMPORTED_IMPLIB for MinGW; in-tree like R36S.
                squirrel = null;
                tinycmmc = tinycmmcW;
                strutcpp = strutcppW;
                miniswig = miniswig.packages.${system}.default; # native tool
                wstsound = wstsoundW;
                priocpp = priocppW;
                logmich = logmichW;
                glm = (pkgs.glm.overrideAttrs (oldAttrs: { meta = {}; }));
                xdgcpp = null;
                libGL = null;
                mcfgthreads = pkgsW.windows.mcfgthreads;
                gtest = null;
                makeWrapper = null;
              }).overrideAttrs (o: {
                strictDeps = true;
                cmakeFlags = (o.cmakeFlags or []) ++ [
                  "-DUSE_SYSTEM_SQUIRREL=OFF"
                  "-DSQUIRREL_SOURCE_DIR=${squirrel-src}"
                  # Prebuilt physfs-win32 (3.0.2) has PHYSFS_getPrefDir; force system
                  # so we never require external/physfs under MinGW.
                  "-DUSE_SYSTEM_PHYSFS=ON"
                  # Explicit import lib (avoids find_package picking a wrong/empty path).
                  # physfs cmake installs libphysfs.dll.a under lib/ on MinGW.
                  "-DPHYSFS_LIBRARY=${physfsw}/lib/libphysfs.dll.a"
                  "-DPHYSFS_INCLUDE_DIR=${physfsw}/include"
                  "-DPHYSFS_SOURCE_DIR=${physfs-src}"
                ];
                # Pull audio codec DLLs into the runtime closure for postFixup
                # (libvorbis imports ogg.dll; must be next to the exe for Wine).
                buildInputs = builtins.filter (x: x != null) (
                  (o.buildInputs or [])
                  ++ [
                    pkgsW.libogg
                    pkgsW.libvorbis
                    pkgsW.libopus
                    pkgsW.opusfile
                    pkgsW.mpg123
                    openalw
                    modplugw
                  ]
                );
                nativeBuildInputs = builtins.filter (x: x != null) (o.nativeBuildInputs or []);
              });

          # Flat layout for redistribution (exe + dlls + data). Only valid after a
          # successful MinGW build that installs supertux-origins.exe.
          supertux-origins-win32 = pkgs.runCommand "supertux-origins-win32" {
            meta = {
              description = "SuperTux Origins Windows (MinGW) flat package";
            };
          } ''
            mkdir -p $out/data
            if [ ! -f ${supertux-origins-mingw64}/bin/supertux-origins.exe ]; then
              echo "error: MinGW build did not produce bin/supertux-origins.exe" >&2
              echo "contents of mingw package:" >&2
              find ${supertux-origins-mingw64} -maxdepth 3 -type f >&2 || true
              exit 1
            fi
            cp -v ${supertux-origins-mingw64}/bin/supertux-origins.exe $out/
            cp -v --dereference --no-preserve=all ${supertux-origins-mingw64}/bin/*.dll $out/ 2>/dev/null || true
            if [ -d ${supertux-origins-mingw64}/data ]; then
              cp -a ${supertux-origins-mingw64}/data/. $out/data/
            fi
            # MinGW PE imports often use undecorated names (ogg.dll) while
            # packages install libogg-0.dll — alias so Wine/Windows loaders find them.
            alias_dll() {
              local from="$1" to="$2"
              if [ -f "$out/$from" ] && [ ! -e "$out/$to" ]; then
                cp -L "$out/$from" "$out/$to"
                echo "aliased $from -> $to"
              fi
            }
            alias_dll libogg-0.dll ogg.dll
            alias_dll libogg.dll ogg.dll
            alias_dll libvorbis-0.dll vorbis.dll
            alias_dll libvorbisfile-3.dll vorbisfile.dll
            alias_dll libopus-0.dll opus.dll
            alias_dll libopusfile-0.dll opusfile.dll
            alias_dll libmodplug-1.dll modplug.dll
            alias_dll libmpg123-0.dll mpg123.dll
            # Fail clearly if ogg is still missing (common Wine failure).
            if ! ls "$out"/ogg.dll "$out"/libogg*.dll 1>/dev/null 2>&1; then
              echo "warning: no ogg.dll / libogg*.dll in flat package; audio may fail" >&2
              echo "dlls present:" >&2
              ls -1 "$out"/*.dll 2>/dev/null >&2 || true
            fi
          '';

          supertux-origins-win32-zip = pkgs.runCommand "supertux-origins-win32-zip-${version}" {
          } ''
            mkdir -p $out
            WORKDIR=$(mktemp -d)
            cp --no-preserve mode,ownership -a ${supertux-origins-win32}/. "$WORKDIR"
            cd "$WORKDIR"
            ${pkgs.zip}/bin/zip -r $out/SuperTux-Origins-${version}-win64.zip .
          '';

          # WebAssembly (Emscripten). CMake path ready; may fail at dep/link stage.
          # See nix/wasm.nix and PORTING.md.
          supertux-origins-wasm = (import ./nix/wasm.nix {
            inherit pkgs self tinycmmc miniswig squirrel physfs-src squirrel-src;
            sdlSrc = sdl2-src;
            sdlVersion = "2.30.3";
            freetypeSrc = freetype-src;
            sdl2TtfSrc = sdl2-ttf-src;
            inherit version;
          }).supertux-wasm;

          # ---------------------------------------------------------------
          # Android (requires allowUnfree + android_sdk.accept_license)
          #   nix build .#supertux-origins-android              # all ABIs (default)
          #   nix build .#supertux-origins-android-arm64-v8a    # single ABI (faster)
          #   nix build .#supertux-origins-android-armeabi-v7a
          #   nix build .#supertux-origins-android-x86_64
          #   nix build .#supertux-origins-android-sdl-libs
          # Expect failures until jni links full game deps from external/.
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
            # Full set for SDL/audio prebuilts (shared). Per-ABI APKs subset this.
            allAndroidAbis = [ "armeabi-v7a" "arm64-v8a" "x86_64" ];
            targetAbis = allAndroidAbis;
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
            androidApkName = "supertux-origins-${version}.apk";
            stbImageH = androidPkgs.fetchurl {
              url = "https://raw.githubusercontent.com/nothings/stb/refs/heads/master/stb_image.h";
              sha256 = "sha256-WUwv411JSItDgtv67I+YNm3vyoGdkWrJW+zz519CALM=";
            };
            mkAndroidApk = abis: android.mkApk {
              appName = "supertux-origins";
              appDir = ./mk/android/app;
              outApkName = androidApkName;
              keystore = ./mk/android/keystore/debug.keystore;
              gameSrcDir = ./src;
              gameExternalDir = ./external;
              glmIncludeDir = "${androidPkgs.glm}/include";
              gameDataDir = ./data;
              inherit stbImageH;
              gameVersion = version;
              squirrelSrc = squirrel-src;
              physfsSrc = physfs-src;
              sdl2TtfSrc = sdl2-ttf-src;
              freetypeSrc = freetype-src;
              inherit abis;
            };
            metaAndroid = desc: old: {
              meta = (old.meta or {}) // {
                description = desc;
              };
            };
          in {
            supertux-origins-android-sdl-libs = android.sdlAndroidLibs;
            # Default: all ABIs in one APK (release-style, slower native compile).
            supertux-origins-android = (mkAndroidApk allAndroidAbis).overrideAttrs
              (metaAndroid "SuperTux Origins Android APK (armeabi-v7a + arm64-v8a + x86_64)");
            # Fast single-ABI iteration outputs:
            supertux-origins-android-armeabi-v7a = (mkAndroidApk [ "armeabi-v7a" ]).overrideAttrs
              (metaAndroid "SuperTux Origins Android APK (armeabi-v7a only)");
            supertux-origins-android-arm64-v8a = (mkAndroidApk [ "arm64-v8a" ]).overrideAttrs
              (metaAndroid "SuperTux Origins Android APK (arm64-v8a only)");
            supertux-origins-android-x86_64 = (mkAndroidApk [ "x86_64" ]).overrideAttrs
              (metaAndroid "SuperTux Origins Android APK (x86_64 only)");
          }
        ) // (
          # R36S / ArkOS hybrid cross (modern GCC + ArkOS glibc/SDL2/GLES sysroot).
          # Sysroot tarball URL is still a placeholder — pass a local path:
          #   nix build .#supertux-origins-r36s \
          #     edit nix/r36s.nix: sysrootSrc = /path/to/arkos-sysroot4.tar.gz;
          # or replace the URL in nix/r36s.nix after `nix store prefetch-file`.
          # Local sysroot: mk/r36s/scripts/make-sysroot-debootstrap.sh
          let
            r36s = import ./nix/r36s.nix {
              inherit (pkgs) lib stdenv stdenvNoCC fetchurl cmake pkg-config writeShellScript zip glm;
              pkgsCross = pkgs.pkgsCross;
            };
            gitDate = self.lastModifiedDate or "19700101";
            gitRev = self.shortRev or self.dirtyShortRev or "dirty";
            r36sVersion = version;
            game = r36s.mkSuperTuxR36s {
              src = self;
              version = r36sVersion;
              enableSound = true;
              physfsSrc = physfs-src;
              squirrelSrc = squirrel-src;
              sdl2TtfSrc = sdl2-ttf-src;
              freetypeSrc = freetype-src;
            };
            portMaster = r36s.mkSuperTuxR36sPortMaster {
              r36sPkg = game;
              version = r36sVersion;
            };
          in {
            supertux-origins-arkos-sysroot = r36s.arkosSysroot;
            supertux-origins-r36s = game;
            supertux-origins-r36s-portmaster = portMaster;
            supertux-origins-r36s-portmaster-zip = r36s.mkSuperTuxR36sPortMasterZip {
              portMasterPkg = portMaster;
              version = r36sVersion;
            };
          }
        );


        apps = {
          #   nix run .#supertux-origins-wasm
          supertux-origins-wasm = {
            type = "app";
            program = "${packages.supertux-origins-wasm}/bin/supertux-wasm";
            meta.description = "Serve and open SuperTux Origins wasm in a browser";
          };
        } // lib.optionalAttrs wineAppsEnabled {
          #   nix run .#supertux-origins-win32
          #   nix run .#supertux-origins-mingw64
          supertux-origins-win32 = mkWineApp packages.supertux-origins-win32 "supertux-origins-win32"
            "SuperTux Origins (MinGW x86_64) via Wine";
          supertux-origins-mingw64 = mkWineApp packages.supertux-origins-mingw64 "supertux-origins-mingw64"
            "SuperTux Origins MinGW store package via Wine (bin/)";
        };

        # `nix flake check` builds these (major ports). Android/R36S need SDK
        # or sysroot and are checked for evaluation only via packages.*.
        checks = {
          supertux-origins = packages.supertux-origins;
          supertux-origins-gles2 = packages.supertux-origins-gles2;
          supertux-origins-mingw64 = packages.supertux-origins-mingw64;
          supertux-origins-win32 = packages.supertux-origins-win32;
          supertux-origins-win32-zip = packages.supertux-origins-win32-zip;
          supertux-origins-wasm = packages.supertux-origins-wasm;
          version-file = pkgs.runCommand "supertux-origins-version-check" {
            inherit version versionBase;
          } ''
            echo "versionBase=$versionBase"
            echo "version=$version"
            echo "$version" | grep -q -E '^[0-9]+\.[0-9]+\.[0-9]+'
            mkdir -p $out
            echo "$version" > $out/version
          '';
        } // lib.optionalAttrs wineAppsEnabled {
          app-supertux-origins-win32 = pkgs.runCommand "check-app-win32" {
          } ''
            test -x ${apps.supertux-origins-win32.program}
            mkdir -p $out
            touch $out/ok
          '';
        };
      }
    );
}
