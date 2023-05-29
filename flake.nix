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
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-23.05";
    flake-utils.url = "github:numtide/flake-utils";

    tinycmmc.url = "github:grumbel/tinycmmc";
    tinycmmc.inputs.nixpkgs.follows = "nixpkgs";
    tinycmmc.inputs.flake-utils.follows = "flake-utils";

    sexpcpp.url = "github:lispparser/sexp-cpp";
    sexpcpp.inputs.nixpkgs.follows = "nixpkgs";
    sexpcpp.inputs.flake-utils.follows = "flake-utils";
    sexpcpp.inputs.tinycmmc.follows = "tinycmmc";

    logmich.url = "github:logmich/logmich";
    logmich.inputs.nixpkgs.follows = "nixpkgs";
    logmich.inputs.tinycmmc.follows = "tinycmmc";

    curl-win32.url = "github:grumnix/curl-win32";
    curl-win32.inputs.nixpkgs.follows = "nixpkgs";
    curl-win32.inputs.tinycmmc.follows = "tinycmmc";

    physfs-win32.url = "github:grumnix/physfs-win32";
    physfs-win32.inputs.nixpkgs.follows = "nixpkgs";
    physfs-win32.inputs.tinycmmc.follows = "tinycmmc";

    SDL2-win32.url = "github:grumnix/SDL2-win32";
    SDL2-win32.inputs.nixpkgs.follows = "nixpkgs";
    SDL2-win32.inputs.tinycmmc.follows = "tinycmmc";

    SDL2_image-win32.url = "github:grumnix/SDL2_image-win32";
    SDL2_image-win32.inputs.nixpkgs.follows = "nixpkgs";
    SDL2_image-win32.inputs.tinycmmc.follows = "tinycmmc";

    freetype-win32.url = "github:grumnix/freetype-win32";
    freetype-win32.inputs.nixpkgs.follows = "nixpkgs";
    freetype-win32.inputs.tinycmmc.follows = "tinycmmc";

    SDL2_ttf-win32.url = "github:grumnix/SDL2_ttf-win32";
    SDL2_ttf-win32.inputs.nixpkgs.follows = "nixpkgs";
    SDL2_ttf-win32.inputs.tinycmmc.follows = "tinycmmc";

    strutcpp.url = "github:grumbel/strutcpp";
    strutcpp.inputs.nixpkgs.follows = "nixpkgs";
    strutcpp.inputs.tinycmmc.follows = "tinycmmc";

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

    priocpp.url = "github:grumbel/priocpp";
    priocpp.inputs.nixpkgs.follows = "nixpkgs";
    priocpp.inputs.flake-utils.follows = "flake-utils";
    priocpp.inputs.tinycmmc.follows = "tinycmmc";
    priocpp.inputs.logmich.follows = "logmich";
    priocpp.inputs.sexpcpp.follows = "sexpcpp";
  };

  outputs = { self, nixpkgs, flake-utils,
              tinycmmc, sexpcpp, curl-win32, logmich,
              SDL2-win32, SDL2_image-win32, freetype-win32, physfs-win32, SDL2_ttf-win32,
              strutcpp, miniswig, xdgcpp, wstsound, squirrel, glew-win32, priocpp }:

    tinycmmc.lib.eachSystemWithPkgs (pkgs:
      {
        packages = rec {
          default = supertux-origins;

          supertux-origins = pkgs.callPackage ./supertux-origins.nix {
            inherit self;

            stdenv = pkgs.gcc12Stdenv;

            SDL2_ttf = if pkgs.targetPlatform.isWindows
                       then SDL2_ttf-win32.packages.${pkgs.system}.default
                       else pkgs.SDL2_ttf;

            sexpcpp = sexpcpp.packages.${pkgs.system}.default;
            squirrel = squirrel.packages.${pkgs.system}.default;
            tinycmmc = tinycmmc.packages.${pkgs.system}.default;
            strutcpp = strutcpp.packages.${pkgs.system}.default;
            miniswig = miniswig.packages.${pkgs.system}.default;
            wstsound = wstsound.packages.${pkgs.system}.default;
            priocpp = priocpp.packages.${pkgs.system}.priocpp-sexp;
            logmich = logmich.packages.${pkgs.system}.default;

            physfs = if pkgs.targetPlatform.isWindows
                     then physfs-win32.packages.${pkgs.system}.default
                     else pkgs.physfs;

            curl = if pkgs.targetPlatform.isWindows
                   then curl-win32.packages.${pkgs.system}.default
                   else pkgs.curl;

            glew = if pkgs.targetPlatform.isWindows
                   then glew-win32.packages.${pkgs.system}.default
                   else pkgs.glew;

            glm = (pkgs.glm.overrideAttrs (oldAttrs: { meta = {}; }));

            SDL2 = if pkgs.targetPlatform.isWindows
                   then SDL2-win32.packages.${pkgs.system}.default
                   else pkgs.SDL2;

            SDL2_image = if pkgs.targetPlatform.isWindows
                         then SDL2_image-win32.packages.${pkgs.system}.default
                         else pkgs.SDL2_image;

            xdgcpp = if !pkgs.targetPlatform.isWindows
                     then xdgcpp.packages.${pkgs.system}.default
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
              $out/SuperTux-${supertux-origins.version}-${pkgs.system}.zip \
              .
          '';
        };
      }
    );
}
