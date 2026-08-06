# R36S / ArkOS (RK3326) cross builds against a published ArkOS sysroot.
#
# The sysroot tarball supplies aarch64 headers + shared libs (glibc ~2.30 era,
# SDL2, GLES, …) so the binary can run on stock ArkOS — unlike pkgsCross
# alone, which links modern nixpkgs glibc/Mesa.
#
# Usage:
#   nix build .#arkos-sysroot
#   nix build .#supertux-milestone1-r36s
#
# If the URL hash is wrong, Nix prints the expected hash on first fetch.
#
{ lib
, stdenv
, stdenvNoCC
, fetchurl
, cmake
, pkg-config
, pkgsCross
}:

let
  # ---------------------------------------------------------------------------
  # Sysroot (device-compatible headers + .so)
  # ---------------------------------------------------------------------------
  arkosSysrootSrc = fetchurl {
    name = "arkos-sysroot.tar.gz";
    url = "http:///localhost:8888/arkos-sysroot2.tar.gz";
    # Replace after:  nix store prefetch-file https://github.com/grumnix/arkos-sysroot.tar.gz
    # (or let `nix build .#arkos-sysroot` print the correct hash).
    hash = "sha256-nIlMQ3P0uBrRQ9/k2x1s9DpdnF8iqA2wBLSB/20uXYg=";
  };

  arkosSysroot = stdenvNoCC.mkDerivation {
    pname = "arkos-sysroot";
    version = "0.1";
    src = arkosSysrootSrc;

    dontConfigure = true;
    dontBuild = true;

    # Foreign aarch64 rootfs: do not patchelf / strip / rewrite shebangs, and
    # allow dangling multiarch/soname symlinks typical of a partial sysroot.
    dontPatchELF = true;
    dontStrip = true;
    dontPatchShebangs = true;
    dontCheckForBrokenSymlinks = true;

    installPhase = ''
      runHook preInstall
      mkdir -p "$out"

      # Accept common layouts:
      #   ./usr/...
      #   ./sysroot/usr/...
      #   ./<top>/usr/...
      if [ -d usr ]; then
        cp -a . "$out/"
      elif [ -d sysroot/usr ]; then
        cp -a sysroot/. "$out/"
      else
        top=
        for d in *; do
          if [ -d "$d/usr" ]; then top="$d"; break; fi
        done
        if [ -z "$top" ]; then
          echo "arkos-sysroot: unrecognized tarball layout (no usr/):" >&2
          find . -maxdepth 3 -type d >&2 || true
          exit 1
        fi
        cp -a "$top"/. "$out/"
      fi

      test -d "$out/usr" || {
        echo "arkos-sysroot: missing $out/usr after install" >&2
        exit 1
      }

      # Convenience symlink used by docs / scripts
      ln -sfn . "$out/sysroot"

      # Record layout for debugging
      {
        echo "arkos-sysroot unpacked for SuperTux Milestone 1"
        echo "source=${arkosSysrootSrc}"
        ls -la "$out" | head -20
        ls "$out/usr/lib/aarch64-linux-gnu" 2>/dev/null | head -5 || true
      } > "$out/SYSROOT.txt"

      runHook postInstall
    '';

    meta = with lib; {
      description = "ArkOS / R36S aarch64 sysroot (glibc + SDL2 + GLES)";
      license = licenses.free;
      platforms = platforms.linux;
      hydraPlatforms = [];
    };
  };

  # Cross toolchain from nixpkgs (compiler only — libs come from the sysroot).
  crossPkgs = pkgsCross.aarch64-multiplatform;
  crossCc = crossPkgs.stdenv.cc;
  targetPrefix = crossCc.targetPrefix; # e.g. aarch64-unknown-linux-gnu-

  # ---------------------------------------------------------------------------
  # Game binary linked against the ArkOS sysroot
  # ---------------------------------------------------------------------------
  mkSuperTuxR36s = {
    src
  , version
  , pname ? "supertux-milestone1-r36s"
  , enableSound ? true
  }:
    stdenv.mkDerivation {
      inherit pname version src;

      nativeBuildInputs = [
        cmake
        pkg-config
        crossCc
      ];

      # No nixpkgs SDL/Mesa on the link line — only the sysroot.
      strictDeps = true;

      cmakeFlags = [
        "-DCMAKE_SYSTEM_NAME=Linux"
        "-DCMAKE_SYSTEM_PROCESSOR=aarch64"
        "-DCMAKE_SYSROOT=${arkosSysroot}"
        "-DCMAKE_FIND_ROOT_PATH=${arkosSysroot}"
        "-DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER"
        "-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY"
        "-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY"
        "-DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY"
        "-DCMAKE_C_COMPILER=${crossCc}/bin/${targetPrefix}gcc"
        "-DCMAKE_CXX_COMPILER=${crossCc}/bin/${targetPrefix}g++"
        "-DCMAKE_C_FLAGS_INIT=-march=armv8-a -mtune=cortex-a35"
        "-DCMAKE_CXX_FLAGS_INIT=-march=armv8-a -mtune=cortex-a35"
        # Prefer the sysroot libstdc++ ABI when possible; still link via cross gcc.
        "-DCMAKE_EXE_LINKER_FLAGS_INIT=--sysroot=${arkosSysroot}"
        "-DENABLE_SDL2=ON"
        "-DENABLE_GLES2=ON"
        "-DENABLE_OPENGL=ON"
        "-DENABLE_GP2X=OFF"
        "-DENABLE_RES320X240=OFF"
        "-DENABLE_SOUND=${if enableSound then "ON" else "OFF"}"
        "-DCMAKE_BUILD_TYPE=Release"
        "-DDATA_PREFIX=${placeholder "out"}/share/supertux-milestone1"
        "-DPROJECT_VERSION_FULL=${version}"
        "-DARKOS_SYSROOT=${arkosSysroot}"
      ];

      preConfigure = ''
        export PKG_CONFIG="pkg-config"
        export PKG_CONFIG_SYSROOT_DIR="${arkosSysroot}"
        export PKG_CONFIG_DIR=""
        export PKG_CONFIG_PATH=""
        export PKG_CONFIG_LIBDIR="${arkosSysroot}/usr/lib/aarch64-linux-gnu/pkgconfig:${arkosSysroot}/usr/lib/pkgconfig:${arkosSysroot}/usr/share/pkgconfig"
        echo "PKG_CONFIG_SYSROOT_DIR=$PKG_CONFIG_SYSROOT_DIR"
        echo "PKG_CONFIG_LIBDIR=$PKG_CONFIG_LIBDIR"
        pkg-config --exists sdl2 && pkg-config --modversion sdl2 || true
      '';

      # cmakeInstallPhase installs the binary; add README + PortMaster-style launcher.
      postInstall = ''
        mkdir -p $out/share/supertux-milestone1
        if [ -d "$src/data" ]; then
          cp -a "$src/data/." $out/share/supertux-milestone1/ || true
        fi
        cat > $out/share/supertux-milestone1/README-R36S.txt << EOF_README
SuperTux Milestone 1 — R36S / ArkOS (sysroot-linked)
====================================================

Binary: bin/supertux-milestone1
  SDL2 + GLES2, logical 640×480, linked against the ArkOS aarch64 sysroot
  (https://github.com/grumnix/arkos-sysroot.tar.gz).

Deploy:
  1. Copy bin/supertux-milestone1 and share/supertux-milestone1/ (data) to the device
  2. Ensure runtime libs exist on ArkOS (SDL2, GLES/EGL, libpng, …)
  3. Run: ./supertux-milestone1 --fullscreen -v

If ldd reports missing .so files (e.g. libopusfile), install the matching
packages on the device or ship them under libs/ + LD_LIBRARY_PATH.
EOF_README
        cat > $out/share/supertux-milestone1/supertux-milestone1.sh << 'LAUNCH'
#!/bin/bash
DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$DIR"
# export LD_LIBRARY_PATH="$DIR/libs:$LD_LIBRARY_PATH"
BIN="$DIR/../bin/supertux-milestone1"
if [ ! -x "$BIN" ]; then BIN="$DIR/supertux-milestone1"; fi
exec "$BIN" --fullscreen "$@"
LAUNCH
        chmod +x $out/share/supertux-milestone1/supertux-milestone1.sh
      '';

      meta = with lib; {
        description = "SuperTux Milestone 1 for R36S/ArkOS (sysroot-linked aarch64)";
        license = licenses.gpl3Plus;
        platforms = platforms.linux;
        hydraPlatforms = [];
      };
    };

in
{
  inherit arkosSysroot mkSuperTuxR36s;
}
