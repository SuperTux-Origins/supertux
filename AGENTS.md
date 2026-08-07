# SuperTux Milestone 1 — Agent Notes

## What this is

This tree is **SuperTux Milestone 1** (classic 0.1.x-era codebase), a Super Mario–inspired 2D platformer starring Tux. Upstream history is from the early 2000s (SourceForge era). The game is GPL-3.0-or-later (REUSE SPDX headers in source files).

`data/` (levels, images, music, sounds, tilesets) is **part of the source tree**. Some agent uploads omit it only to save size; treat a missing `data/` in the working copy as incomplete packaging, not as an optional external dependency.

Canonical layout:

```text
data/
  images/   levels/   music/   sounds/   ...
src/        engine sources
```

## Project goals (read this first)

This is a **historical** codebase. The point of work here is **not** to modernize gameplay, redesign levels, add features, or “improve” the 2004 game design.

**In scope**

- Get the game **running reliably on SDL2** (and keep SDL 1.2 building until that is proven).
- Fix **crashes**, null dereferences, and other hard failures (especially in loaders, video/surface lifetime, and timers).
- Keep the platform layer thin so gameplay files stay free of SDL-version spaghetti.

**Out of scope**

- Gameplay balance, new enemies, level-design features, camera “feel”, UI redesign.
- Large refactors or C++ modernization for their own sake.
- Legacy `TSCONTROL` (old touchscreen path); modern touch is `touch_controls.cpp`.

**Handheld (GP2X / Wiz):** SDL 1.2 is required (Open2x/OpenWiz). See `mk/gp2x/CROSSCOMPILE.md`.
CMake: `-DENABLE_GP2X=ON -DENABLE_RES320X240=ON` + Open2x/OpenWiz toolchain file.

**Handheld (R36S / ArkOS):** RK3326, 640×480, Ubuntu 19.10-based userspace. Prefer SDL2 + GLES2 (no `RES320X240`). Device-compatible builds link against an ArkOS sysroot (glibc ~2.30) — see `mk/r36s/CROSSCOMPILE.md`. Flake: `nix build .#supertux-milestone1-r36s` (sysroot-linked aarch64) and `.#supertux-milestone1-r36s-portmaster` (PortMaster tree for `/roms/ports`).

When unsure, prefer the smallest change that stops a crash or unblocks SDL2 playtest. Track concrete work in `TODO.md`.

## Project state (high level)

| Area | Status |
|------|--------|
| Original Autotools build | **Parked** under `mk/gp2x/` (reference only) |
| **CMake build** | **Done** — root `CMakeLists.txt` (supported path) |
| Graphics / input | SDL 1.2 + optional OpenGL (immediate) or GLES2 (shaders); software via `SurfaceSDL` |
| Audio | SDL_mixer 1.x (optional `-DNOSOUND`); GP2X historically mikmod |
| GP2X / Wiz | **In progress** — CMake `ENABLE_GP2X` + toolchains; see `mk/gp2x/CROSSCOMPILE.md` |
| R36S / ArkOS | **Working** — `mk/r36s/` toolchains + `.#supertux-milestone1-r36s` (sysroot-linked) + `.#supertux-milestone1-r36s-portmaster` |
| SDL2 port | **Compiles** via platform layer + `platform_config.h`; playtest still open |

**Version:** the only source of truth is the top-level `VERSION` file (e.g. `0.1.5-dev`). CMake reads it into `PROJECT_VERSION_FULL` and defines `SUPERTUX_MILESTONE1_VERSION` for the sources. Nix appends `+g<shortRev>` when packaging. Use `--version` to print it. For a release, drop the `-dev` suffix, commit, and tag `vX.Y.Z`.

## Architecture sketch

