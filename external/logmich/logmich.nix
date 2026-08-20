{ stdenv
, lib
, cmake
}:

stdenv.mkDerivation {
  pname = "logmich";
  version = "0.2.0-dev";

  src = lib.cleanSource ./.;

  nativeBuildInputs = [
    cmake
  ];
}
