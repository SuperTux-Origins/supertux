# Shared helpers for mk/*/scripts (source from those scripts).
# Project-agnostic — no SuperTux-specific paths.

nproc_jobs() {
  echo "${NIX_BUILD_CORES:-${JOBS:-$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1)}}"
}

need_cmd() {
  local c
  for c in "$@"; do
    command -v "$c" >/dev/null 2>&1 || {
      echo "error: required command not found: $c" >&2
      exit 1
    }
  done
}

require_env() {
  local v
  for v in "$@"; do
    if [ -z "${!v:-}" ]; then
      echo "error: required environment variable unset: $v" >&2
      exit 1
    fi
  done
}

# Resolve SDL2 lib/include from PREFIX or SDL_PREFIX.
sdl2_paths_from_prefix() {
  local p="$1"
  if [ -f "$p/lib/libSDL2.a" ]; then
    SDL2_LIB="$p/lib/libSDL2.a"
  else
    echo "error: libSDL2.a not found under $p/lib" >&2
    exit 1
  fi
  if [ -f "$p/include/SDL2/SDL.h" ]; then
    SDL2_INC="$p/include/SDL2"
  elif [ -f "$p/include/SDL.h" ]; then
    SDL2_INC="$p/include"
  else
    SDL2_INC="$p/include/SDL2"
  fi
}
