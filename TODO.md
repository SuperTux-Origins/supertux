# SuperTux Milestone 1 — TODO

Progress tracker for CMake migration and SDL2 port. Update this file as work lands.

**Scope:** make the engine build and run on SDL2; fix crashes and hard failures.  
**Not in scope:** gameplay balance, level-design features, UI redesign, or legacy handheld polish. See `AGENTS.md` → Project goals.

## Legend

- `[ ]` pending
- `[~]` in progress
- `[x]` done
- `[!]` blocked / needs decision

---

## Phase 0 — Documentation & inventory

- [x] Document project state (`AGENTS.md`)
- [x] Create this `TODO.md`
- [x] Note missing `data/` (runtime-only requirement)
- [x] Inventory Autotools options (`configure.ac`: debug, gprof, OpenGL, silence, GP2X, 320x240, touchscreen, static)

---

## Phase 1 — CMake build system

Goal: replace Autotools as the supported way to build; default remains **SDL 1.2**.

- [x] Root `CMakeLists.txt` (project, version, options, executable)
- [x] List all engine sources (including optional sound sources)
- [x] Find SDL, SDL_image; optional SDL_mixer; optional OpenGL
- [x] Define `VERSION`, `DATA_PREFIX`, `NOOPENGL` / `NOSOUND` as appropriate
- [x] Install rules for binary + desktop/icon files (data install optional / documented)
- [x] Smoke-compile SDL1 via `nix build` (CMake + platform_sdl1, warnings only; install succeeded with data/)
  - Note: a log labeled “sdl2” actually showed `-DENABLE_SDL2=OFF` / “Backend = SDL 1.2” — that was the SDL1 package succeeding.
- [x] Document CMake options in `AGENTS.md` / this file
- [x] Install rules + optional `data/` install when present
- [x] `DATA_PREFIX` cache variable aligned with old `$datadir/supertux-milestone1` idea
- [x] `flake.nix` packages for **SDL1** and **SDL2** (CMake-based) + `devShells`

### CMake options

| Option | Default | Meaning |
|--------|---------|---------|
| `ENABLE_SOUND` | ON | Link mixer; else `-DNOSOUND` |
| `ENABLE_OPENGL` | ON | Allow GL path; else `-DNOOPENGL` |
| `ENABLE_SDL2` | OFF | Compile `platform_sdl2.cpp` + `USE_SDL2` |
| `DATA_PREFIX` | `${CMAKE_INSTALL_PREFIX}/share/supertux-milestone1` | Compile-time data root |

### Flake packages

| Attribute | Backend |
|-----------|---------|
| `supertux-milestone1` / `sdl1` / `default` | SDL 1.2 |
| `supertux-milestone1-sdl2` / `sdl2` | SDL2 |
| `devShells.default` | SDL1 tools |
| `devShells.sdl2` | SDL2 tools |

Out of scope for initial CMake: GP2X, RES320X240, TSCONTROL, gprof, forced static link.

---

## Phase 2 — Platform abstraction (prepare SDL2 without breaking SDL1)

Goal: one CMake switch (`ENABLE_SDL2`) picks backend implementations; **gameplay code stays free of SDL-version ifdefs**.

### Design

Thin layer in `src/platform*.{h,cpp}`:

1. **Video / window** — init, caption, present, shutdown
2. **Surfaces / images** — still mostly SDL_Surface + IMG_* (shared API shape)
3. **Events / input** — `ST_GetKeyState` helper in `platform_config.h`; further event mapping TBD
4. **Time** — `SDL_GetTicks` / `SDL_Delay` (compatible)
5. **Audio** — still direct mixer for now (SDL1 vs SDL2 mixer headers selected via USE_SDL2)

### Tasks

