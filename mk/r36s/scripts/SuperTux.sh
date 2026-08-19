#!/bin/bash
# SuperTux launcher for R36S / ArkOS (PortMaster-friendly).
# Adapted from Pingus / Windstille handheld launchers — valid SuperTux options only.
DIR="$(cd "$(dirname "$0")" && pwd)"
BIN="$DIR/supertux-origins"
if [ ! -x "$BIN" ]; then BIN="$DIR/bin/supertux-origins"; fi
if [ ! -x "$BIN" ]; then
  echo "SuperTux binary not found next to SuperTux.sh" >&2
  exit 1
fi
# Fixed handheld geometry; fullscreen. Controller profile can be passed via
# --controller if a data/controller/*.scm is shipped with the port.
exec "$BIN" \
  --fullscreen \
  --geometry 640x480 \
  "$@"
