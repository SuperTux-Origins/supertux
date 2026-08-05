# GP2X (Open2x) and Wiz (GPH / OpenWiz-style) cross builds.
#
# Open2x: historical apps toolchain + libpack (nanard.free.fr mirrors).
# Wiz:    GPH_SDK 10.02 (steward-fu mirror) — arm-linux GCC 4.0.2 + DGE SDL 1.2.
#
# Host tools are i686 ELF; every Intel 80386 executable is replaced by a
# qemu-i386 wrapper (-L pkgsi686Linux.glibc) so nested cc1/as work without
# kernel IA32 or host /lib/ld-linux.so.2.
#
# Usage:
#   nix build .#supertux-milestone1-gp2x
#   nix build .#supertux-milestone1-wiz
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
  i686Glibc = pkgsi686Linux.glibc;

  qemuI386 =
    if builtins.pathExists "${qemu}/bin/qemu-i386-static" then
      "${qemu}/bin/qemu-i386-static"
    else
      "${qemu}/bin/qemu-i386";

  bashInterp = "${bash}/bin/bash";

  # Move every Intel 80386 EXEC/DYN under root to a .real + qemu wrapper.
  wrapI686HostTools = ''
    QEMU="${qemuI386}"
    GPREFIX="${i686Glibc}"
    BASH="${bashInterp}"
    echo "Wrapping i686 host tools under $ROOT with $QEMU -L $GPREFIX"
    test -x "$QEMU"
    test -e "$GPREFIX/lib/ld-linux.so.2"
    test -x "$BASH"
    LIST=$(mktemp)
    find "$ROOT" -type f > "$LIST"
    wrapped=0
    while IFS= read -r f; do
      case "$f" in
        *.real|*.a|*.la|*.h|*.o|*.so|*.so.*) continue ;;
      esac
      if ! readelf -h "$f" 2>/dev/null | grep -q 'Machine:[[:space:]]*Intel 80386'; then
        continue
      fi
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
  '';

  # ---- Open2x (classic GP2X) ----
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
      ${wrapI686HostTools}
      "$ROOT/bin/arm-open2x-linux-gcc" --version
      "$ROOT/bin/arm-open2x-linux-g++" --version
    '';
    meta = {
      description = "Open2x apps toolchain + libpack (GP2X SDL 1.2)";
      license = lib.licenses.gpl2Plus;
      platforms = lib.platforms.linux;
    };
  };

  # ---- GPH SDK (GP2X Wiz) ----
  # Official GPH Linux SDK mirrored by steward-fu; triple arm-linux, DGE SDL 1.2.
  gphSdk = fetchurl {
    name = "GPH_SDK-10.02_linux.tar.gz";
    url = "https://github.com/steward-fu/website/releases/download/wiz/GPH_SDK-10.02_linux.tar.gz";
    sha256 = "c0945d5a0ebe1ac75fe0046c1bc80e15be7286cd8ac3accf3f89c6c816d45988";
  };

  openwizSysroot = stdenvNoCC.mkDerivation {
    pname = "openwiz-sysroot";
    version = "gph-sdk-10.02";
    dontUnpack = true;
    nativeBuildInputs = [ file binutils ];
    installPhase = ''
      mkdir -p $out
      tar xzf ${gphSdk} -C $out
      TC="$out/GPH_SDK/tools/gcc-4.0.2-glibc-2.3.6/arm-linux"
      DGE="$out/GPH_SDK/DGE"
      test -x "$TC/bin/arm-linux-gcc"
      test -d "$DGE/lib/target"
      test -d "$DGE/include/SDL"

      # Full toolchain tree (bin, lib, libexec, include/c++/4.0.2, arm-linux/*)
      # so g++ finds libstdc++ headers without extra -isystem flags.
      ROOT="$out/sysroot"
      mkdir -p "$ROOT"
      cp -a "$TC/." "$ROOT/"

      # DGE SDL 1.2 headers + target libs
      cp -a "$DGE/include/." "$ROOT/include/"
      cp -a "$DGE/lib/target/." "$ROOT/lib/"

      # GPH top-level includes/libs (zlib)
      if [ -d "$out/GPH_SDK/include" ]; then
        cp -a "$out/GPH_SDK/include/." "$ROOT/include/"
      fi
      if [ -d "$out/GPH_SDK/lib/target" ]; then
        cp -a "$out/GPH_SDK/lib/target/." "$ROOT/lib/"
      fi

      test -e "$ROOT/include/c++/4.0.2/iostream"
      test -e "$ROOT/include/zlib.h"
      test -e "$ROOT/lib/libSDL.so" -o -e "$ROOT/lib/libSDL.a"
      test -e "$ROOT/lib/libz.a" -o -e "$ROOT/lib/libz.so"
      test -x "$ROOT/bin/arm-linux-gcc"

      ${wrapI686HostTools}

      # Smoke: C++ stdlib + zlib must be visible to the driver
      "$ROOT/bin/arm-linux-g++" -print-file-name=iostream
      "$ROOT/bin/arm-linux-gcc" --version
      "$ROOT/bin/arm-linux-g++" --version
      echo '#include <iostream>
int main(){return 0;}' > /tmp/t.cpp
      "$ROOT/bin/arm-linux-g++" -c /tmp/t.cpp -o /tmp/t.o
    '';
    meta = {
      description = "GPH SDK 10.02 toolchain + DGE SDL 1.2 (GP2X Wiz)";
      license = lib.licenses.gpl2Plus;
      platforms = lib.platforms.linux;
    };
  };

  mkHandheld = {
    src, version, pname
    , openRoot          # path to flat sysroot (bin/include/lib)
    , cc, cxx, ar, ranlib, strip
    , cmakeExtraFlags ? []
    , binarySuffix ? ".gpe"
    , readmeTitle ? "handheld"
  }:
    stdenv.mkDerivation rec {
      inherit pname version src;
      enableParallelBuilding = true;
      nativeBuildInputs = [ cmake pkg-config qemu ];
      dontUseCmakeConfigure = true;

      configurePhase = ''
        runHook preConfigure
        export PATH="${openRoot}/bin:$PATH"
        cmake -S . -B build \
          -DCMAKE_SYSTEM_NAME=Linux \
          -DCMAKE_SYSTEM_PROCESSOR=arm \
          -DCMAKE_C_COMPILER=${openRoot}/bin/${cc} \
          -DCMAKE_CXX_COMPILER=${openRoot}/bin/${cxx} \
          -DCMAKE_AR=${openRoot}/bin/${ar} \
          -DCMAKE_RANLIB=${openRoot}/bin/${ranlib} \
          -DCMAKE_C_COMPILER_WORKS=1 \
          -DCMAKE_CXX_COMPILER_WORKS=1 \
          -DCMAKE_FIND_ROOT_PATH="${openRoot}" \
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
          -DDATA_PREFIX=. \
          -DPROJECT_VERSION_FULL="${version}" \
          ${lib.concatStringsSep " " cmakeExtraFlags}
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
        STRIP="${openRoot}/bin/${strip}"
        if [ -x "$STRIP" ]; then
          "$STRIP" build/supertux-milestone1 || true
        fi
        cp build/supertux-milestone1 $out/bin/supertux-milestone1${binarySuffix}
        chmod +x $out/bin/supertux-milestone1${binarySuffix}
        ln -s supertux-milestone1${binarySuffix} $out/bin/supertux-milestone1
        if [ -d data ]; then
          cp -a data $out/share/supertux-milestone1/
        fi
        cat > $out/share/supertux-milestone1/README-${readmeTitle}.txt << EOF
SuperTux Milestone 1 — ${readmeTitle}
=====================================

Binary: bin/supertux-milestone1${binarySuffix}
  ARM, SDL 1.2, 320×240, ENABLE_GP2X, no sound in this package.

Place data/ next to the binary on the SD card (GMenu2X / Wiz menu).
EOF
        runHook postInstall
      '';

      meta = with lib; {
        description = "SuperTux Milestone 1 (${readmeTitle})";
        license = licenses.gpl2Plus;
        platforms = platforms.linux;
        hydraPlatforms = [];
      };
    };

