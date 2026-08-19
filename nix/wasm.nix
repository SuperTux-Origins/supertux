# SuperTux WebAssembly (Emscripten) packaging
#
# Origins already has first-class EMSCRIPTEN support in CMakeLists.txt and
# mk/emscripten/template.html.in.  This module builds under emscriptenStdenv.
#
# Remaining blockers for a green link (see PORTING.md / TODO.md):
#   - static wasm builds of squirrel / wstsound / tinycmmc family, OR
#   - further CMake soft-disables under EMSCRIPTEN
# PhysFS can be supplied via physfs-src + PHYSFS_SOURCE_DIR.

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

  physfsSrcPath = if physfs-src != null then physfs-src else null;

in
{
  supertux-wasm = pkgs.emscriptenStdenv.mkDerivation rec {
    pname = "supertux-origins-wasm";
    version = "0.6.3-wasm";

    src = lib.cleanSource self;

    nativeBuildInputs = [
      pkgs.buildPackages.cmake
      pkgs.buildPackages.pkg-config
      emscripten
      miniswig.packages.${pkgs.system}.default
    ];

    buildInputs = [
      glmPrefix
      tinycmmc.packages.${pkgs.system}.default
      sexpcpp.packages.${pkgs.system}.default
      logmich.packages.${pkgs.system}.default
      strutcpp.packages.${pkgs.system}.default
      squirrel.packages.${pkgs.system}.default
      wstsound.packages.${pkgs.system}.default
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
    ] ++ lib.optionals (physfsSrcPath != null) [
      "-DPHYSFS_SOURCE_DIR=${physfsSrcPath}"
    ];

    preConfigure = ''
      export EM_CACHE="''${TMPDIR:-/tmp}/emcache-supertux"
      mkdir -p "$EM_CACHE"
      export EM_PORTS="''${TMPDIR:-/tmp}/emports-supertux"
      mkdir -p "$EM_PORTS"
    '';

    dontStrip = true;

    installPhase = ''
      runHook preInstall
      mkdir -p $out/share/supertux-origins-wasm $out/bin
      for f in supertux-origins.html supertux-origins.js supertux-origins.wasm \
               supertux-origins.data supertux-origins.worker.js; do
        if [ -f "$f" ]; then
          cp -v "$f" $out/share/supertux-origins-wasm/
        fi
      done
      find . -maxdepth 2 \( -name '*.html' -o -name '*.wasm' -o -name '*.js' -o -name '*.data' \) \
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
      # Still broken until squirrel/wstsound/tinycmmc static wasm land.
      broken = true;
    };
  };
}
