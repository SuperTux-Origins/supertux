# GP2X / Open2x cross build for SuperTux Milestone 1.
#
# Fetches the historical Open2x apps toolchain (GCC 4.1.1 + glibc 2.3.6) and
# libpack (SDL 1.2, SDL_image, …), then cross-compiles with ENABLE_GP2X.
#
# Host tools are i686 ELF. Every Intel 80386 executable is replaced by a
# qemu-i386 wrapper (-L pkgsi686Linux.glibc) so nested cc1/as work without
# kernel IA32 or host /lib/ld-linux.so.2.
#
# Usage:
#   nix build .#supertux-milestone1-gp2x
#
{ lib
, stdenv
, stdenvNoCC
, fetchurl
, cmake
, pkg-config
, qemu
, pkgsi686Linux
, file
, bash
, binutils
}:

let
  open2xToolchain = fetchurl {
    name = "arm-open2x-linux-apps-gcc-4.1.1-glibc-2.3.6_i686_linux.tar.bz2";
    url = "http://nanard.free.fr/grafx2/arm-open2x-linux-apps-gcc-4.1.1-glibc-2.3.6_i686_linux.tar.bz2.zip";
    sha256 = "ecb53e2799bbd6953621b2eedeed7280f5b03c3b2a6825607cafbe5dc1d545d8";
  };

  open2xLibpack = fetchurl {
    name = "open2x-libpack-20071903-gcc-4.1.1-glibc-2.3.6.tar.bz2";
    url = "http://nanard.free.fr/grafx2/open2x-libpack-20071903-gcc-4.1.1-glibc-2.3.6.tar.bz2.zip";
    sha256 = "31f46111c1d8bd38b720b292f65213adc624b050a614dab1199b56b35244efd7";
  };

  i686Glibc = pkgsi686Linux.glibc;

  qemuI386 =
    if builtins.pathExists "${qemu}/bin/qemu-i386-static" then
      "${qemu}/bin/qemu-i386-static"
    else
      "${qemu}/bin/qemu-i386";

  # Store path — pure sandbox has no /bin/sh or /bin/bash.
  bashInterp = "${bash}/bin/bash";

  open2xSysroot = stdenvNoCC.mkDerivation {
    pname = "open2x-sysroot";
    version = "gcc-4.1.1-glibc-2.3.6";
    dontUnpack = true;
    nativeBuildInputs = [ file binutils ];

    installPhase = ''
      mkdir -p $out
      tar xjf ${open2xToolchain} -C $out
      tar xjf ${open2xLibpack} -C $out
      ROOT="$out/opt/open2x/gcc-4.1.1-glibc-2.3.6"
      if [ -d "$out/gcc-4.1.1-glibc-2.3.6" ]; then
        cp -a "$out/gcc-4.1.1-glibc-2.3.6/." "$ROOT/"
        rm -rf "$out/gcc-4.1.1-glibc-2.3.6"
      fi
      test -x "$ROOT/bin/arm-open2x-linux-gcc"
      test -e "$ROOT/lib/libSDL.so" -o -e "$ROOT/lib/libSDL.a"
      ln -sfn opt/open2x/gcc-4.1.1-glibc-2.3.6 $out/sysroot

      QEMU="${qemuI386}"
      GPREFIX="${i686Glibc}"
      BASH="${bashInterp}"
      echo "Wrapping i686 host tools with $QEMU -L $GPREFIX"
      echo "Wrapper shebang: $BASH"
      test -x "$QEMU"
      test -e "$GPREFIX/lib/ld-linux.so.2"
      test -x "$BASH"

      # Collect paths first so find is not racing with renames.
      LIST=$(mktemp)
      find "$ROOT" -type f > "$LIST"
      wrapped=0
      while IFS= read -r f; do
        case "$f" in
          *.real|*.a|*.la|*.h|*.o|*.so|*.so.*) continue ;;
        esac
        # Host tools are Intel 80386; target libs/binaries are ARM — skip ARM.
        if ! readelf -h "$f" 2>/dev/null | grep -q 'Machine:[[:space:]]*Intel 80386'; then
          continue
        fi
        # Skip non-executables (relocatable objects already excluded by *.o).
        if ! readelf -h "$f" 2>/dev/null | grep -q 'Type:[[:space:]]*EXEC\|Type:[[:space:]]*DYN'; then
          continue
        fi
        mv "$f" "$f.real"
        cat > "$f" << EOF
