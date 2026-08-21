#!/bin/bash
# Compatibility wrapper — prefer SuperTux-Origins.sh
DIR="$(cd "$(dirname "$0")" && pwd)"
exec "$DIR/SuperTux-Origins.sh" "$@"
