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
| `ENABLE_GLES2` | OFF | OpenGL ES 2.0 shader path (`USE_GLES2`); forces OpenGL+SDL2 |
| `ENABLE_SDL2` | OFF | Compile `platform_sdl2.cpp` + `USE_SDL2` |
| `DATA_PREFIX` | `${CMAKE_INSTALL_PREFIX}/share/supertux-milestone1` | Compile-time data root |

### Flake packages

| Attribute | Backend |
|-----------|---------|
| `supertux-milestone1-sdl2` / `default` | SDL2 |
| `supertux-milestone1-sdl1` | SDL 1.2 |
| `supertux-milestone1-sdl2-gles2` | SDL2 + OpenGL ES 2.0 |
| `devShells.*` | same attribute names as packages |

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
- [x] **GLES2 optional path** (`ENABLE_GLES2`): ES 2.0 context, `gles2_renderer` shader quads, texture upload without `GL_UNPACK_ROW_LENGTH` / `GL_RGB10_A2`; desktop immediate-mode GL retained when GLES2 is off
- [ ] Smoke playtest with `data/`: title demo, one level, worldmap, pause menu, options, quit — SDL1, SDL2, and GLES2
- [x] Wire Android APK to GLES2 renderer (default; `USE_GLES2` + `-lGLESv2`; software fallback)

### Rendering strategy

- **A (updated):** owned software backbuffer + blit to window surface on present.
- **B (later, only if A is unstable):** `SDL_Renderer` / textures.
- **C (new):** GLES2 shader path for textured/solid quads — preparation for Android GL; optional on Linux via `-DENABLE_GLES2=ON`.

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
| 2026-07-31 | Silence -Wformat-truncation in dsubdirs/dfiles (PATH_MAX + path_join) |
| 2026-07-31 | Android APK target: nix/android.nix, android/, SDL2+image stb, NOSOUND/NOOPENGL |
| 2026-07-31 | GLES2 renderer: ENABLE_GLES2, gles2_renderer, gl_compat.h; flake gles2 package |
| 2026-07-31 | Android default renderer: GLES2 (Android.mk USE_GLES2, manifest glEsVersion 0x20000) |
| 2026-07-31 | Android menu stuck-DOWN: open (not assumed joystick); menu/joy debug logs; AGENTS diagnose-first |
| 2026-07-31 | Touch→logical letterbox map; no StartTextInput at boot; Android fullscreen theme |

---

## Phase 5 — WebAssembly (Emscripten)

Goal: ship a browser build patterned on the helloworld-fireos reference
(`apk/` in the agent workspace): offline SDL2 static libs, `emcmake` of the
existing CMake tree, `nix run .#supertux-milestone1-wasm` serves over HTTP.

**In scope:** compile + present title/level with SDL2 + GLES2/WebGL; package
`data/` via `--preload-file` when present.  
**Out of scope:** gameplay changes; full mixer/MOD stack on day one; replacing
ASYNCIFY with a perfect main-loop refactor before first paint.

### Design (mirrors `apk/nix/wasm.nix`)

| Piece | Role |
|-------|------|
| `nix/wasm.nix` | `sdlWasmLibs` + `mkApp` + `mkOpenBrowserApp` |
| `nix/scripts/build-wasm-sdl-libs.sh` | emcmake SDL2 (+ SDL2_image) static |
| `nix/scripts/build-wasm-app.sh` | emcmake game; ASYNCIFY + WebGL link flags |
| CMake `SDL2_ROOT` / `EMSCRIPTEN` | skip pkg-config system GLES; `.html` suffix |
| Flake | `wasm-sdl-libs`, `supertux-milestone1-wasm` package + app |

Nested loops (`title()`, `GameSession::run()`, `WorldMap::display()`) use
blocking `while` + `SDL_Delay`. Browsers need either:

1. **ASYNCIFY** (temporary, current default in the wasm link line), or  
2. A single frame callback (`emscripten_set_main_loop`) — preferred long-term.

### Tasks

- [x] Add `nix/wasm.nix` + `build-wasm-sdl-libs.sh` + `build-wasm-app.sh`
- [x] CMake: `SDL2_ROOT`, Emscripten link options, no system GLESv2 on wasm
- [x] Flake packages: `wasm-sdl-libs`, `supertux-milestone1-wasm`
- [x] Flake app: `supertux-milestone1-wasm` (local HTTP server + open browser)
- [x] `nix build .#wasm-sdl-libs` — static `libSDL2.a` + `libSDL2_image.a` (explicit PrivateSDL2 paths)
- [x] `nix build .#supertux-milestone1-wasm` — full link OK (html/js/wasm + preloaded .data)
- [x] Runtime datadir on wasm: `/data` preload + `emscripten_prepare_paths`;
      `open_game_file` tries `/data/<rel>` on MEMFS
