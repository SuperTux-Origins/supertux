# R36S / ArkOS (RK3326) cross builds.
#
# Primary flake package uses pkgsCross.aarch64-multiplatform (SDL2 + GLES2).
# That binary is for CI / qemu-aarch64 smoke tests. Stock ArkOS (Ubuntu 19.10
# glibc ~2.30) needs a matching sysroot — see mk/r36s/CROSSCOMPILE.md.
#
# Usage:
#   nix build .#supertux-milestone1-r36s
#
{ lib
, pkgsBuildHost
, pkgsCross
}:

let
  pkgsTarget = pkgsCross.aarch64-multiplatform;

  mkSuperTuxR36s = {
    src
  , version
  , pname ? "supertux-milestone1-r36s"
  , enableSound ? true
  }:
    pkgsTarget.stdenv.mkDerivation {
      inherit pname version src;

      nativeBuildInputs = with pkgsBuildHost; [
        cmake
        pkg-config
      ];

      buildInputs = with pkgsTarget; [
        SDL2
        SDL2_image
        zlib
        libpng
        libGL
      ] ++ lib.optionals enableSound [
        SDL2_mixer
      ];

      cmakeFlags = [
        "-DENABLE_SDL2=ON"
        "-DENABLE_GLES2=ON"
        "-DENABLE_OPENGL=ON"
        "-DENABLE_GP2X=OFF"
        "-DENABLE_RES320X240=OFF"
        "-DENABLE_SOUND=${if enableSound then "ON" else "OFF"}"
        "-DCMAKE_BUILD_TYPE=Release"
        "-DDATA_PREFIX=${placeholder "out"}/share/supertux-milestone1"
        "-DPROJECT_VERSION_FULL=${version}"
      ];

      # cmakeInstallPhase places the binary; we only add docs + launcher.
      # Inside ''...'' strings, literal shell ${...} must be written as ''${...}.
      postInstall = ''
        mkdir -p $out/share/supertux-milestone1
        if [ -d "$src/data" ]; then
          cp -a "$src/data/." $out/share/supertux-milestone1/ || true
        fi
        cat > $out/share/supertux-milestone1/README-R36S.txt << EOF_README
SuperTux Milestone 1 — R36S / aarch64 (nixpkgs cross)
=====================================================

Binary: bin/supertux-milestone1
  SDL2 + GLES2, logical 640×480 (matches R36S panel).

This package was linked against nixpkgs glibc / SDL2. Stock ArkOS
(Ubuntu 19.10, glibc ~2.30) will often refuse to start it:

  version `GLIBC_2.xx' not found

For a device-ready binary, cross-link against an ArkOS or Debian Buster
sysroot using:

  mk/r36s/CROSSCOMPILE.md
  mk/r36s/toolchain-arkos-aarch64.cmake

qemu-aarch64 can still smoke-test this build on the host.
EOF_README
        cat > $out/share/supertux-milestone1/supertux-milestone1.sh << 'LAUNCH'
#!/bin/bash
DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$DIR"
# Optional private libs: export LD_LIBRARY_PATH="$DIR/libs:$LD_LIBRARY_PATH"
BIN="$DIR/../bin/supertux-milestone1"
if [ ! -x "$BIN" ]; then BIN="$DIR/supertux-milestone1"; fi
exec "$BIN" --fullscreen "$@"
LAUNCH
        chmod +x $out/share/supertux-milestone1/supertux-milestone1.sh
      '';

      meta = with lib; {
        description = "SuperTux Milestone 1 (R36S / aarch64, SDL2+GLES2)";
        license = licenses.gpl3Plus;
        platforms = platforms.linux;
        hydraPlatforms = [];
      };
    };

in
{
  inherit mkSuperTuxR36s;
}