- [x] Sketch API headers (`platform.h`, `platform_config.h`)
- [x] SDL1 backend: `platform_sdl1.cpp` (SetVideoMode / Flip / GL swap)
- [x] Route `st_video_setup` / `flipscreen` / `updatescreen` through platform
- [x] CMake: `ENABLE_SDL2` selects `platform_sdl1.cpp` vs `platform_sdl2.cpp`
- [x] SDL2 compatibility shims in `platform_config.h` (SDLKey, DisplayFormat*, SetAlpha, GetKeyState, Flip, UpdateRect, EnableKeyRepeat, WM icon)
- [x] Route headers/sources through `platform_config.h` instead of raw `SDL.h`
- [x] `st_set_color_key` helper for colour-key flag differences
- [x] `platform_set_icon` on both backends
- [x] Ensure full game still links on SDL1 (nix build); runtime playtest still manual
- [x] Smoke-compile ENABLE_SDL2=ON — links and installs (nix build .#supertux-milestone1-sdl2  # or .#default / .#supertux-milestone1)
- [x] Event helpers: `st_key_ascii`, `st_event_wheel_up/down`, `st_event_mouse_xy`
- [x] menu.cpp / button.cpp use event helpers (no keysym.unicode / button 4–5)
- [x] Remaining event edge cases: joy hat bits; SDL2 focus-loss releases player keys; menu TEXTINPUT
- [~] Playtest SDL2 binary — needs `data/`; prior TileManager SEGV hardened (loader + nested merge + IMG_Init)

---

## Phase 3 — SDL2 backend

- [x] Initial `platform_sdl2.cpp` (window + GetWindowSurface / GL context — strategy A)
- [x] Compat layer so most engine files compile against SDL2 headers (shims)
- [x] **Software backbuffer:** own stable `st_backbuffer`; blit to window surface on present
- [x] SDL2_image / SDL2_mixer: `IMG_Init` in platform_sdl2; `Mix_Init` (OGG/MOD) + `Mix_Quit` on close
- [x] Menu / slot text input under SDL2: `SDL_StartTextInput`, `SDL_TEXTINPUT` in menu, `st_key_ascii` shift fallback
- [x] Joy hat: menu + worldmap use `SDL_HAT_*` bit flags; menu hat left/right
- [x] OpenGL-on-SDL2: GL attributes + compatibility profile; letterboxed viewport from drawable size
- [x] Fullscreen / present: FULLSCREEN → FULLSCREEN_DESKTOP → windowed; software letterbox scale; video re-init falls back instead of exit
- [ ] Smoke playtest with `data/`: title demo, one level, worldmap, pause menu, options, quit — SDL1 and SDL2

### Rendering strategy

- **A (updated):** owned software backbuffer + blit to window surface on present.
- **B (later, only if A is unstable):** `SDL_Renderer` / textures.

---

## Phase 3b — Crash / correctness fixes (worth doing)

These are engine bugs that can crash, corrupt timers, or hide bad data. Not gameplay features.

- [x] **`st_get_ticks` while paused** (`timer.cpp`): frozen clock = `st_pause_count - st_pause_ticks` while paused
- [x] **Nested tileset load** (`TileManager::load_tileset`): `replace` flag; nested forms merge without wiping parent tiles
- [x] **`TileManager::get` fallback**: missing ids return null; null-checks in world brick/box and fish water tile
- [x] **CMake OpenGL/GLU link block**: explicit nesting; GLU only for SDL1
- [x] **TSCONTROL:** `old_mouse_y = screen->h` (was `screen->w`)
- [x] **Menu backspace:** do not write before start of empty text field (`delete_character` / `i > 0`)

---

## Phase 4 — Cleanup

- [ ] Remove or gate dead GP2X-only branches **only if** they obstruct the platform layer or SDL2 build
- [ ] Drop Autotools from “supported” docs once CMake+SDL1 is verified
- [x] Single-source `VERSION` file → CMake `PROJECT_VERSION_FULL` / `SUPERTUX_MILESTONE1_VERSION`; flake appends `+g<rev>`
- [ ] Optional: CI compile job (SDL1 and SDL2)
- [x] Gameloop: missing `break` after menu-mode KEYDOWN (fell through to joy start)
- [x] Gameloop: release movement keys on SDL2 focus loss / minimize

---

## Known constraints

- **No `data/` in this tree** — cannot fully playtest without external assets.
- Codebase is pre-C++11 style; platform layer should not force a style rewrite of the entire game.
- SDL2 port: core video/present + header shims in place; needs runtime testing with `data/`.
- Fixed 15-row level height and other Milestone 1 limits are **by design** — do not expand for content reasons.
- Music may leak across transitions (`music_manager` / SDL_mixer lifetime); fix only if it causes crashes or severe resource exhaustion during normal play.

---

## Explicitly out of scope (do not put on this list)

- New gameplay mechanics, enemy behavior changes, scoring tweaks
- Level or worldmap design / new tiles for design reasons
- Camera “feel”, back-scroll polish, UI/UX redesign
- Rewriting the engine in modern C++, or Renderer migration without a crash/SDL2 need
- Supporting or enhancing GP2X / 320×240 / touchscreen unless requested

---

## Completed log

| Date | Item |
|------|------|
| 2026-07-30 | AGENTS.md, TODO.md, initial CMakeLists.txt |
| 2026-07-30 | Platform layer sketch; setup/screen use platform_present |
| 2026-07-30 | flake.nix: CMake builds, `sdl1` + `sdl2` packages and devShells |
| 2026-07-30 | SDL2 compat shims; headers use platform_config; color-key helper |
| 2026-07-30 | Event helpers for text input + mouse wheel; video re-init safety |
| 2026-07-30 | Confirmed SDL1 nix build success; libpng/jpeg in flake; texture return warnings |
| 2026-07-30 | Fix SDL2: CaptureScreen, format->alpha, const keystate |
| 2026-07-30 | SDL2 build success; silence format/fread warnings |
| 2026-07-30 | Harden TileManager/IMG_Init after SDL2 runtime SEGV |
| 2026-07-31 | Scope: SDL2 + crash fixes only; filed Phase 3b issues; AGENTS goals section |
| 2026-07-31 | Fix pause ticks; nested tileset merge; tile get null; CMake GL link; SDL2 backbuffer |
| 2026-07-31 | SDL2 text input, Mix_Init, joy-hat bits, menu backspace, TSCONTROL mouse y |
| 2026-07-31 | SDL2 fullscreen fallbacks, letterbox present, GL viewport, safe video re-init |
| 2026-07-31 | Fix KEYDOWN fallthrough; SDL2 focus-loss key release; VERSION 0.1.4 |
| 2026-07-31 | VERSION file as sole source; CMake/flake/SUPERTUX_MILESTONE1_VERSION |
