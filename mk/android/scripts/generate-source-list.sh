#!/usr/bin/env bash
# Regenerate mk/android/app/jni/supertux_sources.{list,mk} from the repo tree.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
OUT_DIR="$ROOT/mk/android/app/jni"
cd "$ROOT"
find src -name '*.cpp' | sort > "$OUT_DIR/supertux_sources.list"
{
  echo '# Auto-generated — run mk/android/scripts/generate-source-list.sh'
  echo 'SUPERTUX_SOURCES := \'
  while read -r f; do
    printf '  ../../../../%s \\\n' "$f"
  done < "$OUT_DIR/supertux_sources.list"
  echo '  # (end)'
} > "$OUT_DIR/supertux_sources.mk"
echo "Wrote $(wc -l < "$OUT_DIR/supertux_sources.list") sources to $OUT_DIR"
