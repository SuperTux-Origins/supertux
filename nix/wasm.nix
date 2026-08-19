# SuperTux WebAssembly (Emscripten) packaging
#
# Builds under emscriptenStdenv. Vendored external/ deps are compiled with the
# same stdenv so they are actually wasm-compatible (flake input packages are
# usually native x86_64).
#
# Sound path matches Pingus: static libmodplug + in-tree wstsound (WAV + modules).
# See mk/wasm/scripts/ and PORTING.md.

{ pkgs
, self
, tinycmmc
, sexpcpp
, logmich
, strutcpp
, miniswig
, wstsound
, squirrel
, physfs-src ? null
, squirrel-src ? null
, sdlSrc ? null
, sdlVersion ? "2.30.3"
}:

let
  lib = pkgs.lib;
  emscripten = pkgs.emscripten;
  est = pkgs.emscriptenStdenv;

  glmPrefix = pkgs.runCommand "glm-headers-wasm" { } ''
    mkdir -p $out/include $out/lib/cmake/glm
    cp -a ${pkgs.glm}/include/. $out/include/
    cat > $out/lib/cmake/glm/glmConfig.cmake <<'EOFC'
set(_glm_inc "''${CMAKE_CURRENT_LIST_DIR}/../../../include")
if(NOT TARGET glm::glm)
  add_library(glm::glm INTERFACE IMPORTED)
  set_target_properties(glm::glm PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "''${_glm_inc}")
