#!/usr/bin/env bash
# Serve a SuperTux wasm build directory over HTTP and open a browser.
# Env: APP_NAME (default: supertux-origins), PKG (package dir with *.html)
#      SUPERTUX_WASM_PORT (default 8765), SUPERTUX_WASM_OPEN_QUERY, BROWSER
# Usage: nix run .#supertux-wasm [-- --debug] [-- --verbose]
set -euo pipefail

cli_query=()
while [ "$#" -gt 0 ]; do
  case "$1" in
    --debug)   cli_query+=("debug=1"); shift ;;
    --verbose|-v) cli_query+=("verbose=1"); shift ;;
    --help|-h)
      echo "Usage: serve.sh [--debug] [--verbose]"
      echo "  --debug    open with ?debug=1"
      echo "  --verbose  open with ?verbose=1"
      echo "Env: APP_NAME, PKG, SUPERTUX_WASM_PORT, SUPERTUX_WASM_OPEN_QUERY, BROWSER"
      exit 0
      ;;
    --) shift; break ;;
    -*)
      echo "error: unknown option: $1 (try --help)" >&2
      exit 1
      ;;
    *)
      echo "error: unexpected argument: $1 (try --help)" >&2
      exit 1
      ;;
  esac
done

if [ -n "${PKG:-}" ]; then
  cd "$PKG"
fi

app_name="${APP_NAME:-supertux-origins}"
port="${SUPERTUX_WASM_PORT:-${PINGUS_WASM_PORT:-8765}}"

port_file=$(mktemp)
server_pid=
trap 'kill "$server_pid" 2>/dev/null || true; rm -f "$port_file"' EXIT

python3 -c '
import http.server, socketserver, sys
port_file, port = sys.argv[1], int(sys.argv[2])

class NoCacheHandler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0")
        self.send_header("Pragma", "no-cache")
        self.send_header("Expires", "0")
        super().end_headers()
    def log_message(self, *a):
        pass

socketserver.TCPServer.allow_reuse_address = True
try:
    httpd = socketserver.TCPServer(("127.0.0.1", port), NoCacheHandler)
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
html="${app_name}.html"
if [ ! -f "$html" ]; then
  html=$(ls -1 *.html 2>/dev/null | head -1 || true)
fi
if [ -z "$html" ] || [ ! -f "$html" ]; then
  echo "error: no HTML shell found in $(pwd) (expected ${app_name}.html)" >&2
  ls -la >&2 || true
  exit 1
fi

bust=$(date +%s)
parts=("v=${bust}")
extra="${SUPERTUX_WASM_OPEN_QUERY:-${PINGUS_WASM_OPEN_QUERY:-}}"
if [ -n "$extra" ]; then
  parts+=("$extra")
fi
if [ "${#cli_query[@]}" -gt 0 ]; then
  parts+=("${cli_query[@]}")
fi
q=$(IFS='&'; echo "${parts[*]}")
url="http://127.0.0.1:${port}/${html}?${q}"

echo "Serving SuperTux wasm at $url  (Ctrl-C to stop)"
echo "  Cache-Control: no-store on all responses; ?v=… busts document cache."
echo "  IDBFS origin is tied to this host:port — keep the port stable to retain saves."

if [ -n "${BROWSER:-}" ]; then
  "$BROWSER" "$url" >/dev/null 2>&1 || true
elif command -v xdg-open >/dev/null 2>&1; then
  xdg-open "$url" >/dev/null 2>&1 || true
fi

wait "$server_pid"
