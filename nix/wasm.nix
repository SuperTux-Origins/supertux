# Emscripten / WebAssembly pipeline for SuperTux Milestone 1.
#
# Pattern mirrors helloworld-fireos (apk/nix/wasm.nix):
#   - Build SDL2 (+ SDL2_image) once as static wasm libs from flake inputs
#     (no -sUSE_SDL=2 network port).
#   - emcmake the game CMake tree against that prefix.
#   - Serve HTML over local HTTP (file:// cannot fetch .wasm).
#
# First bring-up uses ASYNCIFY so nested title/gameloop/worldmap loops keep
# working; a real requestAnimationFrame main loop is tracked in TODO.md.
#
# Usage (from flake.nix):
#   wasm = import ./nix/wasm.nix {
#     inherit pkgs;
#     sdlSrc = sdl2-src;
#     sdlImageSrc = sdl2-image-src;
#     sdlVersion = "2.30.3";
#   };
#   packages.supertux-milestone1-wasm = wasm.mkApp {
#     appName = "supertux-milestone1";
#     srcDir = ./.;
#     dataDir = ./data;   # optional; empty tree → no preload
#   };
{ pkgs
, sdlSrc
, sdlVersion
, sdlImageSrc ? null
}:

let
  sdlWasmLibs = pkgs.stdenv.mkDerivation {
    pname = "sdl2-wasm-libs";
    version = if sdlImageSrc != null then "${sdlVersion}+image" else sdlVersion;

    dontUnpack = true;
    dontConfigure = true;
    dontUseCmakeConfigure = true;
    nativeBuildInputs = [ pkgs.emscripten pkgs.cmake pkgs.python3 ];

    env = {
      SDL_SRC = "${sdlSrc}";
    } // pkgs.lib.optionalAttrs (sdlImageSrc != null) {
      SDL_IMAGE_SRC = "${sdlImageSrc}";
    };

    buildPhase = ''
      runHook preBuild
      bash ${./scripts/build-wasm-sdl-libs.sh}
      runHook postBuild
    '';

    installPhase = ''
      runHook preInstall
      mkdir -p $out
      # Prefer staged install prefix from build-wasm-sdl-libs.sh.
      if [ -d prefix ]; then
        cp -a prefix/. $out/
      else
        mkdir -p $out/lib $out/include
        find . -name 'libSDL2.a' -exec cp {} $out/lib/ \;
        find . -name 'libSDL2_image.a' -exec cp {} $out/lib/ \; || true
        if [ -d SDL2-src/include ]; then cp -a SDL2-src/include/. $out/include/; fi
        find . -name 'SDL_image.h' -exec cp {} $out/include/ \; || true
      fi
      # Convenience: pkg-config stubs so CMake pkg_check_modules can work if used.
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
      if [ -f $out/lib/libSDL2_image.a ]; then
        cat > $out/lib/pkgconfig/SDL2_image.pc <<EOF