endif()
set(glm_FOUND TRUE)
EOFC
  '';

  # --- libmodplug static for wasm (Pingus recipe) ----------------------------
  # SuperTux / wstsound force WSTSOUND_WITH_MODPLUG=ON under EMSCRIPTEN.
  modplugWasm = pkgs.stdenv.mkDerivation rec {
    pname = "libmodplug-wasm";
    version = "0.8.9.0";
    src = pkgs.fetchurl {
      url = "https://downloads.sourceforge.net/project/modplug-xmms/libmodplug/${version}/libmodplug-${version}.tar.gz";
      hash = "sha256-RXylpsF5ZW1mwBUFwNlfr66tQym526oPmX0Ao1CK2d4=";
    };
    nativeBuildInputs = [ emscripten pkgs.python3 ];
    dontConfigure = true;
    buildPhase = ''
      runHook preBuild
      export EM_CACHE="''${TMPDIR:-/tmp}/emcache-modplug"
      mkdir -p "$EM_CACHE"
      mkdir -p "$PWD/prefix"
      # libmodplug 0.8.9 still uses the C++ `register` keyword; em++ defaults
      # to C++17 where that is an error. Force C++14 for this ancient tree.
      export CXXFLAGS="-std=gnu++14 -Wno-register ''${CXXFLAGS:-}"
      export CFLAGS="-Wno-register ''${CFLAGS:-}"
      emconfigure ./configure \
        --prefix="$PWD/prefix" \
        --host=wasm32-unknown-emscripten \
        --disable-shared \
        --enable-static \
        --disable-dependency-tracking
      emmake make -j''${NIX_BUILD_CORES:-2}
      emmake make install
      runHook postBuild
    '';
    installPhase = ''
      runHook preInstall
      mkdir -p $out
      cp -a prefix/. $out/
      mkdir -p $out/lib/pkgconfig
      cat > $out/lib/pkgconfig/libmodplug.pc <<EOFPC
prefix=$out
exec_prefix=\''${prefix}
libdir=\''${exec_prefix}/lib
includedir=\''${prefix}/include

Name: libmodplug
Description: modplug module music decoder (wasm32-emscripten)
Version: ${version}
Libs: -L\''${libdir} -lmodplug
Cflags: -I\''${includedir}
EOFPC
      runHook postInstall
    '';
    meta = with lib; {
      description = "Static libmodplug for wasm32-emscripten";
      license = licenses.publicDomain;
      platforms = platforms.linux;
    };
  };

  # Build a simple cmake project from external/ under emscriptenStdenv.
  # Important: emscriptenStdenv defaults to autotools (./configure). Force CMake.
  mkWasmCmake = { pname, srcPath, cmakeFlags ? [], buildInputs ? [] }:
    est.mkDerivation {
      inherit pname;
      version = "0.0.1-wasm";
      src = lib.cleanSource srcPath;
      nativeBuildInputs = [ pkgs.buildPackages.cmake emscripten pkgs.pkg-config ];
      inherit buildInputs;
      dontUseCmakeConfigure = true;
      dontConfigure = true;
      # emscriptenStdenv injects a failing default checkPhase; override fully.
      doCheck = false;
      checkPhase = "echo skip-emscripten-check";
      preBuild = ''
      export EM_CACHE="''${TMPDIR:-/tmp}/emcache-supertux"
      mkdir -p "$EM_CACHE"
      export PKG_CONFIG_PATH="${modplugWasm}/lib/pkgconfig''${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
      export CMAKE_PREFIX_PATH="${modplugWasm}''${CMAKE_PREFIX_PATH:+:$CMAKE_PREFIX_PATH}"
${lib.optionalString (sdl2WasmLibs != null) ''
      # Offline static SDL2 (no emscripten port download).
      export PKG_CONFIG_PATH="${sdl2WasmLibs}/lib/pkgconfig:$PKG_CONFIG_PATH"
      export CMAKE_PREFIX_PATH="${sdl2WasmLibs}:$CMAKE_PREFIX_PATH"
      export SDL2_DIR="${sdl2WasmLibs}"
''}
      emcmake cmake -S . -B build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=$out \
        ${lib.escapeShellArgs cmakeFlags}
      cmake --build build -j''${NIX_BUILD_CORES:-$(nproc)}
    '';
    buildPhase = "runHook preBuild; runHook postBuild";
      installPhase = "runHook preInstall; runHook postInstall";
      dontStrip = true;
    };

  logmichWasm = mkWasmCmake {
    pname = "logmich-wasm";
    srcPath = ../external/logmich;
  };

  sexpcppWasm = mkWasmCmake {
    pname = "sexpcpp-wasm";
    srcPath = ../external/sexpcpp;
  };

  strutcppWasm = mkWasmCmake {
    pname = "strutcpp-wasm";
    srcPath = ../external/strutcpp;
  };

  # tinycmmc is mostly CMake modules — use native flake package for the module path
  tinycmmcNative = tinycmmc.packages.${pkgs.stdenv.hostPlatform.system}.default;

  # PhysFS from source tree if provided
  physfsSrcPath = if physfs-src != null then physfs-src else null;

  # external/squirrel is packaging-only (no CMakeLists). Require flake input.
  squirrelSrcPath =
    if squirrel-src != null then squirrel-src
    else throw "nix/wasm.nix: squirrel-src flake input is required (external/squirrel has no sources). Run: nix flake lock --update-input squirrel-src";

  modplugCmakeFlags = [
    "-DMODPLUG_DIR=${modplugWasm}"
    "-DMODPLUG_INCLUDE_DIRECTORY=${modplugWasm}/include"
    "-DMODPLUG_LIBRARY=${modplugWasm}/lib/libmodplug.a"
    "-DWSTSOUND_WITH_MODPLUG=ON"
    "-DWSTSOUND_WITH_VORBIS=OFF"
    "-DWSTSOUND_WITH_OPUS=OFF"
    "-DWSTSOUND_WITH_MPG123=OFF"
    "-DWSTSOUND_WITH_EFX=OFF"
  ];

  # Offline static SDL2 for wasm (Pingus pattern). Avoids -sUSE_SDL=2 which
  # downloads the emscripten port at compile/link time (nix sandbox has no net).
  sdl2WasmLibs =
    if sdlSrc == null then null
    else pkgs.stdenv.mkDerivation {
      pname = "sdl2-wasm";
      version = sdlVersion;
      dontUnpack = true;
      dontConfigure = true;
      dontUseCmakeConfigure = true;
      nativeBuildInputs = [ emscripten pkgs.cmake pkgs.python3 ];
      env = {
        SDL_SRC = "${sdlSrc}";
      };
      buildPhase = ''
        runHook preBuild
        bash ${../mk/wasm/scripts/build-sdl2.sh}
        runHook postBuild
      '';
      installPhase = ''
        runHook preInstall
        mkdir -p $out
        if [ -d prefix ]; then
          cp -a prefix/. $out/
        else
          mkdir -p $out/lib $out/include
          find . -name 'libSDL2.a' -exec cp {} $out/lib/ \; || true
          if [ -d SDL2-src/include ]; then cp -a SDL2-src/include/. $out/include/; fi
        fi
        mkdir -p $out/lib/pkgconfig
        cat > $out/lib/pkgconfig/sdl2.pc <<EOF
prefix=$out
exec_prefix=\''${prefix}
libdir=\''${prefix}/lib
includedir=\''${prefix}/include
Name: sdl2
Description: SDL2 (wasm static)
Version: ${sdlVersion}
Libs: -L\''${libdir} -lSDL2
Cflags: -I\''${includedir} -I\''${includedir}/SDL2
EOF
        runHook postInstall
      '';
    };

