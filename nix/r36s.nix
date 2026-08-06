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
{ lib
, stdenv
, stdenvNoCC
, fetchurl
, cmake
, pkg-config
, pkgsCross
, writeShellScript
}:

let
  arkosSysrootSrc = fetchurl {
    name = "arkos-sysroot.tar.gz";
    url = "http:///localhost:8888/arkos-sysroot2.tar.gz";
    # Replace after:  nix store prefetch-file https://github.com/grumnix/arkos-sysroot.tar.gz
    # (or let `nix build .#arkos-sysroot` print the correct hash).
    hash = "sha256-nIlMQ3P0uBrRQ9/k2x1s9DpdnF8iqA2wBLSB/20uXYg=";
  };

  # Allow hash to be overridden by the user who already fetched the tarball;
  # if the placeholder remains, Nix will print the expected hash.
  khrplatformH = ../mk/r36s/include/KHR/khrplatform.h;

  arkosSysroot = stdenvNoCC.mkDerivation {
    pname = "arkos-sysroot";
    version = "0.1";
    src = arkosSysrootSrc;

    dontConfigure = true;
    dontBuild = true;
    dontPatchELF = true;
    dontStrip = true;
    dontPatchShebangs = true;
    dontCheckForBrokenSymlinks = true;

    installPhase = ''
      runHook preInstall
      mkdir -p "$out"

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

      for base in "$out/usr/include" "$out/usr/lib" "$out/lib"; do
        if [ -d "$base/aarch64-linux-gnu" ] && [ ! -e "$base/aarch64-unknown-linux-gnu" ]; then
          ln -sfn aarch64-linux-gnu "$base/aarch64-unknown-linux-gnu"
        fi
      done

      mkdir -p "$out/usr/include/KHR"
      cp -f ${khrplatformH} "$out/usr/include/KHR/khrplatform.h"

      # Debian libc.so linker scripts embed absolute /lib/... paths. Rewrite
      # ONLY the multiarch absolute prefixes (not a bare "/lib/" which would
      # re-match inside /nix/store/.../lib/... and double the path).
      find "$out" -type f \( -name 'libc.so' -o -name 'libpthread.so' -o -name 'libm.so' -o -name 'libdl.so' -o -name 'librt.so' -o -name 'libutil.so' -o -name 'libresolv.so' -o -name 'libanl.so' -o -name 'libBrokenLocale.so' -o -name 'libthread_db.so' \) 2>/dev/null | while read -r f; do
        if grep -qE 'GROUP|INPUT' "$f" 2>/dev/null; then
          echo "patching linker script $f"
          # Match only when the path starts at a token boundary (space, (, =).
          sed -i -E \
            -e "s#(^|[[:space:](=])/usr/lib/aarch64-linux-gnu/#\1$out/usr/lib/aarch64-linux-gnu/#g" \
            -e "s#(^|[[:space:](=])/lib/aarch64-linux-gnu/#\1$out/lib/aarch64-linux-gnu/#g" \
            "$f" || true
          case "$f" in *libc.so) echo "---- $f ----"; cat "$f"; echo "--------";; esac
        fi
      done

      ln -sfn . "$out/sysroot"
      echo "arkos-sysroot ready" > "$out/SYSROOT.txt"
      runHook postInstall
    '';

    meta = with lib; {
      description = "ArkOS / R36S aarch64 sysroot (glibc + SDL2 + GLES)";
      license = licenses.free;
      platforms = platforms.linux;
      hydraPlatforms = [];
    };
  };

  crossPkgs = pkgsCross.aarch64-multiplatform;
  crossCc = crossPkgs.stdenv.cc;
  targetPrefix = crossCc.targetPrefix;

  # Wrappers inject -nostdinc + ordered isystem so:
  #   1) libstdc++ (from nixpkgs gcc)
  #   2) gcc fixed headers (stddef.h)
  #   3) ArkOS glibc headers only (never gcc's modern sys-include)
  # That avoids __attr_dealloc_free errors from mixing glibc 2.30 cdefs with
  # modern stdlib.h, and keeps #include_next <stdlib.h> working.
  mkWrappers = sysroot: let
    gcc = crossCc.cc;
    tp = lib.removeSuffix "-" targetPrefix; # aarch64-unknown-linux-gnu
    libdir = "${sysroot}/usr/lib/aarch64-linux-gnu";
    # libstdc++ lives under the gcc package; versioned path.
    cxxInc = "${gcc}/include/c++/${gcc.version}";
    cxxIncTarget = "${cxxInc}/${tp}";
    fixedInc = "${gcc}/lib/gcc/${tp}/${gcc.version}/include";
    fixedInc2 = "${gcc}/lib/gcc/${tp}/${gcc.version}/include-fixed";
    libgccDir = "${gcc}/lib/gcc/${tp}/${gcc.version}";
    commonC = ''
      -nostdinc \
      -isystem ${fixedInc} \
      -isystem ${fixedInc2} \
      -isystem ${sysroot}/usr/include/aarch64-linux-gnu \
      -isystem ${sysroot}/usr/include \
      --sysroot=${sysroot} \
      -Wl,--sysroot=${sysroot} \
      -B${libdir} \
      -B${libgccDir} \
      -L${libdir} \
      -L${libgccDir} \
      -L${sysroot}/usr/lib \
      -L${sysroot}/lib \
      -L${sysroot}/lib/aarch64-linux-gnu \
      -pthread \
      -Wl,-rpath-link,${libdir} \
      -Wl,-rpath-link,${sysroot}/usr/lib/aarch64-linux-gnu \
      -Wl,-rpath-link,${sysroot}/lib/aarch64-linux-gnu \
      -Wl,-rpath-link,${sysroot}/usr/lib/aarch64-linux-gnu/pulseaudio \
      -Wl,-rpath-link,${sysroot}/lib/aarch64-linux-gnu/pulseaudio \
      -Wl,--as-needed \
      -march=armv8-a \
      -mtune=cortex-a35 \
    '';
    commonCxx = ''
      -nostdinc \
      -isystem ${cxxInc} \
      -isystem ${cxxIncTarget} \
      -isystem ${cxxInc}/backward \
      -isystem ${fixedInc} \
      -isystem ${fixedInc2} \
      -isystem ${sysroot}/usr/include/aarch64-linux-gnu \
      -isystem ${sysroot}/usr/include \
      --sysroot=${sysroot} \
      -Wl,--sysroot=${sysroot} \
      -B${libdir} \
      -B${libgccDir} \
      -L${libdir} \
      -L${libgccDir} \
      -L${sysroot}/usr/lib \
      -L${sysroot}/lib \
      -L${sysroot}/lib/aarch64-linux-gnu \
      -pthread \
      -Wl,-rpath-link,${libdir} \
      -Wl,-rpath-link,${sysroot}/usr/lib/aarch64-linux-gnu \
      -Wl,-rpath-link,${sysroot}/lib/aarch64-linux-gnu \
      -Wl,-rpath-link,${sysroot}/usr/lib/aarch64-linux-gnu/pulseaudio \
      -Wl,-rpath-link,${sysroot}/lib/aarch64-linux-gnu/pulseaudio \
      -Wl,--as-needed \
      -march=armv8-a \
      -mtune=cortex-a35 \
    '';
    bintoolsBin = "${crossCc.bintools}/bin";
  in {
    cc = writeShellScript "aarch64-arkos-gcc" ''
      export PATH="${crossCc.bintools}/bin:$PATH"
      exec ${gcc}/bin/${targetPrefix}gcc \
        -B${crossCc.bintools}/bin \
        ${commonC} \
        "$@"
    '';
    cxx = writeShellScript "aarch64-arkos-g++" ''
      export PATH="${crossCc.bintools}/bin:$PATH"
      exec ${gcc}/bin/${targetPrefix}g++ \
        -B${crossCc.bintools}/bin \
        ${commonCxx} \
        "$@"
    '';
  };

  mkSuperTuxR36s = {
    src
  , version
  , pname ? "supertux-milestone1-r36s"
  , enableSound ? true
  }:
    let
      wrappers = mkWrappers arkosSysroot;
    in
    stdenv.mkDerivation {
      inherit pname version src;

      nativeBuildInputs = [
        cmake
        pkg-config
        crossCc.bintools
      ];

      strictDeps = true;

      # Avoid host cmakeDefaults forcing the wrong compilers after our flags.
      cmakeFlags = [
        "-DCMAKE_SYSTEM_NAME=Linux"
        "-DCMAKE_SYSTEM_PROCESSOR=aarch64"
        "-DCMAKE_SYSROOT=${arkosSysroot}"
        "-DCMAKE_FIND_ROOT_PATH=${arkosSysroot}"
        "-DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER"
        "-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY"
        "-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY"
        "-DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY"
        "-DCMAKE_C_COMPILER=${wrappers.cc}"
        "-DCMAKE_CXX_COMPILER=${wrappers.cxx}"
        "-DCMAKE_C_COMPILER_WORKS=1"
        "-DCMAKE_CXX_COMPILER_WORKS=1"
        "-DCMAKE_C_COMPILER_FORCED=TRUE"
        "-DCMAKE_CXX_COMPILER_FORCED=TRUE"
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
        pkg-config --exists sdl2 && pkg-config --modversion sdl2 || true

        ZLIB_LIB=
        for cand in \
          "${arkosSysroot}/usr/lib/aarch64-linux-gnu/libz.so" \
          "${arkosSysroot}/lib/aarch64-linux-gnu/libz.so" \
          "${arkosSysroot}/usr/lib/aarch64-linux-gnu/libz.so.1" \
          "${arkosSysroot}/lib/aarch64-linux-gnu/libz.so.1"
        do
          if [ -e "$cand" ]; then ZLIB_LIB="$cand"; break; fi
        done
        if [ -z "$ZLIB_LIB" ]; then
          echo "arkos-sysroot: no libz.so found" >&2
          exit 1
        fi
        cmakeFlagsArray+=(
          "-DZLIB_INCLUDE_DIR=${arkosSysroot}/usr/include"
          "-DZLIB_LIBRARY=$ZLIB_LIB"
        )
      '';

      postInstall = ''
        mkdir -p $out/share/supertux-milestone1
        if [ -d "$src/data" ]; then
          cp -a "$src/data/." $out/share/supertux-milestone1/ || true
        fi
        cat > $out/share/supertux-milestone1/README-R36S.txt << EOF_README
SuperTux Milestone 1 — R36S / ArkOS (sysroot-linked)
====================================================

Binary: bin/supertux-milestone1
  SDL2 + GLES2, linked against the ArkOS aarch64 sysroot.

Deploy the binary + share/supertux-milestone1 data to the device.
EOF_README
        cat > $out/share/supertux-milestone1/supertux-milestone1.sh << 'LAUNCH'
#!/bin/bash
DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$DIR"
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
