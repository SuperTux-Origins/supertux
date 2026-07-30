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

## Project goals (read this first)

This is a **historical** codebase. The point of work here is **not** to modernize gameplay, redesign levels, add features, or “improve” the 2004 game design.

**In scope**

- Get the game **running reliably on SDL2** (and keep SDL 1.2 building until that is proven).
- Fix **crashes**, null dereferences, and other hard failures (especially in loaders, video/surface lifetime, and timers).
- Keep the platform layer thin so gameplay files stay free of SDL-version spaghetti.

**Out of scope**

- Gameplay balance, new enemies, level-design features, camera “feel”, UI redesign.
- Investing in legacy targets (`GP2X`, `RES320X240`, `TSCONTROL`) unless explicitly requested.
- Large refactors or C++ modernization for their own sake.

When unsure, prefer the smallest change that stops a crash or unblocks SDL2 playtest. Track concrete work in `TODO.md`.

## Project state (high level)

| Area | Status |
|------|--------|
| Original Autotools build | Present (`configure.ac`, `Makefile.am`, `autogen.sh`) |
| **CMake build** | **Done** — root `CMakeLists.txt`; Autotools retained for reference |
| Graphics / input | SDL 1.2 + optional OpenGL; software path via `SurfaceSDL` |
| Audio | SDL_mixer 1.x (optional `-DNOSOUND`); GP2X uses a separate path |
| Platforms of historical interest | Desktop Linux/Windows, experimental GP2X, 320×240 test build |
| SDL2 port | **Compiles** via platform layer + `platform_config.h`; playtest still open |

**Version:** the only source of truth is the top-level `VERSION` file (e.g. `0.1.5-dev`). CMake reads it into `PROJECT_VERSION_FULL` and defines `SUPERTUX_MILESTONE1_VERSION` for the sources. Nix appends `+g<shortRev>` when packaging. Use `--version` to print it. For a release, drop the `-dev` suffix, commit, and tag `vX.Y.Z`.

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
nix build .#supertux-milestone1-sdl2  # or .#default / .#supertux-milestone1  # or .#sdl2 — SDL2
nix develop                           # SDL1 dev shell
nix develop .#sdl2  # or default                    # SDL2 dev shell
```

Both flake packages use **CMake** (not Autotools). Win32 zip packages remain SDL1-oriented via existing win32 SDL inputs.

### Platform layer

Video init/present goes through `src/platform.h` (`platform_sdl1.cpp` or `platform_sdl2.cpp`). SDL2 uses strategy A (window surface + `SDL_UpdateWindowSurface`). Compatibility helpers live in `platform_config.h` (keys, alpha, DisplayFormat, Flip→present, wheel/text helpers).

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

**Nix:** both `nix build .#supertux-milestone1-sdl1` (SDL1) and `.#supertux-milestone1-sdl2` have linked successfully. Runtime playtest of the SDL2 binary is still open.

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
7. **Do not change gameplay or level design** unless fixing a crash or an SDL2 blocker. No balance tweaks, new mechanics, or content work.
8. **Commit message in chat:** After any larger change (new subsystem, multi-file refactor, completed TODO phase, etc.), end the reply with a proposed **git commit message** (subject + optional body). Use imperative mood, explain *why* when non-obvious, and keep the subject ~50–72 characters when practical. Do not run `git commit` unless the user asks.

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
