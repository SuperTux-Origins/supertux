# SuperTux Milestone 1

A classic Super Mario–inspired 2D platformer starring **Tux**. Run, jump, collect coins, stomp enemies, and rescue Penny from Nolok’s fortress.

This tree is the **Milestone 1** (0.1.x-era) game, kept close to the original design, with a modern port layer so it runs on current desktops and Android.

## What’s new in this port

- **SDL2** backend (SDL 1.2 still available)
- **OpenGL ES 2.0** path for mobile / embedded GPUs
- **On-screen touch controls** on Android (fixed layout; independent of keyboard remaps)
- **Android Back** treated like Escape in menus and gameplay
- Game data can load from the **APK assets** (no extract-to-disk required)

Gameplay, levels, and balance stay the Milestone 1 experience.

## Playing

| Action | Default keys |
|--------|----------------|
| Move | Arrow keys |
| Jump | Space |
| Duck | Down |
| Run / shoot | Left Ctrl |
| Pause | P |
| Menu / Back | Escape |

Controls can be changed under **Options**. On a phone or tablet, use the on-screen pad (d-pad, Jump, Action, Menu).

Options are saved between runs. For a full list of command-line flags, run:

```bash
supertux-milestone1 --help
```

## Build with Nix

From the repo root (with [Nix](https://nixos.org/) and flakes enabled):

```bash
# Desktop — SDL2 (default)
nix build .#supertux-milestone1-sdl2
# or: nix build

# Desktop — SDL 1.2
nix build .#supertux-milestone1-sdl1

# Desktop — SDL2 + OpenGL ES 2.0
nix build .#supertux-milestone1-sdl2-gles2

# Dev shells (same names as packages)
nix develop                                  # SDL2
nix develop .#supertux-milestone1-sdl1
nix develop .#supertux-milestone1-sdl2-gles2
```

The binary is `result/bin/supertux-milestone1`.

## Build the Android APK

```bash
nix build .#supertux-milestone1-android
# → result/supertux-milestone1-<date>-<rev>.apk
```

Repo-root `data/` is packaged into the APK as assets. Install with:

```bash
nix run .#install-android-supertux-milestone1   # adb install -r
```

Target baseline is roughly **Fire OS 5 / API 22** (`armeabi-v7a` + `arm64-v8a`), with GLES2 as the default renderer.

## CMake (without Nix)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_SDL2=ON
cmake --build build
```

Useful options: `ENABLE_SOUND`, `ENABLE_OPENGL`, `ENABLE_GLES2`, `ENABLE_SDL2`.

## Project home

**https://github.com/SuperTux-Origins/supertux-milestone1**

This is a maintained port of the classic SuperTux Milestone 1 (0.1.x) game. The original early-2000s project lived at [super-tux.sf.net](http://super-tux.sf.net/).

## License

GNU **GPL-3.0-or-later**. See `COPYING` and the REUSE headers in the sources.

## WebAssembly (browser)

```bash
nix build .#supertux-milestone1-wasm
nix run .#supertux-milestone1-wasm    # local HTTP server + open browser
```

Also: `nix build .#wasm-sdl-libs` and `nix build .#wasm-zlib-libs` for the
static dependency prefixes. The game uses SDL2 + GLES2/WebGL, preloads `data/`
into MEMFS at `/data`, and stores config/saves in IDBFS under `/home/web_user`.
Sound is off by default; the level editor is not offered in the web build.

