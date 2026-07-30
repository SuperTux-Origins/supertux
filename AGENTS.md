# SuperTux Milestone 1 — Agent Notes

## What this is

This tree is **SuperTux Milestone 1** (classic 0.1.x-era codebase), a Super Mario–inspired 2D platformer starring Tux. Upstream history is from the early 2000s (SourceForge era). The game is GPL-2.0.

This working copy is **source-only for the engine**. The large `data/` tree (levels, images, music, sounds, tilesets) is **not** shipped in this archive because of size. You need a matching Milestone 1 `data/` directory at runtime (or point `DATA_PREFIX` / `datadir` at one).

Canonical layout once data is present:

```text
data/
  images/   levels/   music/   sounds/   ...
src/        engine sources
```

## Project state (high level)

| Area | Status |
|------|--------|
| Original Autotools build | Present (`configure.ac`, `Makefile.am`, `autogen.sh`) |
| **CMake build** | **In progress / primary target** — see root `CMakeLists.txt` |
| Graphics / input | SDL 1.2 + optional OpenGL; software path via `SurfaceSDL` |
| Audio | SDL_mixer 1.x (optional `-DNOSOUND`); GP2X uses a separate path |
| Platforms of historical interest | Desktop Linux/Windows, experimental GP2X, 320×240 test build |
| SDL2 port | Planned; must keep SDL1 workable via a **backend layer**, not mass `#ifdef`s |

Version string in Autotools is `0.1.4` (`configure.ac`); `defines.h` falls back to `"0.1.1"` if `VERSION` is unset. CMake defines `VERSION` consistently.

## Architecture sketch

- **Entry**: `src/supertux.cpp` → directory setup, parse args, audio/video/joystick, menus, then title / level / leveleditor.
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
#   -DENABLE_SOUND=ON|OFF
#   -DENABLE_SDL2=ON|OFF   # OFF = SDL 1.2 (default); ON = SDL2 platform backend
#   -DDATA_PREFIX=/path/to/data-parent   # install/runtime data root
```

Binary name: `supertux-milestone1`.

### Nix flake

```bash
nix build .#supertux-milestone1       # or .#sdl1 / default — SDL 1.2
nix build .#supertux-milestone1-sdl2  # or .#sdl2 — SDL2
nix develop                           # SDL1 dev shell
nix develop .#sdl2                    # SDL2 dev shell
```

Both flake packages use **CMake** (not Autotools). Win32 zip packages remain SDL1-oriented via existing win32 SDL inputs.

### Platform layer

Video init/present goes through `src/platform.h` (`platform_sdl1.cpp` or `platform_sdl2.cpp`). SDL2 uses strategy A (window surface + `SDL_UpdateWindowSurface`). See `TODO.md` Phase 2–3 for remaining API migration (`SDL_Flip`, `SDL_WM_*`, key state, …).

Runtime still expects assets under `DATA_PREFIX` / discovered `datadir` (see `st_directory_setup()`). Without `data/`, the binary will not be playable.

Autotools remain in the tree for reference but are not the maintained path forward.

## Dependencies

**SDL1 path (current default):**

- C++98-ish codebase; a modern C++ compiler is fine
- SDL 1.2 development libraries
- SDL_image 1.x
- SDL_mixer 1.x (unless `ENABLE_SOUND=OFF`)
- OpenGL (optional)
- zlib, libpng, libjpeg (often pulled via SDL_image)

**SDL2 path (target):** SDL2, SDL2_image, SDL2_mixer — wired through a compatibility/backend layer, not by rewriting every call site at once.

**This sandbox:** only SDL2 runtime was observed installed; SDL 1.2 *development* packages are required to verify the default CMake build until Phase 3 lands.

## Coding guidelines for agents

1. **Track work in `TODO.md`** — check items off, add notes when blocked.
2. **Keep SDL1 building** until the SDL2 backend is selected at CMake time and proven.
3. **No spaghetti `#ifdef USE_SDL2`** across gameplay files. Isolate differences in:
   - video init / window / present
   - surface/texture upload and blit
   - event translation
   - audio open/load/play
4. Do not require `data/` to *compile*; do document that it is required to *run*.
5. Prefer clear, minimal diffs. This is a historical codebase — match local style (tabs/spaces as in neighboring files) unless reforming a whole module.
6. GP2X / 320×240 paths are legacy; do not invest in them unless explicitly requested. CMake may omit those options initially.
7. **Commit message in chat:** After any larger change (new subsystem, multi-file refactor, completed TODO phase, etc.), end the reply with a proposed **git commit message** (subject + optional body). Use imperative mood, explain *why* when non-obvious, and keep the subject ~50–72 characters when practical. Do not run `git commit` unless the user asks.

## Key source map

| Path | Role |
|------|------|
| `src/supertux.cpp` | `main` |
| `src/setup.cpp` | paths, video/audio/joystick init, CLI |
| `src/platform.h` / `platform_sdl1.cpp` / `platform_sdl2.cpp` | SDL version backends |
| `src/screen.cpp` / `texture.cpp` | drawing backends |
| `src/gameloop.cpp` | level session |
| `src/world.cpp` / `player.cpp` / `badguy.cpp` | gameplay |
| `src/worldmap.cpp` | overworld |
| `src/menu.cpp` / `title.cpp` | UI |
| `src/sound.cpp` / `music_manager.cpp` | audio |
| `src/lispreader.cpp` | data parsing |

## License

GNU GPL v2 (see `COPYING`). Preserve license headers on source files.