prefix=$out
libdir=\''${prefix}/lib
includedir=\''${prefix}/include
Name: SDL2_image
Description: SDL2_image (wasm static)
Version: 2.8.2
Requires: sdl2
Libs: -L\''${libdir} -lSDL2_image
Cflags: -I\''${includedir}
EOF
      fi
      runHook postInstall
    '';
  };


  # Static zlib for wasm (lispreader / .gz levels). Offline — no -sUSE_ZLIB port.
  zlibWasmLibs = pkgs.stdenv.mkDerivation {
    pname = "zlib-wasm-libs";
    version = pkgs.zlib.version;

    dontUnpack = true;
    dontConfigure = true;
    nativeBuildInputs = [ pkgs.emscripten ];

    env = {
      ZLIB_SRC = "${pkgs.zlib.src}";
    };

    buildPhase = ''
      runHook preBuild
      bash ${./scripts/build-wasm-zlib.sh}
      runHook postBuild
    '';

    installPhase = ''
      runHook preInstall
      mkdir -p $out
      if [ -d prefix ]; then
        cp -a prefix/. $out/
      else
        echo "error: build-wasm-zlib.sh did not produce prefix/" >&2
        exit 1
      fi
      # Optional pkg-config for completeness.
      mkdir -p $out/lib/pkgconfig
      cat > $out/lib/pkgconfig/zlib.pc <<EOF
prefix=$out
exec_prefix=\''${prefix}
libdir=\''${prefix}/lib
includedir=\''${prefix}/include
Name: zlib
Description: zlib (wasm static)
Version: ${pkgs.zlib.version}
Libs: -L\''${libdir} -lz
Cflags: -I\''${includedir}
EOF
      runHook postInstall
    '';

    meta = with pkgs.lib; {
      description = "Static zlib built for wasm32-emscripten";
      license = licenses.zlib;
      platforms = platforms.linux;
    };
  };


  wasmShell = pkgs.writeText "supertux-wasm-shell.html" ''
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no">
  <title>SuperTux Milestone 1</title>
  <style>
    html, body {
      margin: 0; padding: 0; height: 100%;
      background: #0a0a12; color: #c8d0e0;
      font-family: system-ui, sans-serif;
      overflow: hidden;
    }
    #wrap {
      display: flex; flex-direction: column;
      align-items: center; justify-content: center;
      min-height: 100%; gap: 0.75rem;
    }
    canvas.emscripten {
      display: block;
      background: #000;
      image-rendering: pixelated;
      image-rendering: crisp-edges;
      max-width: 100vw; max-height: 90vh;
      outline: none;
    }
    #status { font-size: 0.9rem; opacity: 0.85; min-height: 1.2em; }
    #progress {
      width: min(320px, 80vw); height: 6px;
      background: #1a1a28; border-radius: 3px; overflow: hidden;
    }
    #progress > div {
      height: 100%; width: 0%; background: #4af;
      transition: width 0.15s ease-out;
    }
  </style>
