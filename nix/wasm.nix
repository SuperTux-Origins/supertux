{ pkgs
, sdlSrc
, sdlVersion
, sdlImageSrc ? null
, sdlMixerSrc ? null
, libxmpSrc ? null
}:

let
  lib = pkgs.lib;

  # Shared install helper bits for a single-component prefix under $PWD/prefix.
  installPrefixPhase = ''
    runHook preInstall
    mkdir -p $out
    if [ -d prefix ]; then
      cp -a prefix/. $out/
    else
      mkdir -p $out/lib $out/include
    fi
    runHook postInstall
  '';

  # --- SDL2 only -----------------------------------------------------------
  sdl2WasmLibs = pkgs.stdenv.mkDerivation {
    pname = "sdl2-wasm";
    version = sdlVersion;
    dontUnpack = true;
    dontConfigure = true;
    dontUseCmakeConfigure = true;
    nativeBuildInputs = [ pkgs.emscripten pkgs.cmake pkgs.python3 ];
    env = {
      SDL_SRC = "${sdlSrc}";
    };
    buildPhase = ''
      runHook preBuild
      bash ${./scripts/build-wasm-sdl-libs.sh}
      runHook postBuild
    '';
    installPhase = ''
      runHook preInstall
      mkdir -p $out
      if [ -d prefix ]; then
        cp -a prefix/. $out/
      else
        mkdir -p $out/lib $out/include
        find . -name 'libSDL2.a' -exec cp {} $out/lib/ \;
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

  # --- SDL2_image (depends on sdl2WasmLibs) --------------------------------
  sdl2Image = if sdlImageSrc == null then null else pkgs.stdenv.mkDerivation {
      pname = "sdl2-image-wasm";
      version = "2.8.2";
      dontUnpack = true;
      dontConfigure = true;
      dontUseCmakeConfigure = true;
      nativeBuildInputs = [ pkgs.emscripten pkgs.cmake pkgs.python3 ];
      env = {
        SDL_PREFIX = "${sdl2WasmLibs}";
        SDL_IMAGE_SRC = "${sdlImageSrc}";
      };
      buildPhase = ''
        runHook preBuild
        bash ${./scripts/build-wasm-sdl-libs.sh}
        runHook postBuild
      '';
      installPhase = ''
        runHook preInstall
        mkdir -p $out/lib $out/include $out/lib/pkgconfig
        if [ -d prefix ]; then
          # Prefer only image artifacts if present; still copy tree for cmake configs.
          cp -a prefix/. $out/
          # Drop SDL2 core copies if the script staged any (should not with SDL_PREFIX).
          rm -f $out/lib/libSDL2.a $out/lib/libSDL2main.a 2>/dev/null || true
        fi
        if [ ! -f $out/lib/libSDL2_image.a ]; then
          find . -name 'libSDL2_image.a' -exec cp {} $out/lib/ \; || true
        fi
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

  # --- SDL2_mixer + libxmp (depends on sdl2WasmLibs) -----------------------
  sdl2Mixer = if sdlMixerSrc == null then null else pkgs.stdenv.mkDerivation {
      pname = "sdl2-mixer-wasm";
      version = "2.8.0";
      dontUnpack = true;
      dontConfigure = true;
      dontUseCmakeConfigure = true;
      nativeBuildInputs = [ pkgs.emscripten pkgs.cmake pkgs.python3 ];
      env = {
        SDL_PREFIX = "${sdl2WasmLibs}";
        SDL_MIXER_SRC = "${sdlMixerSrc}";
      } // lib.optionalAttrs (libxmpSrc != null) {
        LIBXMP_SRC = "${libxmpSrc}";
      };
      buildPhase = ''
        runHook preBuild
        bash ${./scripts/build-wasm-sdl-libs.sh}
        runHook postBuild
      '';
      installPhase = ''
        runHook preInstall
        mkdir -p $out/lib $out/include $out/lib/pkgconfig
        if [ -d prefix ]; then
          cp -a prefix/. $out/
          rm -f $out/lib/libSDL2.a $out/lib/libSDL2main.a 2>/dev/null || true
        fi
        for lib in libSDL2_mixer.a libxmp.a; do
          if [ ! -f $out/lib/$lib ]; then
            find . -name "$lib" -exec cp {} $out/lib/ \; || true
          fi
        done
        if [ -f $out/lib/libSDL2_mixer.a ]; then
          xmp_libs=""
          if [ -f $out/lib/libxmp.a ]; then xmp_libs=" -lxmp"; fi
          cat > $out/lib/pkgconfig/SDL2_mixer.pc <<EOF
prefix=$out
libdir=\''${prefix}/lib
includedir=\''${prefix}/include
Name: SDL2_mixer
Description: SDL2_mixer (wasm static)
Version: 2.8.0
Requires: sdl2
Libs: -L\''${libdir} -lSDL2_mixer$xmp_libs
Cflags: -I\''${includedir}
EOF
        fi
        if [ -f $out/lib/libxmp.a ]; then
          cat > $out/lib/pkgconfig/libxmp.pc <<EOF
prefix=$out
libdir=\''${prefix}/lib
includedir=\''${prefix}/include
Name: libxmp
Description: libxmp (wasm static)
Version: 4.6.0
Libs: -L\''${libdir} -lxmp
Cflags: -I\''${includedir}
EOF
        fi
        runHook postInstall
      '';
    };

  # Combined prefix for the game (and legacy attribute name).
  sdlWasmLibs = pkgs.symlinkJoin {
    name = "sdl2-wasm-libs-${sdlVersion}";
    paths =
      [ sdl2WasmLibs ]
      ++ lib.optional (sdl2Image != null) sdl2Image
      ++ lib.optional (sdl2Mixer != null) sdl2Mixer;
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


  # HTML shell with source/revision footer. versionFull / gitRev filled by mkApp.
  mkWasmShell = { versionFull, gitRev, sourceUrl }:
    let
      revUrl =
        if gitRev == "dirty" || gitRev == "" then sourceUrl
        else "${sourceUrl}/tree/${gitRev}";
    in
    pkgs.writeText "supertux-wasm-shell.html" ''
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no">
  <title>SuperTux Milestone 1 ${versionFull}</title>
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
      max-width: 100vw; max-height: 85vh;
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
    #meta {
      font-size: 0.75rem; opacity: 0.7; text-align: center;
      line-height: 1.5; max-width: 90vw;
    }
    #meta a { color: #8cf; }
  </style>
</head>
<body>
  <div id="wrap">
    <canvas class="emscripten" id="canvas" oncontextmenu="event.preventDefault()" tabindex="-1"></canvas>
    <div id="status">Loading…</div>
    <div id="progress"><div id="progress-bar"></div></div>
    <div id="meta">
      SuperTux Milestone 1 <strong>${versionFull}</strong><br>
      <a href="${sourceUrl}" target="_blank" rel="noopener">Source</a>
      ·
      <a href="${revUrl}" target="_blank" rel="noopener">Revision ${gitRev}</a>
    </div>
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
  , versionFull ? "0.1.5-dev"
  , gitRev ? "dirty"
  , sourceUrl ? "https://github.com/SuperTux-Origins/supertux-milestone1"
  }:
    let
      shell = mkWasmShell { inherit versionFull gitRev sourceUrl; };
    in
    pkgs.stdenv.mkDerivation {
      pname = "${appName}-wasm";
      version = versionFull;

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
        PROJECT_VERSION_FULL = versionFull;
        WASM_SHELL = "${shell}";
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

      # Fixed port so IndexedDB (IDBFS) keeps the same origin across runs.
      # Override with SUPERTUX_WASM_PORT=… if 8765 is busy.
      port="''${SUPERTUX_WASM_PORT:-8765}"

      port_file=$(mktemp)
      server_pid=
      trap 'kill "$server_pid" 2>/dev/null || true; rm -f "$port_file"' EXIT

      ${pkgs.python3}/bin/python3 -c '
      import http.server, socketserver, sys
      port_file, port = sys.argv[1], int(sys.argv[2])
      class Quiet(http.server.SimpleHTTPRequestHandler):
          def log_message(self, *a): pass
      socketserver.TCPServer.allow_reuse_address = True
      try:
          httpd = socketserver.TCPServer(("127.0.0.1", port), Quiet)
      except OSError as e:
          sys.stderr.write(
              "error: cannot bind 127.0.0.1:%s (%s)\n"
              "       set SUPERTUX_WASM_PORT to a free port\n" % (port, e))
          sys.exit(1)
      open(port_file, "w").write(str(httpd.server_address[1]))
      httpd.serve_forever()
      ' "$port_file" "$port" &
      server_pid=$!

      for i in $(seq 1 50); do
        [ -s "$port_file" ] && break
        sleep 0.05
      done
      if [ ! -s "$port_file" ]; then
        echo "error: local HTTP server failed to start on port $port" >&2
        exit 1
      fi
      port=$(cat "$port_file")
      html="${appName}.html"
      if [ ! -f "$html" ]; then
        html=$(ls -1 *.html 2>/dev/null | head -1 || true)
      fi
      url="http://127.0.0.1:''${port}/''${html}"
      echo "Serving ${appName} at $url  (Ctrl-C to stop)"
      echo "  IDBFS origin is tied to this host:port — keep the port stable to retain saves."

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
  inherit sdl2WasmLibs sdlWasmLibs zlibWasmLibs mkApp mkOpenBrowserApp;
  inherit sdl2Image sdl2Mixer;
}
