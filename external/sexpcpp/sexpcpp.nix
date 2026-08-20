{ stdenv
, lib
, cmake
, gtest
}:

let
  versionFile = lib.fileContents ./VERSION;
  # Strip optional +g... suffix if present; keep -dev
  version = builtins.head (lib.splitString "+" versionFile);
in
stdenv.mkDerivation {
  pname = "sexp-cpp";
  inherit version;

  src = lib.cleanSource ./.;

  cmakeFlags = [
    "-DBUILD_TESTS=ON"
    "-DWARNINGS=ON"
    "-DWERROR=ON"
    "-DPROJECT_VERSION_FULL=${version}"
  ];

  doCheck = true;

  nativeBuildInputs = [
    cmake
  ];

  buildInputs = [
    gtest
  ];
}
