{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs?ref=nixos-unstable";

    tinycmmc.url = "github:grumbel/tinycmmc";
    tinycmmc.inputs.nixpkgs.follows = "nixpkgs";

    squirrel_src.url = "github:albertodemichelis/squirrel?rev=f77074bdd6152d230609146a3d424c6f49e3770f";
    squirrel_src.flake = false;
  };

  outputs = { self, nixpkgs, tinycmmc, squirrel_src }:
    tinycmmc.lib.eachSystemWithPkgs (pkgs:
      rec {
        packages = rec {
          default = squirrel;

          squirrel = pkgs.stdenv.mkDerivation {
            pname = "squirrel";
            version = "3.2";

            src = squirrel_src;

            nativeBuildInputs = [
              pkgs.buildPackages.cmake
            ];
          };
        };

        apps = rec {
          default = sq;

          sq = {
            type = "app";
            program = "${packages.squirrel}/bin/sq";
          };
        };
      }
    );
}