in
{
  inherit logmichWasm sexpcppWasm strutcppWasm modplugWasm;

  supertux-wasm = est.mkDerivation rec {
    pname = "supertux-origins-wasm";
    version = "0.6.3-wasm";

    src = lib.cleanSource self;

    # emscriptenStdenv defaults to ./configure; SuperTux is CMake-only.
    dontConfigure = true;
    doCheck = false;
    checkPhase = "echo skip-emscripten-check";

    nativeBuildInputs = [
      pkgs.buildPackages.cmake
      pkgs.buildPackages.pkg-config
      emscripten
      miniswig.packages.${pkgs.stdenv.hostPlatform.system}.default
    ];

    # Do NOT put native (x86_64) wstsound/squirrel from flake inputs here —
    # they break find_package / are wrong arch. Prefer in-tree external/ under
    # emcmake, with modplug provided via CMAKE flags.
    buildInputs = [
      glmPrefix
      logmichWasm
      sexpcppWasm
      strutcppWasm
      tinycmmcNative
      modplugWasm
    ] ++ lib.optional (sdl2WasmLibs != null) sdl2WasmLibs;

    # Offline SDL2: do NOT put -sUSE_SDL=2 on CMAKE_C/CXX_FLAGS (triggers
    # emscripten port download per TU). Headers come from sdl2WasmLibs.
    cmakeFlags = [
      "-DENABLE_OPENGL=ON"
      "-DENABLE_OPENGLES2=ON"
      "-DUSE_SYSTEM_SDL2_TTF=OFF"
      "-DUSE_SYSTEM_PHYSFS=OFF"
      "-DBUILD_TESTS=OFF"
      "-DINSTALL_SUBDIR_BIN=bin"
      "-DINSTALL_SUBDIR_SHARE=data"
      "-Dglm_DIR=${glmPrefix}/lib/cmake/glm"
      "-DPRIO_USE_JSONCPP=OFF"
      # Point CMake at static modplug so external/wstsound configures.
      "-DCMAKE_PREFIX_PATH=${modplugWasm}${lib.optionalString (sdl2WasmLibs != null) ";${sdl2WasmLibs}"}"
      "-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=BOTH"
      "-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=BOTH"
      "-DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=BOTH"
    ] ++ modplugCmakeFlags
      ++ lib.optionals (physfsSrcPath != null) [
      "-DPHYSFS_SOURCE_DIR=${physfsSrcPath}"
    ] ++ [
      "-DSQUIRREL_SOURCE_DIR=${squirrelSrcPath}"
      "-DUSE_SYSTEM_SQUIRREL=OFF"
      "-DPROJECT_VERSION_FULL=${version}"
    ];

    preBuild = ''
      export EM_CACHE="''${TMPDIR:-/tmp}/emcache-supertux"
      mkdir -p "$EM_CACHE" "$EM_CACHE/ports" "$EM_CACHE/downloads"
      export EM_PORTS="''${TMPDIR:-/tmp}/emports-supertux"
      mkdir -p "$EM_PORTS"
      # Seed SDL2 port so emscripten does not hit the network (nix sandbox).
      # ports/sdl2.py requests name=sdl2 → typically $EM_CACHE/ports/sdl2.zip
      # or the URL basename release-2.32.10.zip under downloads/.
      cp -f ${sdl2PortZip} "$EM_CACHE/ports/sdl2.zip"
      cp -f ${sdl2PortZip} "$EM_CACHE/ports/release-2.32.10.zip"
      cp -f ${sdl2PortZip} "$EM_CACHE/downloads/release-2.32.10.zip"
      cp -f ${sdl2PortZip} "$EM_CACHE/downloads/sdl2.zip"
      # Also place under EM_PORTS tree (some emscripten layouts).
      cp -f ${sdl2PortZip} "$EM_PORTS/sdl2.zip"
      cp -f ${sdl2PortZip} "$EM_PORTS/release-2.32.10.zip"
      export PKG_CONFIG_PATH="${modplugWasm}/lib/pkgconfig''${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
      export CMAKE_PREFIX_PATH="${modplugWasm}''${CMAKE_PREFIX_PATH:+:$CMAKE_PREFIX_PATH}"
      emcmake cmake -S . -B build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=$out \
        ${lib.escapeShellArgs cmakeFlags}
      cmake --build build -j''${NIX_BUILD_CORES:-$(nproc)}
    '';
    buildPhase = "runHook preBuild; runHook postBuild";

    dontStrip = true;

    installPhase = ''
      runHook preInstall
      mkdir -p $out/share/supertux-origins-wasm $out/bin
      find . -maxdepth 3 \( -name '*.html' -o -name '*.wasm' -o -name '*.js' -o -name '*.data' \) \
        -exec cp -v {} $out/share/supertux-origins-wasm/ \; || true
      if [ -f ${../mk/wasm/scripts/serve.sh} ]; then
        cp ${../mk/wasm/scripts/serve.sh} $out/bin/supertux-wasm-serve
        chmod +x $out/bin/supertux-wasm-serve
      fi
      runHook postInstall
    '';

    meta = with lib; {
      description = "SuperTux (Origins) WebAssembly build";
      platforms = [ "x86_64-linux" "aarch64-linux" ];
    };
  };
}