- **Entry**: `src/supertux.cpp` → directory setup, parse args, audio/video/joystick, menus, then `app_run()` (unified frame pump) or CLI level / leveleditor.
- **Video**: `setup.cpp` chooses SDL software or OpenGL (`use_gl`). Drawing goes through `Surface` / `SurfaceImpl` (`texture.cpp`) and helpers in `screen.cpp`.
- **Game**: `World` + `Player` + `BadGuy` + tiles (`tile.cpp`) + collision (`collision.cpp`). Session loop in `gameloop.cpp`.
- **World map**: `worldmap.cpp` (separate from in-level `World`).
- **Data format**: Lisp-like level/worldmap files via `lispreader.cpp`.
- **Config / saves**: under a user save dir (`st_save_dir`); slots and worldmap `.stsg` files.

Important globals: `screen` (`SDL_Surface*`), `datadir`, `use_gl`, `use_fullscreen`, joystick keymap — see `globals.h`.

Existing compile-time switches (legacy):

- `NOOPENGL` — force software rendering
- `NOSOUND` — strip audio
- `GP2X` — handheld-specific input/sound
- `RES320X240` — low-res layout
- `TSCONTROL` — touchscreen-ish controls
- `DEBUG` — debug messages / surface checks

Prefer **not** to grow this set for SDL1 vs SDL2. New dual-backend work should go behind a small platform API (see TODO).

## Build (CMake — preferred)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
# optional:
#   -DENABLE_OPENGL=ON|OFF
#   -DENABLE_GLES2=ON|OFF  # ES 2.0 shader path (implies OpenGL+SDL2)
#   -DENABLE_SOUND=ON|OFF
#   -DENABLE_SDL2=ON|OFF   # OFF = SDL 1.2 (default); ON = SDL2 platform backend
#   -DDATA_PREFIX=/path/to/data-parent   # install/runtime data root
```

Binary name: `supertux-milestone1`.

### Nix flake

```bash
nix build .#supertux-milestone1-sdl2        # or .#default — SDL2
nix build .#supertux-milestone1-sdl1        # SDL 1.2
nix build .#supertux-milestone1-sdl2-gles2  # SDL2 + OpenGL ES 2.0
nix build .#wasm-sdl-libs                   # SDL2 (+ image) static for wasm32
nix build .#wasm-zlib-libs                  # static zlib for wasm32 (lispreader)
nix build .#supertux-milestone1-wasm        # Emscripten HTML/JS/Wasm
nix run .#supertux-milestone1-wasm          # serve over HTTP + open browser
nix build .#supertux-milestone1-r36s        # aarch64 SDL2+GLES2 (ArkOS sysroot; see mk/r36s/)
nix build .#supertux-milestone1-r36s-portmaster  # PortMaster tree for /roms/ports/
nix develop                                 # SDL2 (matches default package)
nix develop .#supertux-milestone1-sdl1
nix develop .#supertux-milestone1-sdl2-gles2
```

Both flake packages use **CMake** (not Autotools). Win32 zip packages remain SDL1-oriented via existing win32 SDL inputs.

**Unified frame pump (`app_loop`):** Desktop and WebAssembly share the same top-level state machine in `src/app_loop.cpp` (`APP_SCREEN_TITLE` / `WORLDMAP` / `SESSION` / `CONFIRM` / `TEXT`). `app_run()` drives it — via `emscripten_set_main_loop` on wasm, or a desktop `while (app_frame)` paced by `st_frame_delay`. Nested `title()` / `GameSession::run()` / `WorldMap::display()` remain for CLI level start and the level editor only. Hand-offs use `app_request_*` when `app_loop_active()`. Blocking wrappers (`confirm_dialog`, `display_text_file`, `save_hs`) refuse under the pump.

**WebAssembly:** `nix/wasm.nix` + `mk/wasm/scripts/` builds offline SDL2 (+ SDL2_image) via `emcmake` (same flake `sdl2-src` / `sdl2-image-src` inputs as Android — **not** Emscripten’s network `-sUSE_SDL=2` port). Game flags: `ENABLE_SDL2=ON`, `ENABLE_GLES2=ON` (WebGL); `ENABLE_SOUND` optional (mixer+libxmp in `wasm-sdl-libs`, default off until playtested). Offline static **zlib** is `wasm-zlib-libs` (`ZLIB_ROOT`). Under wasm, `st_frame_delay()` no-ops while `app_loop_active()` so `requestAnimationFrame` paces the tab. **ASYNCIFY** is **off** by default (`ENABLE_ASYNCIFY=1` only as a safety net if a residual wait freezes the tab). Assets preload to `/data`; config/saves use **IDBFS** under `/home/web_user`. Level editor is hidden on wasm.

### Platform layer

Video init/present goes through `src/platform.h` (`platform_sdl1.cpp` or `platform_sdl2.cpp`). SDL2 uses strategy A (window surface + `SDL_UpdateWindowSurface`). Compatibility helpers live in `platform_config.h` (keys, alpha, DisplayFormat, Flip→present, wheel/text helpers).

Runtime resolves assets under `DATA_PREFIX` / discovered `datadir` (see `st_directory_setup()`), normally the repo-root `data/` tree.

GP2X/Wiz cross-compile: `mk/gp2x/CROSSCOMPILE.md` + `toolchain-open2x.cmake` /
`toolchain-openwiz.cmake`. Autotools under `mk/gp2x/` remain reference-only.

R36S/ArkOS: `mk/r36s/CROSSCOMPILE.md` + `toolchain-arkos-aarch64.cmake` (and
optional `toolchain-arkos-armhf.cmake`). Sysroot from device, Debian Buster
debootstrap, or the published tarball used by flake `.#supertux-milestone1-r36s`
(sysroot-linked; not modern nixpkgs glibc).

