# SuperTux WebAssembly (Emscripten) packaging
#
# Builds under emscriptenStdenv. Vendored external/ deps are compiled with the
# same stdenv so they are actually wasm-compatible (flake input packages are
# usually native x86_64).

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

  # Build a simple cmake project from external/ under emscriptenStdenv.
  # Important: emscriptenStdenv defaults to autotools (./configure). Force CMake.
  mkWasmCmake = { pname, srcPath, cmakeFlags ? [], buildInputs ? [] }:
    est.mkDerivation {
      inherit pname;
      version = "0.0.1-wasm";
      src = lib.cleanSource srcPath;
      nativeBuildInputs = [ pkgs.buildPackages.cmake emscripten ];
      inherit buildInputs;
      dontUseCmakeConfigure = true;
      dontConfigure = true;
      # emscriptenStdenv injects a failing default checkPhase; override fully.
      doCheck = false;
    checkPhase = "echo skip-emscripten-check";
      checkPhase = "echo skip-emscripten-check";
      preBuild = ''
        export EM_CACHE="''${TMPDIR:-/tmp}/emcache-${pname}"
        mkdir -p "$EM_CACHE"
        # emcmake drives cmake with the emscripten toolchain
        emcmake cmake -S . -B build           -DCMAKE_BUILD_TYPE=Release           -DCMAKE_INSTALL_PREFIX=$out           -DBUILD_TESTS=OFF           -DWARNINGS=OFF           -DWERROR=OFF           ${lib.concatStringsSep " " cmakeFlags}
        cmake --build build -j''${NIX_BUILD_CORES:-$(nproc)}
        cmake --install build
      '';
      # Skip default phases that expect autotools/cmake hooks
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

in
{
  inherit logmichWasm sexpcppWasm strutcppWasm;

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

    buildInputs = [
      glmPrefix
      logmichWasm
      sexpcppWasm
      strutcppWasm
      tinycmmcNative
      # squirrel / wstsound still from flake until wasm static builds exist
      squirrel.packages.${pkgs.stdenv.hostPlatform.system}.default
      wstsound.packages.${pkgs.stdenv.hostPlatform.system}.default
    ];

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
    ] ++ lib.optionals (physfsSrcPath != null) [
      "-DPHYSFS_SOURCE_DIR=${physfsSrcPath}"
    ];

    preBuild = ''
      export EM_CACHE="''${TMPDIR:-/tmp}/emcache-supertux"
      mkdir -p "$EM_CACHE"
      export EM_PORTS="''${TMPDIR:-/tmp}/emports-supertux"
      mkdir -p "$EM_PORTS"
      emcmake cmake -S . -B build         -DCMAKE_BUILD_TYPE=Release         -DCMAKE_INSTALL_PREFIX=$out         ${lib.concatStringsSep " " cmakeFlags}
      cmake --build build -j''${NIX_BUILD_CORES:-$(nproc)}
    '';
    buildPhase = "runHook preBuild; runHook postBuild";
    # installPhase already custom below

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
