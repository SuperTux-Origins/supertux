# Parked: Autotools + GP2X (legacy)

This directory holds the **historical Autotools build** and the **GP2X cross wrappers**.  
It is **not** the supported build path. Desktop, Windows, Android, and WASM use **CMake** and the root `flake.nix`.

Do not invest in this tree unless explicitly requested. A full CMake/flake GP2X port is deferred indefinitely.

## What was moved here

| Path | Role |
|------|------|
| `configure.ac` | Autoconf: SDL 1.2 desktop + optional GP2X / 320×240 / touchscreen / static / silence / OpenGL |
| `Makefile.am`, `src/Makefile.am` | Automake source lists (incomplete vs current `src/` — see below) |
| `autogen.sh` | `aclocal` / `automake` / `autoconf` bootstrap (`WANT_AUTOMAKE=1.6`) |
| `autoconf/*.m4` | Bundled macros: `sdl.m4`, `ax_check_gl.m4`, `libmikmod.m4`, `acx_pthread.m4` |
| `gp2x_configure` | Cross-`configure` with hardcoded Open2X / “official” ARM toolchain paths |
| `gp2x_make` | Same PATH setup, then `make` |
| `autopackage/default.apspec` | Obsolete Autopackage (Berlios-era) desktop package spec |

## Value retained in `configure.ac` (already covered by CMake)

| Flag / check | CMake equivalent |
|--------------|------------------|
| `--enable-debug` → `-DDEBUG -O0 -g3` | `-DENABLE_DEBUG=ON` |
| `--disable-opengl` → `-DNOOPENGL` | `-DENABLE_OPENGL=OFF` |
| `--enable-silence` → `-DNOSOUND` | `-DENABLE_SOUND=OFF` |
| SDL / SDL_image / SDL_mixer / zlib / jpeg / png | `pkg_check_modules` + `find_package(ZLIB)` |
| `DATA_PREFIX` → `$datadir/supertux-milestone1` | `-DDATA_PREFIX=…` |
| `--enable-gprof` (`-pg`) | Not ported (out of scope) |
| `--enable-static` | Not ported as a first-class option |

## GP2X-only bits (not in CMake)

These are the only reasons to keep this tree at all:

- `--enable-gp2x` → `-DGP2X` (input/sound `#ifdef` paths in engine sources)
- With sound: **libmikmod** via `AM_PATH_LIBMIKMOD` + `MikMod_Init` (GP2X music path, not SDL_mixer MOD)
- **SDL_gfx** (`rotozoomSurface`) required for GP2X builds
- `--enable-320x240` → `-DRES320X240`
- `--enable-touchscreen` → `-DTSCONTROL` (legacy; modern touch is `touch_controls.cpp` under Android/SDL2)
- `gp2x_configure` / `gp2x_make`: example cross PATH +  
  `./configure --enable-320x240 --enable-gp2x --disable-opengl --enable-silence --host=arm-linux …`

Toolchain prefixes in the wrappers (`/aesop/cross/…`, `/opt/open2x/…`) are historical machine paths; they will not work without local edits.

## Why this is not a drop-in build anymore

1. **Source list is stale.** `src/Makefile.am` lacks modern units (`platform_sdl1.cpp` / `platform_sdl2.cpp`, `touch_controls.cpp`, `app_loop.cpp`, `gles2_renderer.cpp`, `game_file.cpp`, …). CMakeLists.txt is the source of truth.
2. **Layout.** `AC_CONFIG_SRCDIR([src/supertux.cpp])` and `SUBDIRS = src data` assume the Autotools files live at the **repo root** next to `src/` and `data/`. This parked copy does not rewire those paths; restoring a working Autotools build would mean either symlinking back to the root or rewriting `configure.ac` / makefiles for an out-of-tree layout.
3. **`data/Makefile.am`** was referenced by `AC_OUTPUT` but is not shipped in this archive.
4. **Autopackage** is dead upstream; keep only as archaeology.

## If you must bootstrap (unsupported)

From a **full** source tree that still has these files at the root (or after restoring that layout):

```bash
./autogen.sh
./gp2x_configure   # edit CMD_PREFIX / CMD_HOST first
./gp2x_make
```

Or plain desktop SDL 1.2:

```bash
./autogen.sh
./configure --disable-opengl   # example
make
```

Preferred today:

```bash
cmake -S . -B build -DENABLE_SDL2=ON
cmake --build build
# or: nix build .#supertux-milestone1-sdl2
```
