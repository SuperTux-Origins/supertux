{ self
, stdenv
, lib

, cmake
, pkg-config
, makeWrapper ? null
, libGL ? null

, SDL2
, SDL2_image
, SDL2_ttf

, glm
, libpng
, mcfgthreads ? null
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
, versionFull ? null
}:

stdenv.mkDerivation rec {
  pname = "supertux-origins";
  # Prefer flake-supplied version (VERSION + revCount + hash). Fallback reads
  # VERSION at build time via CMake if null.
  version =
    if versionFull != null then versionFull
    else lib.strings.removeSuffix "\n" (builtins.readFile ./VERSION);

  src = lib.cleanSource ./.;

  patchPhase = ''
    substituteInPlace config.h.cmake \
       --replace "#define _SQ64" ""
  '';

  strictDeps = true;

  cmakeFlags = [
    "-DINSTALL_SUBDIR_BIN=bin"
    "-DUSE_SYSTEM_SDL2_TTF=ON"
    "-DBUILD_TESTS=ON"
    "-DPROJECT_VERSION_FULL=${version}"
  ] ++
  lib.optional useGLES2 "-DENABLE_OPENGLES2=ON";

  # Transitive runtime DLLs (ogg, vorbis, opus, zlib, …) live under dependency
  # store paths, not only the top-level package bin/. Scan the buildInputs
  # closure so Wine/flat packages get a complete set.
  # Only runtime (target) deps — never nativeBuildInputs. Closing over cmake /
  # pkg-config / miniswig under hostPlatform=x86_64-windows tries to evaluate
  # Linux bash for the Windows host and fails flake check.
  windowsDllRoots = lib.optionals stdenv.hostPlatform.isWindows (
    lib.closePropagation (
      buildInputs
      ++ lib.optional (mcfgthreads != null) mcfgthreads
      ++ [ stdenv.cc.cc ]
    )
  );

  postFixup =
    (lib.optionalString stdenv.hostPlatform.isWindows ''
       # Materialize runtime DLLs into $out/bin for Wine / redistribution.
       # Nixpkgs may already link some; transitive audio deps (ogg.dll) are
       # often missing unless we scan the full buildInputs closure.
       mkdir -p $out/bin
       materialize_dll() {
         local src="$1"
         [ -f "$src" ] || return 0
         local base dest tmp
         base=$(basename "$src")
         dest="$out/bin/$base"
         tmp="$dest.tmp.$$"
         cp -L "$src" "$tmp"
         mv -f "$tmp" "$dest"
       }
       for f in "$out/bin"/*.dll; do
         [ -e "$f" ] || continue
         if [ -L "$f" ]; then
           materialize_dll "$f"
         fi
       done
       for root in ${lib.escapeShellArgs (map toString windowsDllRoots)}; do
         [ -d "$root" ] || continue
         # Prefer bin/ and lib/; skip huge unrelated trees when possible.
         find "$root" -type f -iname '*.dll' 2>/dev/null | while read -r src; do
           materialize_dll "$src"
         done
       done
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
  ++ lib.optional (libGL != null) libGL
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