</head>
<body>
  <div id="wrap">
    <canvas class="emscripten" id="canvas" oncontextmenu="event.preventDefault()" tabindex="-1"></canvas>
    <div id="status">Loading…</div>
    <div id="progress"><div id="progress-bar"></div></div>
  </div>
  <script type='text/javascript'>
    var statusElement = document.getElementById('status');
    var progressBar = document.getElementById('progress-bar');
    var Module = {
      preRun: [],
      postRun: [],
      print: function(text) {
        if (arguments.length > 1)
          text = Array.prototype.slice.call(arguments).join(' ');
        console.log(text);
      },
      printErr: function(text) {
        if (arguments.length > 1)
          text = Array.prototype.slice.call(arguments).join(' ');
        console.error(text);
      },
      canvas: (function() {
        var canvas = document.getElementById('canvas');
        canvas.addEventListener('webglcontextlost', function(e) {
          alert('WebGL context lost. Reload the page.');
          e.preventDefault();
        }, false);
        return canvas;
      })(),
      setStatus: function(text) {
        if (!Module.setStatus.last) Module.setStatus.last = { time: Date.now(), text: ''' };
        if (text === Module.setStatus.last.text) return;
        var m = text.match(/([^(]+)\((\d+(\.\d+)?)\/(\d+)\)/);
        var now = Date.now();
        if (m && now - Module.setStatus.last.time < 30) return;
        Module.setStatus.last.time = now;
        Module.setStatus.last.text = text;
        if (m) {
          progressBar.style.width = (parseInt(m[2]) / parseInt(m[4]) * 100) + '%';
          statusElement.innerHTML = m[1];
        } else {
          progressBar.style.width = text ? progressBar.style.width : '100%';
          statusElement.innerHTML = text;
          if (!text) document.getElementById('progress').style.display = 'none';
        }
      },
      totalDependencies: 0,
      monitorRunDependencies: function(left) {
        this.totalDependencies = Math.max(this.totalDependencies, left);
        Module.setStatus(left
          ? 'Preparing… (' + (this.totalDependencies - left) + '/' + this.totalDependencies + ')'
          : 'All downloads complete.');
      }
    };
    Module.setStatus('Downloading…');
    window.onerror = function(event) {
      Module.setStatus('Exception thrown, see JavaScript console');
      Module.setStatus = function(text) {
        if (text) console.error('[post-exception status] ' + text);
      };
    };
  </script>
  {{{ SCRIPT }}}
</body>
</html>

  '';

  mkApp = {
    appName
  , srcDir
  , dataDir ? null
  , enableSound ? false
  , enableGles2 ? true
  , enableAsyncify ? false   # main path uses app_loop; set true if residual waits freeze
  }:
    pkgs.stdenv.mkDerivation {
      pname = "${appName}-wasm";
      version = "0.1.0";

      dontUnpack = true;
      dontConfigure = true;
      dontUseCmakeConfigure = true;
      nativeBuildInputs = [ pkgs.emscripten pkgs.cmake pkgs.python3 pkgs.pkg-config ];

      env = {
        APP_NAME = appName;
        SRC_DIR = "${srcDir}";
        SDL_WASM_LIBS = sdlWasmLibs;
        ENABLE_SOUND = if enableSound then "1" else "0";
        ENABLE_GLES2 = if enableGles2 then "1" else "0";
        ENABLE_ASYNCIFY = if enableAsyncify then "1" else "0";
        WASM_SHELL = "${wasmShell}";
        PKG_CONFIG_PATH = "${sdlWasmLibs}/lib/pkgconfig";
        ZLIB_WASM_LIBS = zlibWasmLibs;
      } // pkgs.lib.optionalAttrs (dataDir != null) {
        DATA_DIR = "${dataDir}";
      };

      buildPhase = ''
        runHook preBuild
        # Emscripten + pkg-config stubs for our static libs.
        export EM_PKG_CONFIG_PATH="${sdlWasmLibs}/lib/pkgconfig"
        export PKG_CONFIG_PATH="${sdlWasmLibs}/lib/pkgconfig''${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
        bash ${./scripts/build-wasm-app.sh}
        runHook postBuild
      '';

      installPhase = ''
        mkdir -p $out
        for f in ${appName}.html ${appName}.js ${appName}.wasm ${appName}.data; do
          if [ -f "$f" ]; then cp "$f" $out/; fi
        done
        # Fallback: whatever emscripten left in build/
        if [ ! -f $out/${appName}.html ]; then
          find build -maxdepth 1 -type f \( -name '*.html' -o -name '*.js' -o -name '*.wasm' -o -name '*.data' \) \
            -exec cp {} $out/ \; || true
        fi
        ls -la $out
      '';

      meta = with pkgs.lib; {
        description = "SuperTux Milestone 1 (WebAssembly / Emscripten)";
        license = licenses.gpl3Plus;
        platforms = platforms.linux;
      };
    };

  mkOpenBrowserApp = {
    pkg
  , appName
  , description ? "Serve and open the ${appName} wasm build in a browser"
  }: {
    type = "app";
    program = toString (pkgs.writeShellScript "serve-${appName}-wasm" ''
      set -euo pipefail
      cd ${pkg}

      port_file=$(mktemp)
      server_pid=
      trap 'kill "$server_pid" 2>/dev/null || true; rm -f "$port_file"' EXIT

      ${pkgs.python3}/bin/python3 -c '
      import http.server, socketserver, sys
      port_file = sys.argv[1]
      class Quiet(http.server.SimpleHTTPRequestHandler):
          def log_message(self, *a): pass
      socketserver.TCPServer.allow_reuse_address = True
      with socketserver.TCPServer(("127.0.0.1", 0), Quiet) as httpd:
          open(port_file, "w").write(str(httpd.server_address[1]))
          httpd.serve_forever()
      ' "$port_file" &
      server_pid=$!

      for i in $(seq 1 50); do
        [ -s "$port_file" ] && break
        sleep 0.05
      done
      if [ ! -s "$port_file" ]; then
        echo "error: local HTTP server failed to start" >&2
        exit 1
      fi
      port=$(cat "$port_file")
      html="${appName}.html"
      if [ ! -f "$html" ]; then
        html=$(ls -1 *.html 2>/dev/null | head -1 || true)
      fi
      url="http://127.0.0.1:''${port}/''${html}"
      echo "Serving ${appName} at $url  (Ctrl-C to stop)"

      if [ -n "''${BROWSER:-}" ]; then
        "$BROWSER" "$url" >/dev/null 2>&1 || true
      else
        ${pkgs.xdg-utils}/bin/xdg-open "$url" >/dev/null 2>&1 || true
      fi

      wait "$server_pid"
    '');
    meta.description = description;
  };
in {
  inherit sdlWasmLibs zlibWasmLibs mkApp mkOpenBrowserApp;
}
