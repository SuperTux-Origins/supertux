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
    flake-utils.url = "github:numtide/flake-utils";

    tinycmmc.url = "github:grumbel/tinycmmc";
    tinycmmc.inputs.nixpkgs.follows = "nixpkgs";

    # Official MinGW SDL2 devel packages (same pattern as pingus).
    # Exposed under packages.x86_64-windows / i686-windows via eachWin32SystemWithPkgs.
    SDL2-win32.url = "github:grumnix/SDL2-win32";
    SDL2-win32.inputs.nixpkgs.follows = "nixpkgs";
    SDL2-win32.inputs.tinycmmc.follows = "tinycmmc";

    SDL2_image-win32.url = "github:grumnix/SDL2_image-win32";
    SDL2_image-win32.inputs.nixpkgs.follows = "nixpkgs";
    SDL2_image-win32.inputs.tinycmmc.follows = "tinycmmc";

    SDL2_mixer-win32-x64 = {
      url = "https://github.com/libsdl-org/SDL_mixer/releases/download/release-2.8.1/SDL2_mixer-2.8.1-win32-x64.zip";
      flake = false;
    };

    SDL2_mixer-win32-x86 = {
      url = "https://github.com/libsdl-org/SDL_mixer/releases/download/release-2.8.1/SDL2_mixer-2.8.1-win32-x86.zip";
      flake = false;
    };

    # Official MinGW devel archive (same shape as SDL2_image-win32).
    sdl2-mixer-mingw-devel = {
      url = "https://github.com/libsdl-org/SDL_mixer/releases/download/release-2.8.1/SDL2_mixer-devel-2.8.1-mingw.tar.gz";
      flake = false;
    };

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

  outputs = { self, nixpkgs, flake-utils, tinycmmc, SDL2-win32, SDL2_image-win32
            , sdl2-mixer-mingw-devel, SDL2_mixer-win32-x64, SDL2_mixer-win32-x86
            , sdl2-src, sdl2-image-src, sdl2-mixer-src, libxmp-src }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
        lib = nixpkgs.lib;
        isWin = pkgs.stdenv.hostPlatform.isWindows;
        versionBase = lib.strings.removeSuffix "\n" (builtins.readFile ./VERSION);
        gitRev = "${self.shortRev or self.dirtyShortRev or "dirty"}";
        version = "${versionBase}+g${gitRev}";

        # ---- SuperTux derivation (native Linux) ----
        mkSuperTuxLinux = {
          useSDL2 ? true,
          useGLES2 ? false,
          enableDebug ? false,
          pname ? "supertux-milestone1",
        }:
          let
            effectiveSDL2 = useSDL2 || useGLES2;
          in
          pkgs.stdenv.mkDerivation rec {
            inherit pname version;
            src = lib.cleanSource ./.;
            enableParallelBuilding = true;
            dontStrip = true;
            separateDebugInfo = true;
            nativeBuildInputs = [
              pkgs.buildPackages.cmake
              pkgs.buildPackages.pkg-config
              pkgs.addDriverRunpath
            ];
            cmakeFlags = [
              "-DCMAKE_BUILD_TYPE=${if enableDebug then "Debug" else "RelWithDebInfo"}"
              "-DENABLE_SOUND=ON"
              "-DENABLE_OPENGL=ON"
              "-DENABLE_GLES2=${if useGLES2 then "ON" else "OFF"}"
              "-DENABLE_SDL2=${if effectiveSDL2 then "ON" else "OFF"}"
              "-DENABLE_DEBUG=${if enableDebug then "ON" else "OFF"}"
              "-DDATA_PREFIX=${placeholder "out"}/share/supertux-milestone1"
              "-DPROJECT_VERSION_FULL=${version}"
            ];
            buildInputs = (
              if effectiveSDL2 then [
                pkgs.SDL2 pkgs.SDL2_image pkgs.SDL2_mixer
              ] else [
                pkgs.SDL pkgs.SDL_image pkgs.SDL_mixer
              ]
            ) ++ [ pkgs.zlib ]
              ++ (if useGLES2 then [ pkgs.libGL pkgs.libglvnd ]
                  else if effectiveSDL2 then [ pkgs.libGL ]
                  else [ pkgs.libGL pkgs.libGLU ])
              ++ [ pkgs.libpng pkgs.libjpeg pkgs.libtiff ];
            postFixup = ''
              addDriverRunpath $out/bin/supertux-milestone1
            '';
            meta = with lib; {
              description = "SuperTux Milestone 1";
              license = licenses.gpl2Plus;
              platforms = platforms.linux;
              mainProgram = "supertux-milestone1";
            };
          };

        pkgSdl2 = mkSuperTuxLinux { useSDL2 = true; pname = "supertux-milestone1-sdl2"; };
        pkgSdl1 = mkSuperTuxLinux { useSDL2 = false; pname = "supertux-milestone1-sdl1"; };
        pkgSdl2Gles2 = mkSuperTuxLinux {
          useSDL2 = true; useGLES2 = true;
          pname = "supertux-milestone1-sdl2-gles2";
        };
        # Desktop SDL2 with ENABLE_DEBUG: -O0 -g3, extra warnings (see CMakeLists).
        pkgSdl2Debug = mkSuperTuxLinux {
          useSDL2 = true;
          enableDebug = true;
          pname = "supertux-milestone1-sdl2-debug";
        };

        # ---- Windows cross (from Linux only, same idea as android/wasm) ----
        # SDL2-win32 flakes expose packages under x86_64-windows / i686-windows.
        # Build with pkgsCross; stage .exe + DLLs + data/ into a flat output.
        # Official SDL2_mixer mingw devel → $out (like SDL2_image-win32 template).
        # winSystem: x86_64-windows | i686-windows → triplet for the archive layout.
        mkSdl2MixerWin = winSystem:
          let
            triplet =
              if winSystem == "x86_64-windows" then "x86_64-w64-mingw32"
              else if winSystem == "i686-windows" then "i686-w64-mingw32"
              else throw "mkSdl2MixerWin: unknown winSystem ${winSystem}";
          in
          pkgs.stdenvNoCC.mkDerivation {
            pname = "SDL2_mixer";
            version = "2.8.1";
            src = sdl2-mixer-mingw-devel;
            # Prebuilt tree; no compile.
            dontConfigure = true;
            dontBuild = true;
            installPhase = ''
              mkdir $out
              find . -print
              cp -vr ${triplet}/. $out/
              substituteInPlace $out/lib/pkgconfig/SDL2_mixer.pc \
                --replace "prefix=/tmp/tardir/SDL2_mixer-2.8.1/build-mingw/install-${triplet}" \
                          "prefix=$out"
               cp -v ${if winSystem == "x86_64-windows" then SDL2_mixer-win32-x64 else SDL2_mixer-win32-x86}/optional/libxmp.dll $out/
            '';
          };

        mkWinCross = { crossPkgs, winSystem, pname }:
          let
            sdl2Win = SDL2-win32.packages.${winSystem}.default;
            sdl2ImageWin = SDL2_image-win32.packages.${winSystem}.default;
            sdl2MixerWin = mkSdl2MixerWin winSystem;
            game = crossPkgs.stdenv.mkDerivation {
              inherit pname version;
              src = lib.cleanSource ./.;
              enableParallelBuilding = true;
              dontStrip = true;
              separateDebugInfo = false;
              nativeBuildInputs = [
                crossPkgs.buildPackages.cmake
                crossPkgs.buildPackages.pkg-config
              ];
              cmakeFlags = [
                "-DCMAKE_BUILD_TYPE=RelWithDebInfo"
                "-DENABLE_SOUND=ON"
                "-DENABLE_OPENGL=ON"
                "-DENABLE_GLES2=OFF"
                "-DENABLE_SDL2=ON"
                "-DDATA_PREFIX=${placeholder "out"}/share/supertux-milestone1"
                "-DPROJECT_VERSION_FULL=${version}"
              ];
              buildInputs = [ sdl2Win sdl2ImageWin sdl2MixerWin crossPkgs.zlib ];
              postFixup = ''
                mkdir -p $out/bin/
                find ${crossPkgs.windows.mcfgthreads} -iname "*.dll" -exec ln -sfv {} $out/bin/ \;
                find ${crossPkgs.stdenv.cc.cc} -iname "*.dll" -exec ln -sfv {} $out/bin/ \;
                ln -sfv ${sdl2Win}/bin/*.dll $out/bin/
                ln -sfv ${sdl2ImageWin}/bin/*.dll $out/bin/
                ln -sfv ${sdl2MixerWin}/bin/*.dll $out/bin/ || true
                # Optional codec DLLs shipped next to SDL2_mixer.dll in some archives.
                find ${sdl2MixerWin} -iname "*.dll" -exec ln -sfv {} $out/bin/ \; || true
              '';
              meta = with lib; {
                description = "SuperTux Milestone 1 (${pname})";
                license = licenses.gpl2Plus;
                platforms = platforms.windows;
                mainProgram = "supertux-milestone1";
              };
            };
          in
          # Flat redistributable: .exe + DLLs + data/ at the root (pingus-style).
          pkgs.runCommand pname { } ''
            mkdir -p $out/data
            cp -vr ${game}/bin/supertux-milestone1.exe $out/ 2>/dev/null \
              || cp -vr ${game}/bin/*.exe $out/
            cp -vLr ${game}/bin/*.dll $out/ 2>/dev/null || true
            if [ -d ${game}/share/supertux-milestone1 ]; then
              cp -vr ${game}/share/supertux-milestone1/. $out/data/
            fi
          '';

        # Only define cross packages on Linux build hosts.
        win64Package = if isWin then null else mkWinCross {
          crossPkgs = pkgs.pkgsCross.mingwW64;
          winSystem = "x86_64-windows";
          pname = "supertux-milestone1-win32-x64";
        };
        win32Package = if isWin then null else mkWinCross {
          crossPkgs = pkgs.pkgsCross.mingw32;
          winSystem = "i686-windows";
          pname = "supertux-milestone1-win32-x86";
        };


        # ---- Linux-only: Android + wasm (must not eval on *-windows) ----
        linuxExtras =
          if isWin then { packages = {}; apps = {}; checks = {}; }
          else
          let
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
            gp2x = import ./nix/gp2x.nix {
              inherit (pkgs) lib stdenv stdenvNoCC fetchurl cmake pkg-config qemu file pkgsi686Linux bash binutils;
            };
            r36s = import ./nix/r36s.nix {
              inherit (pkgs) lib stdenv stdenvNoCC fetchurl cmake pkg-config writeShellScript zip;
              pkgsCross = pkgs.pkgsCross;
            };
            # Build once; packages and PortMaster packaging both consume this.
            supertuxMilestone1R36s = r36s.mkSuperTuxR36s {
              src = lib.cleanSource ./.;
              inherit version;
              pname = "supertux-milestone1-r36s";
            };
            supertuxMilestone1R36sPortMaster = r36s.mkSuperTuxR36sPortMaster {
              r36sPkg = supertuxMilestone1R36s;
              inherit version;
              pname = "supertux-milestone1-r36s-portmaster";
              screenshotSrc = ./supertux-milestone1.png;
            };
            android = import ./nix/android.nix {              pkgs = androidPkgs;
              sdlSrc = sdl2-src;
              sdlVersion = "2.30.3";
              sdlMixerSrc = sdl2-mixer-src;
              sdlMixerVersion = "2.8.0";
              libxmpSrc = libxmp-src;
              inherit androidSdk buildToolsVersion packagePlatform compilePlatform targetAbis;
            };
            wasm = import ./nix/wasm.nix {
              inherit pkgs;
              sdlSrc = sdl2-src;
              sdlImageSrc = sdl2-image-src;
              sdlMixerSrc = sdl2-mixer-src;
              libxmpSrc = libxmp-src;
              sdlVersion = "2.30.3";
            };
            wasmDataDir = if builtins.pathExists ./data then ./data else null;
            gitDate =
              if self ? lastModifiedDate then builtins.substring 0 8 self.lastModifiedDate
              else "00000000";
            androidApkName = "supertux-milestone1-${gitDate}-${gitRev}.apk";
            stbImageH = androidPkgs.fetchurl {
              url = "https://raw.githubusercontent.com/nothings/stb/refs/heads/master/stb_image.h";
              sha256 = "sha256-WUwv411JSItDgtv67I+YNm3vyoGdkWrJW+zz519CALM=";
            };
          in {
            packages = {
              open2x-sysroot = gp2x.open2xSysroot;
              openwiz-sysroot = gp2x.openwizSysroot;
              arkos-sysroot = r36s.arkosSysroot;
              supertux-milestone1-gp2x = gp2x.mkSuperTuxGp2x {
                src = lib.cleanSource ./.;
                inherit version;
                pname = "supertux-milestone1-gp2x";
              };
              supertux-milestone1-wiz = gp2x.mkSuperTuxWiz {
                src = lib.cleanSource ./.;
                inherit version;
                pname = "supertux-milestone1-wiz";
              };
              # aarch64 SDL2+GLES2 linked against published ArkOS sysroot.
              # See mk/r36s/CROSSCOMPILE.md and nix/r36s.nix.
              supertux-milestone1-r36s = supertuxMilestone1R36s;
              # PortMaster-ready tree: launcher + data + metadata for /roms/ports.
              #   nix build .#supertux-milestone1-r36s-portmaster
              #   cp -a result/* /roms/ports/
              supertux-milestone1-r36s-portmaster = supertuxMilestone1R36sPortMaster;
              # Single zip for PortMaster autoinstall:
              #   nix build .#supertux-milestone1-r36s-portmaster-zip
              #   → result/supertux-milestone1.zip → ports/PortMaster/autoinstall/
              supertux-milestone1-r36s-portmaster-zip = r36s.mkSuperTuxR36sPortMasterZip {
                portMasterPkg = supertuxMilestone1R36sPortMaster;
                inherit version;
                pname = "supertux-milestone1-r36s-portmaster-zip";
              };
              android-sdl-libs = android.sdlAndroidLibs;
              supertux-milestone1-android = android.mkApk {
                appName = "supertux-milestone1";
                appDir = ./mk/android/app;
                outApkName = androidApkName;
                keystore = ./mk/android/keystore/debug.keystore;
                gameSrcDir = ./src;
                gameDataDir = ./data;
                stbImageH = stbImageH;
                gameVersion = version;
              };
              wasm-sdl2 = wasm.sdl2WasmLibs;
              wasm-sdl2-image = wasm.sdl2Image;
              wasm-sdl2-mixer = wasm.sdl2Mixer;
              wasm-sdl-libs = wasm.sdlWasmLibs;
              wasm-zlib-libs = wasm.zlibWasmLibs;
              supertux-milestone1-wasm = wasm.mkApp {
                appName = "supertux-milestone1";
                srcDir = ./.;
                dataDir = wasmDataDir;
                enableSound = true;
                enableGles2 = true;
                enableAsyncify = false;
                versionFull = version;
                gitRev = gitRev;
                sourceUrl = "https://github.com/SuperTux-Origins/supertux-milestone1";
              };
            };
            apps = {
              install-android-supertux-milestone1 = android.mkInstallApp {
                pkg = android.mkApk {
                  appName = "supertux-milestone1";
                  appDir = ./mk/android/app;
                  outApkName = androidApkName;
                  keystore = ./mk/android/keystore/debug.keystore;
                  gameSrcDir = ./src;
                  gameDataDir = ./data;
                  stbImageH = stbImageH;
                  gameVersion = version;
                };
                apkFileName = androidApkName;
              };
              supertux-milestone1-wasm = wasm.mkOpenBrowserApp {
                pkg = wasm.mkApp {
                  appName = "supertux-milestone1";
                  srcDir = ./.;
                  dataDir = wasmDataDir;
                  enableSound = true;
                  enableGles2 = true;
                  enableAsyncify = false;
                  versionFull = version;
                  gitRev = gitRev;
                  sourceUrl = "https://github.com/SuperTux-Origins/supertux-milestone1";
                };
                appName = "supertux-milestone1";
              };
            };
            checks = {
              open2x-sysroot = gp2x.open2xSysroot;
              openwiz-sysroot = gp2x.openwizSysroot;
              arkos-sysroot = r36s.arkosSysroot;
              supertux-milestone1-gp2x = gp2x.mkSuperTuxGp2x {
                src = lib.cleanSource ./.;
                inherit version;
                pname = "supertux-milestone1-gp2x";
              };
              supertux-milestone1-wiz = gp2x.mkSuperTuxWiz {
                src = lib.cleanSource ./.;
                inherit version;
                pname = "supertux-milestone1-wiz";
              };
              supertux-milestone1-r36s = supertuxMilestone1R36s;
              android-sdl-libs = android.sdlAndroidLibs;
              supertux-milestone1-android = android.mkApk {
                appName = "supertux-milestone1";
                appDir = ./mk/android/app;
                outApkName = androidApkName;
                keystore = ./mk/android/keystore/debug.keystore;
                gameSrcDir = ./src;
                gameDataDir = ./data;
                stbImageH = stbImageH;
                gameVersion = version;
              };
              wasm-sdl-libs = wasm.sdlWasmLibs;
              supertux-milestone1-wasm = wasm.mkApp {
                appName = "supertux-milestone1";
                srcDir = ./.;
                dataDir = wasmDataDir;
                enableSound = true;
                enableGles2 = true;
                enableAsyncify = false;
                versionFull = version;
                gitRev = gitRev;
                sourceUrl = "https://github.com/SuperTux-Origins/supertux-milestone1";
              };
            };
          };

        # Wine runner (helloworld-fireos nix/windows.nix mkRunApp pattern).
        # wineWow64Packages (Wine 10+): single `wine` binary; temp WINEPREFIX each run.
        mkWineApp = pkg: name: description: {
          type = "app";
          program = toString (pkgs.writeShellScript name ''
            set -euo pipefail
            export WINEPREFIX=$(mktemp -d)
            export WINEARCH=win64
            export WINEDLLOVERRIDES="mscoree,mshtml="
            # Prefer colocated SDL2.dll over Wine's built-in.
            export WINEDLLOVERRIDES="SDL2=n,$WINEDLLOVERRIDES"
            trap 'rm -rf "$WINEPREFIX"' EXIT
            ${pkgs.wineWow64Packages.stable}/bin/wineboot --init >/dev/null 2>&1 || true
            cd ${pkg}
            exe=
            for c in supertux-milestone1.exe *.exe; do
              if [ -f "$c" ]; then exe="$c"; break; fi
            done
            if [ -z "$exe" ]; then
              echo "error: no .exe found in ${pkg}" >&2
              exit 1
            fi
            exec ${pkgs.wineWow64Packages.stable}/bin/wine "./$exe" "$@"
          '');
          meta.description = description;
        };
        mkWin32Zip = pkg: name: pkgs.runCommand name {} ''
            mkdir -p $out
            WORKDIR=$(mktemp -d)

            cp --no-preserve mode,ownership --verbose --recursive \
              ${pkg}/. "$WORKDIR"

            cd "$WORKDIR"
            ${nixpkgs.legacyPackages.x86_64-linux.zip}/bin/zip \
              -r \
              $out/${name}-${version}-${system}.zip \
              .
          '';


        # Lightweight output shape checks (used by checks.*).
        mkSanity = name: pkg: script:
          pkgs.runCommand "check-${name}" {
            nativeBuildInputs = [ pkgs.file pkgs.binutils ];
          } ''
            set -euo pipefail
            echo "== sanity: ${name} =="
            ${script}
            mkdir -p "$out"
            echo ok > "$out/result"
          '';

      in rec {
        packages = {
          default = pkgSdl2;
          supertux-milestone1-sdl2 = pkgSdl2;
        } // lib.optionalAttrs (!isWin) {
          # Native Linux + Windows cross under the *build* system (like android/wasm).
          supertux-milestone1-sdl1 = pkgSdl1;
          supertux-milestone1-sdl2-gles2 = pkgSdl2Gles2;
          # SDL2 + ENABLE_DEBUG (unoptimized, extra warnings).
          supertux-milestone1-sdl2-debug = pkgSdl2Debug;
          supertux-milestone1-win32-x64 = win64Package; # mingwW64 → x86_64 PE
          supertux-milestone1-win32-x86 = win32Package; # mingw32  → i686 PE
          supertux-milestone1-win32-x64-zip = mkWin32Zip win64Package "supertux-milestone1";
          supertux-milestone1-win32-x86-zip = mkWin32Zip win32Package "supertux-milestone1";
        } // linuxExtras.packages;

        checks = {
          # Always build the default native package.
          supertux-milestone1-sdl2 = pkgSdl2;
          sanity-sdl2 = mkSanity "sdl2" pkgSdl2 ''
            test -x ${pkgSdl2}/bin/supertux-milestone1
            file ${pkgSdl2}/bin/supertux-milestone1 | grep -qi 'elf\|executable'
            test -d ${pkgSdl2}/share/supertux-milestone1 || test -d ${pkgSdl2}/share
          '';
        } // lib.optionalAttrs (!isWin) {
          supertux-milestone1-sdl1 = pkgSdl1;
          supertux-milestone1-sdl2-gles2 = pkgSdl2Gles2;
          supertux-milestone1-win32-x64 = win64Package;
          supertux-milestone1-win32-x86 = win32Package;
          supertux-milestone1-win32-x64-zip = mkWin32Zip win64Package "supertux-milestone1";
          supertux-milestone1-win32-x86-zip = mkWin32Zip win32Package "supertux-milestone1";

          sanity-sdl1 = mkSanity "sdl1" pkgSdl1 ''
            test -x ${pkgSdl1}/bin/supertux-milestone1
            file ${pkgSdl1}/bin/supertux-milestone1 | grep -qi 'elf\|executable'
          '';
          sanity-sdl2-gles2 = mkSanity "sdl2-gles2" pkgSdl2Gles2 ''
            test -x ${pkgSdl2Gles2}/bin/supertux-milestone1
            file ${pkgSdl2Gles2}/bin/supertux-milestone1 | grep -qi 'elf\|executable'
          '';
          # Win packages are flat: .exe + DLLs (+ data/) at the root.
          sanity-win64 = mkSanity "win64" win64Package ''
            test -f ${win64Package}/supertux-milestone1.exe \
              || ls ${win64Package}/*.exe >/dev/null
            file ${win64Package}/supertux-milestone1.exe 2>/dev/null \
              | grep -qi 'PE32+\|MS Windows' \
              || file ${win64Package}/*.exe | grep -qi 'PE32+\|MS Windows'
            ls ${win64Package}/*.dll >/dev/null
          '';
          sanity-win32 = mkSanity "win32" win32Package ''
            test -f ${win32Package}/supertux-milestone1.exe \
              || ls ${win32Package}/*.exe >/dev/null
            file ${win32Package}/supertux-milestone1.exe 2>/dev/null \
              | grep -qi 'PE32\|MS Windows' \
              || file ${win32Package}/*.exe | grep -qi 'PE32\|MS Windows'
            ls ${win32Package}/*.dll >/dev/null
          '';
        } // linuxExtras.checks
          // lib.optionalAttrs (!isWin) {
          # Android / wasm shape checks (packages live in linuxExtras).
          sanity-android = mkSanity "android" packages.supertux-milestone1-android ''
            apk=$(find ${packages.supertux-milestone1-android} -name '*.apk' | head -1)
            test -n "$apk"
            test -s "$apk"
            # APK is a zip; must start with PK.
            test "$(od -An -tx1 -N2 "$apk" | tr -d ' \n')" = "504b"
          '';
          sanity-wasm = mkSanity "wasm" packages.supertux-milestone1-wasm ''
            test -f ${packages.supertux-milestone1-wasm}/supertux-milestone1.html \
              || test -f ${packages.supertux-milestone1-wasm}/index.html \
              || ls ${packages.supertux-milestone1-wasm}/*.html >/dev/null
            ls ${packages.supertux-milestone1-wasm}/*.wasm >/dev/null
            ls ${packages.supertux-milestone1-wasm}/*.js >/dev/null
          '';
          sanity-gp2x = mkSanity "gp2x" packages.supertux-milestone1-gp2x ''
            bin=${packages.supertux-milestone1-gp2x}/bin/supertux-milestone1.gpe
            test -x "$bin"
            file "$bin" | grep -qi 'ARM\|arm'
            readelf -h "$bin" | grep -q 'soft'
          '';
          sanity-wiz = mkSanity "wiz" packages.supertux-milestone1-wiz ''
            bin=${packages.supertux-milestone1-wiz}/bin/supertux-milestone1.gpe
            test -x "$bin"
            file "$bin" | grep -qi 'ARM\|arm'
          '';
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
          supertux-milestone1-sdl2-debug = {
            type = "app";
            program = "${pkgSdl2Debug}/bin/supertux-milestone1";
            meta.description = "SuperTux Milestone 1 (SDL2, ENABLE_DEBUG)";
          };
        } // lib.optionalAttrs (!isWin) {
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
          supertux-milestone1-win32-x64 = mkWineApp win64Package
            "supertux-milestone1-win32-x64-wine"
            "SuperTux Milestone 1 (Win64 via Wine, temp WINEPREFIX)";
          supertux-milestone1-win32-x86 = mkWineApp win32Package
            "supertux-milestone1-win32-x86-wine"
            "SuperTux Milestone 1 (Win32 via Wine, temp WINEPREFIX)";
        } // linuxExtras.apps;
      }
    );
}
