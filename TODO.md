# SuperTux Milestone 1 — TODO

Progress tracker for CMake migration and SDL2 port. Update this file as work lands.

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
- [ ] Smoke-compile on a machine with SDL1.2 dev packages
  - Note (2026-07-30): this sandbox has **SDL2** runtime only; SDL 1.2 *dev* packages needed for local verify of default backend.
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
- [ ] Replace remaining `SDL_GetKeyState` / `SDL_WM_*` / `SDL_Flip` call sites via platform or `ST_*` helpers
- [ ] Ensure full game still links and runs on SDL1 with data present
- [ ] Event translation layer if SDL2 event field differences bite

---

## Phase 3 — SDL2 backend

- [x] Initial `platform_sdl2.cpp` (window + GetWindowSurface / GL context — strategy A)
- [ ] Finish software path: drawing code assumptions vs window surface format
- [ ] SDL2_image / SDL2_mixer compile + runtime check
- [ ] Event mapping (key/joy/mouse/quit; `SDL_GetKeyboardState`)
- [ ] OpenGL-on-SDL2 polish (shadow surface vs real FBO)
- [ ] Fix regressions (vsync, fullscreen, HiDPI)
- [ ] Verify title demo, level play, worldmap, menus, leveleditor smoke paths

### Rendering strategy

- **A (chosen for first bring-up):** software `SDL_Surface` via `SDL_GetWindowSurface`, present with `SDL_UpdateWindowSurface`.
- **B (later):** `SDL_Renderer` / textures.

---

## Phase 4 — Cleanup

- [ ] Remove or gate dead GP2X-only branches if they obstruct the platform layer
- [ ] Drop Autotools from “supported” docs once CMake+SDL1 is verified
- [ ] Align `VERSION` everywhere (defines, desktop file, CMake project version)
- [ ] Optional: CI compile job (SDL1 and SDL2)

---

## Known constraints

- **No `data/` in this tree** — cannot fully playtest without external assets.
- Codebase is pre-C++11 style; platform layer should not force a style rewrite of the entire game.
- SDL2 port is incomplete: many files still include `<SDL.h>` assuming 1.2 symbols (`SDL_WM_SetCaption`, `SDL_Flip`, …). Compile with `-DENABLE_SDL2=ON` will expose those; fix by routing through platform / `platform_config.h`.

---

## Completed log

| Date | Item |
|------|------|
| 2026-07-30 | AGENTS.md, TODO.md, initial CMakeLists.txt |
| 2026-07-30 | Platform layer sketch; setup/screen use platform_present |
| 2026-07-30 | flake.nix: CMake builds, `sdl1` + `sdl2` packages and devShells |
