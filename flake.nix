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
    nixpkgs.url = "github:NixOS/nixpkgs?ref=nixos-unstable";

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
  };

  outputs = { self, nixpkgs, tinycmmc, SDL-win32, SDL_mixer-win32, SDL_image-win32 }:
    tinycmmc.lib.eachSystemWithPkgs (pkgs:
      let
        # Shared CMake builder. Binary is always named supertux-milestone1;
        # pname differs per backend package. meta.mainProgram must match the
        # on-disk binary for `nix run`.
        mkSuperTux = { useSDL2 ? true, pname ? "supertux-milestone1" }:
          pkgs.stdenv.mkDerivation rec {
            inherit pname;
            version = "0.1.4";

            src = nixpkgs.lib.cleanSource ./.;

            enableParallelBuilding = true;

            nativeBuildInputs = [
              pkgs.buildPackages.cmake
              pkgs.buildPackages.pkg-config
            ]
            ++ (nixpkgs.lib.optionals pkgs.stdenv.targetPlatform.isLinux [
              pkgs.makeWrapper
              # So the binary can dlopen the host OpenGL driver (NixOS).
              pkgs.addDriverRunpath
            ]);

            cmakeFlags = [
              "-DENABLE_SOUND=ON"
              "-DENABLE_OPENGL=ON"
              "-DENABLE_SDL2=${if useSDL2 then "ON" else "OFF"}"
              "-DDATA_PREFIX=${placeholder "out"}/share/supertux-milestone1"
            ];

            postFixup = ""
              + (nixpkgs.lib.optionalString (pkgs.stdenv.targetPlatform.isLinux && useSDL2) ''
                   addDriverRunpath $out/bin/supertux-milestone1
                   # Host GPU driver + libGL (NixOS OpenGL is impure outside the sandbox).
                   wrapProgram $out/bin/supertux-milestone1 \
                     --prefix LD_LIBRARY_PATH : "/run/opengl-driver/lib:/run/opengl-driver-32/lib"
                 '')
              + (nixpkgs.lib.optionalString pkgs.stdenv.targetPlatform.isWindows ''
                   mkdir -p $out/bin/
                   find ${pkgs.windows.mcfgthreads} -iname "*.dll" -exec ln -sfv {} $out/bin/ \;
                   find ${pkgs.stdenv.cc.cc} -iname "*.dll" -exec ln -sfv {} $out/bin/ \;
                   ${if useSDL2 then "" else ''
                   ln -sfv ${SDL-win32.packages.${pkgs.system}.default}/bin/*.dll $out/bin/
                   ln -sfv ${SDL_image-win32.packages.${pkgs.system}.default}/bin/*.dll $out/bin/
                   ln -sfv ${SDL_mixer-win32.packages.${pkgs.system}.default}/bin/*.dll $out/bin/
                   ''}
                 '');

            buildInputs =
              (if pkgs.stdenv.targetPlatform.isWindows && !useSDL2
               then [
                 SDL-win32.packages.${pkgs.system}.default
                 SDL_image-win32.packages.${pkgs.system}.default
                 SDL_mixer-win32.packages.${pkgs.system}.default
               ]
               else if useSDL2
               then [
                 pkgs.SDL2
                 pkgs.SDL2_image
                 pkgs.SDL2_mixer
                 pkgs.libGL
                 pkgs.libGLU
               ]
               else [
                 pkgs.SDL
                 pkgs.SDL_image
                 pkgs.SDL_mixer
                 pkgs.libGL
                 pkgs.libGLU
               ]) ++
              [
                pkgs.zlib
                pkgs.libpng
                pkgs.libjpeg
              ];

            meta = with nixpkgs.lib; {
              description = "SuperTux Milestone 1 (${if useSDL2 then "SDL2" else "SDL 1.2"} backend)";
              longDescription = ''
                Classic SuperTux Milestone 1 engine with a CMake build and a
                dual SDL1/SDL2 platform layer. Default builds use SDL2.
              '';
              license = licenses.gpl2Plus;
              platforms = platforms.linux ++ platforms.windows;
              # CMake always installs bin/supertux-milestone1 regardless of pname.
              mainProgram = "supertux-milestone1";
            };
          };

        pkgSdl2 = mkSuperTux {
          useSDL2 = true;
          pname = "supertux-milestone1-sdl2";
        };

        pkgSdl1 = mkSuperTux {
          useSDL2 = false;
          pname = "supertux-milestone1-sdl1";
        };
      in
      {
        packages = rec {
          # Default package = SDL2 backend
          default = supertux-milestone1;
          supertux-milestone1 = supertux-milestone1-sdl2;

          supertux-milestone1-sdl2 = pkgSdl2;
          supertux-milestone1-sdl1 = pkgSdl1;

          supertux-milestone1-win32 = pkgs.runCommand "supertux-milestone1-win32" {} ''
            mkdir -p $out
            mkdir -p $out/data/

            cp -vr ${supertux-milestone1-sdl1}/bin/supertux-milestone1.exe $out/ 2>/dev/null || \
              cp -vr ${supertux-milestone1-sdl1}/bin/supertux-milestone1 $out/
            cp -vLr ${supertux-milestone1-sdl1}/bin/*.dll $out/ 2>/dev/null || true
            if [ -d ${supertux-milestone1-sdl1}/share/supertux-milestone1 ]; then
              cp -vr ${supertux-milestone1-sdl1}/share/supertux-milestone1/. $out/data/
            fi
          '';

          supertux-milestone1-win32-zip = pkgs.runCommand "supertux-milestone1-win32-zip" {} ''
            mkdir -p $out
            WORKDIR=$(mktemp -d)

            cp --no-preserve mode,ownership --verbose --recursive \
              ${supertux-milestone1-win32}/. "$WORKDIR"

            cd "$WORKDIR"
            ${nixpkgs.legacyPackages.x86_64-linux.zip}/bin/zip \
              -r \
              $out/supertux-milestone1-${pkgSdl1.version}-${pkgs.system}.zip \
              .
          '';
        };

        # `nix run .#…` entry points (program path must be the real binary name)
        apps = {
          default = {
            type = "app";
            program = "${pkgSdl2}/bin/supertux-milestone1";
            meta = {
              description = "SuperTux Milestone 1 (SDL2 backend, default)";
            };
          };
          supertux-milestone1 = {
            type = "app";
            program = "${pkgSdl2}/bin/supertux-milestone1";
            meta = {
              description = "SuperTux Milestone 1 (SDL2 backend, default)";
            };
          };
          supertux-milestone1-sdl2 = {
            type = "app";
            program = "${pkgSdl2}/bin/supertux-milestone1";
            meta = {
              description = "SuperTux Milestone 1 with the SDL2 platform backend";
            };
          };
          supertux-milestone1-sdl1 = {
            type = "app";
            program = "${pkgSdl1}/bin/supertux-milestone1";
            meta = {
              description = "SuperTux Milestone 1 with the legacy SDL 1.2 backend";
            };
          };
        };

        # Dev shells for each backend (default = SDL2)
        devShells = {
          default = pkgs.mkShell {
            name = "supertux-m1-sdl2";
            packages = [
              pkgs.cmake
              pkgs.pkg-config
              pkgs.SDL2
              pkgs.SDL2_image
              pkgs.SDL2_mixer
              pkgs.libGL
              pkgs.zlib
              pkgs.libpng
              pkgs.libjpeg
            ];
          };
          sdl2 = pkgs.mkShell {
            name = "supertux-m1-sdl2";
            packages = [
              pkgs.cmake
              pkgs.pkg-config
              pkgs.SDL2
              pkgs.SDL2_image
              pkgs.SDL2_mixer
              pkgs.libGL
              pkgs.zlib
              pkgs.libpng
              pkgs.libjpeg
            ];
          };
          sdl1 = pkgs.mkShell {
            name = "supertux-m1-sdl1";
            packages = [
              pkgs.cmake
              pkgs.pkg-config
              pkgs.SDL
              pkgs.SDL_image
              pkgs.SDL_mixer
              pkgs.libGL
              pkgs.zlib
              pkgs.libpng
              pkgs.libjpeg
            ];
          };
        };
      }
    );
}