in
{
  inherit open2xSysroot openwizSysroot;

  mkSuperTuxGp2x = { src, version, pname ? "supertux-milestone1-gp2x" }:
    mkHandheld {
      inherit src version pname;
      openRoot = "${open2xSysroot}/sysroot";
      cc = "arm-open2x-linux-gcc";
      cxx = "arm-open2x-linux-g++";
      ar = "arm-open2x-linux-ar";
      ranlib = "arm-open2x-linux-ranlib";
      strip = "arm-open2x-linux-strip";
      cmakeExtraFlags = [
        ''-DCMAKE_C_FLAGS_INIT="-msoft-float -fomit-frame-pointer"''
        ''-DCMAKE_CXX_FLAGS_INIT="-msoft-float -fomit-frame-pointer"''
        ''-DOPEN2X_ROOT=${open2xSysroot}/sysroot''
      ];
      binarySuffix = ".gpe";
      readmeTitle = "GP2X-Open2x";
    };

  mkSuperTuxWiz = { src, version, pname ? "supertux-milestone1-wiz" }:
    mkHandheld {
      inherit src version pname;
      openRoot = "${openwizSysroot}/sysroot";
      cc = "arm-linux-gcc";
      cxx = "arm-linux-g++";
      ar = "arm-linux-ar";
      ranlib = "arm-linux-ranlib";
      strip = "arm-linux-strip";
      cmakeExtraFlags = [
        ''-DCMAKE_C_FLAGS_INIT="-mcpu=arm926ej-s -mtune=arm926ej-s -fomit-frame-pointer"''
        ''-DCMAKE_CXX_FLAGS_INIT="-mcpu=arm926ej-s -mtune=arm926ej-s -fomit-frame-pointer"''
        ''-DOPENWIZ_ROOT=${openwizSysroot}/sysroot''
      ];
      binarySuffix = ".gpe";
      readmeTitle = "GP2X-Wiz-GPH";
    };
}
