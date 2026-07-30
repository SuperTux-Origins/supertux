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
  - Note (2026-07-30): this sandbox has **SDL2** runtime only (`libsdl2-2.0-0`), not SDL 1.2 or SDL_image 1.x dev packages. CMake configure for the default SDL1 path cannot be verified here until packages are installed or the SDL2 backend exists.
- [x] Document CMake options in `AGENTS.md` / this file
- [x] Install rules + optional `data/` install when present
- [x] `DATA_PREFIX` cache variable aligned with old `$datadir/supertux-milestone1` idea

### CMake options (implemented)

| Option | Default | Meaning |
|--------|---------|---------|
| `ENABLE_SOUND` | ON | Link SDL_mixer; else `-DNOSOUND` |
| `ENABLE_OPENGL` | ON | Allow GL path; else `-DNOOPENGL` |
| `ENABLE_SDL2` | OFF | Select SDL2 backend when implemented |
| `DATA_PREFIX` | `${CMAKE_INSTALL_PREFIX}/share/supertux-milestone1` | Compile-time data root |

Out of scope for initial CMake (legacy Autotools-only unless requested): GP2X, RES320X240, TSCONTROL, gprof, forced static link.

---

## Phase 2 — Platform abstraction (prepare SDL2 without breaking SDL1)

Goal: one CMake switch (`ENABLE_SDL2`) picks backend implementations; **gameplay code stays free of SDL-version ifdefs**.

### Design

Introduce a thin layer under something like `src/platform/` (names flexible):

1. **Video / window**
   - Init/shutdown, create window/surface or renderer, present (`flipscreen` / `updatescreen`), fullscreen toggle.
2. **Surfaces / images**
   - Load via SDL_image (1.x or 2.x), pixel access as needed by current `SurfaceSDL` / OpenGL upload.
3. **Events / input**
   - Poll events into existing handling or a small translated event struct so `menu.cpp` / `player.cpp` are not littered with SDL2 event-type renames.
4. **Time**
   - `SDL_GetTicks` / delay (behavior is similar across versions).
5. **Audio**
   - Open device, load chunks/music, play; wrap Mix_Chunk / Mix_Music differences if any.

Existing `Surface` / `SurfaceImpl` already separates SDL vs OpenGL **render** paths. Extend that idea:

- Keep `SurfaceSDL` as the software implementation **for SDL1**.
- Add `SurfaceSDL2` (or adapt blit to SDL2 textures/renderer) behind the same `SurfaceImpl` interface.
- Window creation moves out of scattered `SDL_SetVideoMode` into `platform` init used by `st_video_setup_*`.

### Tasks

- [ ] Sketch API headers (`platform_video.h`, `platform_events.h`, `platform_audio.h` or single `platform.h`)
- [ ] Move SDL1 window/surface init from `setup.cpp` into `platform_sdl1.cpp`
- [ ] Route `flipscreen` / `clearscreen` / low-level blit through platform where practical
- [ ] SDL1 audio open/load remains default implementation of audio interface
- [ ] CMake: `ENABLE_SDL2=OFF` builds only SDL1 backend objects
- [ ] Ensure full game still links and runs on SDL1 with data present

---

## Phase 3 — SDL2 backend

- [ ] Implement `platform_sdl2.cpp` (window, renderer or streaming texture from software framebuffer — choose one strategy and document it in AGENTS)
- [ ] SDL2_image load path
- [ ] Event mapping (key/joy/mouse/quit; text input if needed)
- [ ] SDL2_mixer integration
- [ ] OpenGL with SDL2 context creation (if `ENABLE_OPENGL`) — or defer GL-on-SDL2 to a follow-up
- [ ] CMake `ENABLE_SDL2=ON` finds SDL2 packages and compiles SDL2 backend only
- [ ] Fix behavioral regressions (vsync, fullscreen, scaling on HiDPI)
- [ ] Verify title demo, level play, worldmap, menus, leveleditor smoke paths

### Preferred rendering strategy (decision pending)

- **A.** Maintain software `SDL_Surface` framebuffer, upload to SDL2 texture each frame (closest to current SDL software path; less invasive).
- **B.** Migrate drawing to `SDL_Renderer` / textures (more work, cleaner long-term).

Default recommendation for first SDL2 bring-up: **A**, then optional **B**.

---

## Phase 4 — Cleanup

- [ ] Remove or gate dead GP2X-only branches if they obstruct the platform layer
- [ ] Drop Autotools from “supported” docs once CMake+SDL1 is verified
- [ ] Align `VERSION` everywhere (defines, desktop file, CMake project version)
- [ ] Optional: continuous integration compile job (SDL1 and SDL2)

---

## Known constraints

- **No `data/` in this tree** — cannot fully playtest without external assets.
- Codebase is pre-C++11 style; platform layer should not force a style rewrite of the entire game.
- Sound sources are already conditionally listed in Autotools; mirror that in CMake.

---

## Completed log

| Date | Item |
|------|------|
| 2026-07-30 | AGENTS.md, TODO.md, initial CMakeLists.txt |
