{ self
, stdenv
, lib

, cmake
, pkg-config
, makeWrapper
, libGL ? null
, libsm ? null
, libice ? null

, SDL2
, SDL2_image
, SDL2_ttf

, freetype

, glew ? null
, glm
, libpng
, mcfgthreads
, mesa
, physfs
, logmich
, sexpcpp
, squirrel ? null
, tinycmmc
, strutcpp
, miniswig
, wstsound
, priocpp
, xdgcpp
, gtest

, useGLES2 ? false
}:

stdenv.mkDerivation rec {
  pname = "supertux-origins";
  # FIXME: Should use `git describe` to get the version
  # number or leave it to cmake, but the .git/ directory
  # isn't included in the Nix store.
  version = "0.6.3-${lib.substring 0 8 self.lastModifiedDate}-${self.shortRev or "dirty"}";

  src = lib.cleanSource ./.;

  patchPhase = let
    ver = builtins.splitVersion version;
  in ''
    substituteInPlace config.h.cmake \
       --replace "#define _SQ64" ""

     cat > version.cmake <<EOF
SET(SUPERTUX_VERSION_MAJOR ${builtins.elemAt ver 0})
SET(SUPERTUX_VERSION_MINOR ${builtins.elemAt ver 1})
SET(SUPERTUX_VERSION_PATCH ${builtins.elemAt ver 2})
SET(SUPERTUX_VERSION_TWEAK ${builtins.elemAt ver 3})
SET(SUPERTUX_VERSION_STRING "v${version}")
SET(SUPERTUX_VERSION_BUILD "${builtins.elemAt ver 4}")
EOF
  '';

  cmakeFlags = [
    "-DINSTALL_SUBDIR_BIN=bin"
    "-DUSE_SYSTEM_SDL2_TTF=ON"
    "-DBUILD_TESTS=ON"
  ] ++
  lib.optional useGLES2 "-DENABLE_OPENGLES2=ON";

  postFixup =
    (lib.optionalString stdenv.hostPlatform.isWindows ''
       mkdir -p $out/bin/
       find ${mcfgthreads} -iname "*.dll" -exec ln -sfv {} $out/bin/ \;
       find ${stdenv.cc.cc} -iname "*.dll" -exec ln -sfv {} $out/bin/ \;
       ln -sfv ${SDL2_image}/bin/*.dll $out/bin/
       ln -sfv ${SDL2_ttf}/bin/*.dll $out/bin/
       ln -sfv ${SDL2}/bin/*.dll $out/bin/
       ln -sfv ${physfs}/bin/*.dll $out/bin/
       ${lib.optionalString (squirrel != null) "ln -sfv ${squirrel}/bin/*.dll $out/bin/"}
       ln -sfv ${strutcpp}/bin/*.dll $out/bin/
       ln -sfv ${wstsound}/bin/*.dll $out/bin/
    '')
    + (lib.optionalString stdenv.hostPlatform.isLinux ''
       # The game only uses SDL. Under pure Nix, the dynamic linker still has to
       # resolve libraries *SDL2* is linked against (X11 backend). DT_RUNPATH on
       # our binary does not help transitive deps, so expose the usual SDL2
       # closure via LD_LIBRARY_PATH. This is not raw X11 usage by SuperTux.
       wrapProgram $out/bin/supertux-origins \
         --prefix LD_LIBRARY_PATH : ${lib.makeLibraryPath (
           [ SDL2 SDL2_image SDL2_ttf physfs ]
           ++ lib.optional (libGL != null) libGL
           ++ lib.optional (libsm != null) libsm
           ++ lib.optional (libice != null) libice
           ++ lib.optional (xdgcpp != null) xdgcpp
         )}
    '');


  nativeBuildInputs = [
    cmake
    pkg-config

    miniswig
  ]
  ++ (lib.optional stdenv.hostPlatform.isLinux makeWrapper);

  buildInputs = [
    SDL2
    SDL2_image
    SDL2_ttf
    glm
    libpng
    physfs
    logmich
    sexpcpp
    tinycmmc
    strutcpp
    wstsound
    priocpp
    # checkInputs
    gtest
  ]
  ++ lib.optional (squirrel != null) squirrel
  ++ (lib.optional (!stdenv.hostPlatform.isWindows) xdgcpp);

  meta = with lib; {
    description = "SuperTux (Origins) — 2D platform game";
    homepage = "https://github.com/SuperTux-Origins/supertux";
    license = licenses.gpl3Plus;
    platforms = if stdenv.hostPlatform.isWindows
                then [ "x86_64-windows" ]
                else platforms.linux;
    mainProgram = "supertux-origins";
  };
}