## Dependencies

**SDL1 path (current default):**

- C++98-ish codebase; a modern C++ compiler is fine
- SDL 1.2 development libraries
- SDL_image 1.x
- SDL_mixer 1.x (unless `ENABLE_SOUND=OFF`)
- OpenGL (optional)
- zlib, libpng, libjpeg (often pulled via SDL_image)

**SDL2 path (target):** SDL2, SDL2_image, SDL2_mixer — wired through a compatibility/backend layer, not by rewriting every call site at once.

**GLES2 path (optional / Android prep):** with `-DENABLE_GLES2=ON` (implies SDL2 + OpenGL), video requests an ES 2.0 context and drawing goes through `src/gles2_renderer.cpp` (textured/solid quads via GLSL ES 1.00). Desktop immediate-mode GL remains the default when GLES2 is off. Link against `GLESv2`.

**Nix:** `.#supertux-milestone1-sdl1`, `.#supertux-milestone1-sdl2` (default), and `.#supertux-milestone1-sdl2-gles2` are the desktop packages. Dev shells use the same attribute names. Wasm: `.#wasm-sdl-libs`, `.#wasm-zlib-libs`, `.#supertux-milestone1-wasm` (package + `nix run` app).

## Coding guidelines for agents

1. **Track work in `TODO.md`** — check items off, add notes when blocked.
2. **Keep SDL1 building** until the SDL2 backend is selected at CMake time and proven.
3. **No spaghetti `#ifdef USE_SDL2`** across gameplay files. Isolate differences in:
   - video init / window / present
   - surface/texture upload and blit
   - event translation
   - audio open/load/play
4. `data/` belongs in the tree. Do not invent alternate data-discovery schemes; runtime still uses `DATA_PREFIX` / `datadir` as today.
5. Prefer clear, minimal diffs. This is a historical codebase — match local style (tabs/spaces as in neighboring files) unless reforming a whole module.
6. GP2X / Wiz: use CMake `ENABLE_GP2X` + `ENABLE_RES320X240` and the Open2x/OpenWiz
   toolchain files; keep SDL1. Do not revive Autotools for new work.
   R36S/ArkOS: SDL2 + GLES2, **no** `RES320X240`; link against an ArkOS-compatible
   sysroot for on-device runs (`mk/r36s/`).