- [x] First browser smoke: canvas paints (even without full `data/`)
- [x] With `data/`: title screen → start game → one level → quit
- [x] IDBFS for config/saves under `/home/web_user` (survive reload)
- [x] Replace ASYNCIFY with real frame pump (title / session / worldmap / confirm / text)
  - [x] Extract `GameSession::frame()` / `WorldMap::frame()` (busy loops call them)
  - [x] Extract `title_frame()` (+ init/shutdown); `title()` calls the loop
  - [x] Top-level `app_loop` state machine (TITLE / WORLDMAP / SESSION) + `emscripten_set_main_loop`
  - [x] Hand-off from load-game + worldmap enter-level (no nested run/display when `app_loop_active`)
  - [x] Contrib subset levels via `app_request_session(subset, levelnb, ST_GL_PLAY)`
  - [~] Frame remaining dialogs (confirm, credits, high scores, editor)
    - [x] `confirm_dialog_begin/frame/result`
    - [x] `display_text_file_begin/frame/end` (credits / intro)
    - [x] Wire app_loop screens for confirm + credits (no nested calls)
    - [x] High scores frame helpers (`save_hs_begin/frame`)
    - [x] Hide level editor on Emscripten; skip intro under app_loop
  - [x] Drop ASYNCIFY by default (`enableAsyncify = false`); re-enable if residual waits freeze
    - [x] `st_frame_delay()` no-op under `app_loop_active()`
- [ ] Optional: SDL2_mixer + formats for wasm (or keep `ENABLE_SOUND=OFF`)
- [x] Document `nix build` / `nix run` wasm targets in `AGENTS.md` / README

### Flake attributes (Phase 5)

| Attribute | Meaning |
|-----------|---------|
| `wasm-sdl-libs` | Cached SDL2 (+ image) static wasm libs |
| `supertux-milestone1-wasm` | package: `.html` / `.js` / `.wasm` (+ `.data` if preloaded) |
| `apps.supertux-milestone1-wasm` | serve package over HTTP and open browser |

### Known blockers

- **No `data/` in thin trees** — preload skipped; runtime will miss assets.
- **Main loops** — ASYNCIFY is a stopgap; stack size may need tuning.
- **Audio** — first wasm target is `ENABLE_SOUND=OFF` until mixer is ported.
- **C++98 + emscripten** — should work; watch for missing POSIX (`access`, paths).

---

| Date | Item |
|------|------|
| 2026-08-03 | Phase 5 scaffold: wasm.nix, scripts, CMake EMSCRIPTEN/SDL2_ROOT, flake targets |
| 2026-08-03 | Emscripten datadir/userdir + open_game_file /data; SDL wasm install prefix |
| 2026-08-03 | Fix wasm SDL2_image: pass SDL2_LIBRARY/INCLUDE_DIR to PrivateSDL2 finder |
| 2026-08-03 | First successful `nix build .#supertux-milestone1-wasm` (zlib via emconfigure; ASYNCIFY; data preload) |
| 2026-08-03 | IDBFS mount + sync on startup / saveconfig / worldmap savegame |
| 2026-08-03 | Extract GameSession::frame + WorldMap::frame (ASYNCIFY exit path) |
| 2026-08-03 | Extract title_frame / title_init / title_shutdown |
| 2026-08-03 | app_loop: Emscripten TITLE/WORLDMAP/SESSION state machine + main_loop |
| 2026-08-03 | Split wasm zlib into flake output `wasm-zlib-libs` |
| 2026-08-03 | app_request_session(subset, levelnb, mode); contrib subset non-blocking |
| 2026-08-03 | Extract confirm_dialog + display_text_file frame helpers |
| 2026-08-03 | Emscripten: no level editor menu; skip intro; save_hs frames |
| 2026-08-03 | st_frame_delay; optional ENABLE_ASYNCIFY=0 for wasm link |
| 2026-08-03 | ASYNCIFY off by default (mkApp enableAsyncify=false) |
| 2026-08-03 | Custom wasm HTML shell; non-blocking wait_for_event/fade under app_loop |
| 2026-08-03 | IDBFS syncfs without Asyncify.handleSleep (ASYNCIFY-off safe) |