#!$BASH
exec "$QEMU" -L "$GPREFIX" "$f.real" "\$@"
EOF
        chmod +x "$f"
        wrapped=$((wrapped + 1))
      done < "$LIST"
      rm -f "$LIST"
      echo "Wrapped $wrapped i686 host executables"
      test "$wrapped" -gt 0

      # Sanity: compiler --version under qemu
      head -1 "$ROOT/bin/arm-open2x-linux-gcc"
      "$ROOT/bin/arm-open2x-linux-gcc" --version
      "$ROOT/bin/arm-open2x-linux-g++" --version
    '';

    meta = {
      description = "Open2x apps toolchain + libpack (GP2X SDL 1.2), qemu-wrapped host tools";
      license = lib.licenses.gpl2Plus;
      platforms = lib.platforms.linux;
    };
  };

in
{
  inherit open2xSysroot;

  mkSuperTuxGp2x = { src, version, pname ? "supertux-milestone1-gp2x" }:
    let
      open2xRoot = "${open2xSysroot}/sysroot";
    in
    stdenv.mkDerivation rec {
      inherit pname version src;
      enableParallelBuilding = true;

      nativeBuildInputs = [ cmake pkg-config qemu ];

      dontUseCmakeConfigure = true;

      configurePhase = ''
        runHook preConfigure
        export OPEN2X_ROOT="${open2xRoot}"
        export PATH="${open2xRoot}/bin:$PATH"

        cmake -S . -B build \
          -DCMAKE_SYSTEM_NAME=Linux \
          -DCMAKE_SYSTEM_PROCESSOR=arm \
          -DCMAKE_C_COMPILER=${open2xRoot}/bin/arm-open2x-linux-gcc \
          -DCMAKE_CXX_COMPILER=${open2xRoot}/bin/arm-open2x-linux-g++ \
          -DCMAKE_AR=${open2xRoot}/bin/arm-open2x-linux-ar \
          -DCMAKE_RANLIB=${open2xRoot}/bin/arm-open2x-linux-ranlib \
          -DCMAKE_C_COMPILER_WORKS=1 \
          -DCMAKE_CXX_COMPILER_WORKS=1 \
          -DCMAKE_C_FLAGS_INIT="-msoft-float -fomit-frame-pointer" \
          -DCMAKE_CXX_FLAGS_INIT="-msoft-float -fomit-frame-pointer" \
          -DCMAKE_FIND_ROOT_PATH="${open2xRoot}" \
          -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
          -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
          -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
          -DCMAKE_BUILD_TYPE=Release \
          -DENABLE_GP2X=ON \
          -DENABLE_RES320X240=ON \
          -DENABLE_SDL2=OFF \
          -DENABLE_OPENGL=OFF \
          -DENABLE_GLES2=OFF \
          -DENABLE_SOUND=OFF \
          -DOPEN2X_ROOT="${open2xRoot}" \
          -DDATA_PREFIX=. \
          -DPROJECT_VERSION_FULL="${version}"
        runHook postConfigure
      '';

      buildPhase = ''
        runHook preBuild
        cmake --build build -j''${NIX_BUILD_CORES:-2}
        runHook postBuild
      '';

      installPhase = ''
        runHook preInstall
        mkdir -p $out/bin $out/share/supertux-milestone1
        STRIP="${open2xRoot}/bin/arm-open2x-linux-strip"
        if [ -x "$STRIP" ]; then
          "$STRIP" build/supertux-milestone1 || true
        fi
        cp build/supertux-milestone1 $out/bin/supertux-milestone1.gpe
        chmod +x $out/bin/supertux-milestone1.gpe
        ln -s supertux-milestone1.gpe $out/bin/supertux-milestone1
        if [ -d data ]; then
          cp -a data $out/share/supertux-milestone1/
        fi
        cat > $out/share/supertux-milestone1/README-GP2X.txt << EOF
SuperTux Milestone 1 — GP2X / Open2x build
==========================================

Binary: bin/supertux-milestone1.gpe  (ARM soft-float, SDL 1.2, 320×240, no sound)

On the SD card (typical GMenu2X layout):
  /mnt/sd/games/supertux/
    supertux-milestone1.gpe
    data/

Open2x userspace provides libSDL-1.2 / libSDL_image. Built with
Open2x gcc-4.1.1-glibc-2.3.6 + ENABLE_GP2X + ENABLE_RES320X240.
EOF
        runHook postInstall
      '';

      meta = with lib; {
        description = "SuperTux Milestone 1 for GP2X (Open2x, SDL 1.2, 320×240)";
        license = licenses.gpl2Plus;
        platforms = platforms.linux;
        hydraPlatforms = [];
      };
    };
}
