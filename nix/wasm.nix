# SuperTux WebAssembly (Emscripten) packaging
#
# Origins already has first-class EMSCRIPTEN support in CMakeLists.txt:
#   -sUSE_SDL=2 -sUSE_SDL_IMAGE=2 -sUSE_FREETYPE=2 -sFULL_ES2=1
#   exceptions, GROWABLE_ARRAYBUFFERS=0, FORCE_FILESYSTEM, preload @/data
# and mk/emscripten/template.html.in.
#
# This module builds the game under emscriptenStdenv and relies on those
# CMake flags for the SDL/image/freetype ports.  Remaining static deps
# (physfs, squirrel, tinycmmc family, wstsound, …) still need wasm builds
# or further CMake conditionals — tracked in TODO.md / PORTING.md.
#
# The heavier Pingus-style static SDL stack is kept as
# nix/wasm-pingus-reference.nix for when we need offline ports instead of
# -sUSE_* emscripten ports.

{ pkgs
, self
, tinycmmc
, sexpcpp
, logmich
, strutcpp
, miniswig
, wstsound
, squirrel
, priocpp
}:

let
  lib = pkgs.lib;
  emscripten = pkgs.emscripten;

  # Header-only glm with a cmake config that works under emscripten FIND_ROOT.
  glmPrefix = pkgs.runCommand "glm-headers-wasm" { } ''
    mkdir -p $out/include $out/lib/cmake/glm
    cp -a ${pkgs.glm}/include/. $out/include/
    cat > $out/lib/cmake/glm/glmConfig.cmake <<'EOF'
set(_glm_inc "''${CMAKE_CURRENT_LIST_DIR}/../../../include")
if(NOT TARGET glm::glm)
  add_library(glm::glm INTERFACE IMPORTED)
  set_target_properties(glm::glm PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "''${_glm_inc}")
endif()
set(glm_FOUND TRUE)
EOF
  '';

in
{
  # Primary wasm game package (WIP — see PORTING.md).
  # Uses emscripten ports for SDL2 / SDL2_image / FreeType via CMake USE_FLAGS.
  # ENABLE_OPENGLES2=ON selects the GLES2 code path (no GLEW).
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

    # Host-side inputs that provide headers / cmake configs.  Many of these
    # are still native packages; a full offline wasm link will need static
    # wasm builds of physfs, squirrel, wstsound, tinycmmc stack, etc.
    buildInputs = [
      glmPrefix
      tinycmmc.packages.${pkgs.system}.default
      sexpcpp.packages.${pkgs.system}.default
      logmich.packages.${pkgs.system}.default
      strutcpp.packages.${pkgs.system}.default
      squirrel.packages.${pkgs.system}.default
      wstsound.packages.${pkgs.system}.default
      priocpp.packages.${pkgs.system}.priocpp-sexp
    ];

    cmakeFlags = [
      "-DENABLE_OPENGL=ON"
      "-DENABLE_OPENGLES2=ON"
      "-DUSE_SYSTEM_SDL2_TTF=OFF"
      "-DBUILD_TESTS=OFF"
      "-DINSTALL_SUBDIR_BIN=bin"
      "-DINSTALL_SUBDIR_SHARE=data"
      # Point cmake at our glm prefix
      "-Dglm_DIR=${glmPrefix}/lib/cmake/glm"
    ];

    # Ensure emcc/em++ are on PATH and cache is writable.
    preConfigure = ''
      export EM_CACHE="''${TMPDIR:-/tmp}/emcache-supertux"
      mkdir -p "$EM_CACHE"
      # Emscripten expects these for -sUSE_* ports
      export EM_PORTS="''${TMPDIR:-/tmp}/emports-supertux"
      mkdir -p "$EM_PORTS"
    '';

    # CMake already appends the heavy USE_FLAGS when EMSCRIPTEN is detected.
    # We only need to make sure the compiler is em++.
    dontStrip = true;

    # Install the .html / .js / .wasm / data preload artefacts.
    installPhase = ''
      runHook preInstall
      mkdir -p $out/share/supertux-origins-wasm $out/bin
      # Typical emscripten outputs next to the binary name
      for f in supertux-origins.html supertux-origins.js supertux-origins.wasm \
               supertux-origins.data supertux-origins.worker.js; do
        if [ -f "$f" ]; then
          cp -v "$f" $out/share/supertux-origins-wasm/
        fi
      done
      # Fallback: anything matching
      find . -maxdepth 2 \( -name '*.html' -o -name '*.wasm' -o -name '*.js' -o -name '*.data' \) \
        -exec cp -v {} $out/share/supertux-origins-wasm/ \; || true
      # Convenience wrapper that serves the build (optional)
      if [ -f ${../mk/wasm/scripts/serve.sh} ]; then
        cp ${../mk/wasm/scripts/serve.sh} $out/bin/supertux-wasm-serve
        chmod +x $out/bin/supertux-wasm-serve
      fi
      runHook postInstall
    '';

    meta = with lib; {
      description = "SuperTux (Origins) WebAssembly build";
      platforms = [ "x86_64-linux" "aarch64-linux" ];
      # Mark broken until static wasm deps for physfs/squirrel/wstsound land
      # and the link succeeds end-to-end.  Remove when green.
      broken = true;
    };
  };
}
