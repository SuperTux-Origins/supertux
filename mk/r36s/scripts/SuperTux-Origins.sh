#!/bin/bash
# SuperTux Origins launcher for R36S / ArkOS (PortMaster-friendly).
# Adapted from Pingus / Windstille handheld launchers.
DIR="$(cd "$(dirname "$0")" && pwd)"
BIN="$DIR/supertux-origins"
if [ ! -x "$BIN" ]; then BIN="$DIR/bin/supertux-origins"; fi
if [ ! -x "$BIN" ]; then
  echo "SuperTux Origins binary not found next to SuperTux-Origins.sh" >&2
  exit 1
fi
exec "$BIN" \
  --fullscreen \
  --geometry 640x480 \
  "$@"