7. **Do not change gameplay or level design** unless fixing a crash or an SDL2 blocker. No balance tweaks, new mechanics, or content work.
8. **Commit message in chat:** After any larger change (new subsystem, multi-file refactor, completed TODO phase, etc.), end the reply with a proposed **git commit message** (subject + optional body). Use imperative mood, explain *why* when non-obvious, and keep the subject ~50–72 characters when practical. Do not run `git commit` unless the user asks.
9. **Diagnose before fixing — no speculative hacks.** If the root cause is not confirmed by logs, a reproducible test, or a clear code path, **do not** “fix” by papering over symptoms (extra guards, random hints, dead-zone tweaks, `#ifdef` workarounds). Instead:
   - Add **targeted debug output** (`st_vlog` / `SDL_Log`) at the decision points that must fire for the bug to occur (input events, menu actions, init flags, open/close of devices).
   - On Android, `verbose_mode` is forced on so `st_vlog` already goes to logcat; prefer that over inventing a parallel logging path.
   - Re-run, collect logcat (or desktop stderr), and only then land a minimal fix tied to what the logs show.
   - Harmless resource hygiene (e.g. closing a joystick you opened but rejected) is fine; theory-driven behavior changes without evidence are not.

## Key source map

| Path | Role |
|------|------|
| `src/supertux.cpp` | `main` |
| `src/setup.cpp` | paths, video/audio/joystick init, CLI |
| `src/platform.h` / `platform_sdl1.cpp` / `platform_sdl2.cpp` | SDL version backends |
| `src/gl_compat.h` | OpenGL vs GLES2 includes |
| `src/gles2_renderer.cpp` | ES 2.0 shader quads (when `USE_GLES2`) |
| `src/screen.cpp` / `texture.cpp` | drawing backends |
| `src/gameloop.cpp` | level session |
| `src/world.cpp` / `player.cpp` / `badguy.cpp` | gameplay |
| `src/worldmap.cpp` | overworld |
| `src/menu.cpp` / `title.cpp` | UI |
| `src/sound.cpp` / `music_manager.cpp` | audio |
| `src/lispreader.cpp` | data parsing |
| `mk/android/app/` | Android packaging (manifest, jni/Android.mk, icons) |
| `nix/android.nix` + `mk/android/scripts/` | APK pipeline (scripts usable outside Nix) |
| `nix/wasm.nix` + `mk/wasm/scripts/` | Emscripten app + static SDL/zlib recipes |
| `mk/r36s/` + `nix/r36s.nix` | R36S/ArkOS aarch64 (and armhf) cross notes + flake package |

### Android APK

```bash
nix build .#supertux-milestone1-android
# -> result/supertux-milestone1-<date>-<rev>.apk
nix run .#install-android-supertux-milestone1   # adb install -r
```

- SDL2 is built once as `android-sdl-libs` (ndk-build); the game links it as a prebuilt.
- SDL2_image is compiled into `libmain.so` with the stb backend (no system libpng).
- **SDL2_mixer** is built into `android-sdl-libs` (OGG via in-tree stb_vorbis) and linked from `libmain.so`. Force silence with `SUPER_TUX_ENABLE_SOUND=0` in `mk/android/app/jni/Android.mk` if needed.
- **GLES2 is the default renderer** on Android (`USE_GLES2`, linked `-lGLESv2`). ES 2.0 is required by the Android CDD and available on API 22 / Fire OS 5. Software fallback remains if context creation fails.
- The repo-root `data/` tree is packaged into the APK as assets.
- Target baseline matches Fire OS 5 / API 22 (`armeabi-v7a` + `arm64-v8a`).

## License

GNU GPL v3 or later (see `COPYING`). Source files use REUSE SPDX headers (`SPDX-FileCopyrightText` / `SPDX-License-Identifier: GPL-3.0-or-later`).
