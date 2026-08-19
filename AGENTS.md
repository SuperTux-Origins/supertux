# AGENTS.md — SuperTux porting project

## Project goal

Port SuperTux (SuperTux-Origins fork at tip a1fd30f11 (and later porting commits))
to:

- **Desktop Linux** (OpenGL 3.3 + GLEW)
- **WebAssembly / Emscripten** (WebGL / GLES2 path)
- **Android** (NDK, GLES2)
- **R36S / ArkOS** (aarch64, GLES2, hybrid toolchain + old sysroot)
- **Windows (MinGW)** (cross from Linux)

High-level packaging and flake outputs should mirror the structure used by
[Pingus](https://github.com/Pingus/pingus) and
[Windstille](https://github.com/WindstilleTeam/windstille)
(`mk/`, `nix/`, platform-specific scripts, `linuxPorts` / checks hygiene).

## Key reference documents

- **PORTING.md** — platform-specific quirks, workarounds, library breakdown,
  and lessons learned while porting (image codecs, GLES vs desktop GL,
  controller input, R36S sysroot, Emscripten flags, Android NDK macros,
  flake check surface, etc.). Read this before inventing new solutions.
- **PORTS.md** — high-level packaging map and flake outputs.
- **TODO.md** — open tasks and progress tracking.
- Recipes adapted from Pingus / Windstille `mk/` and `nix/`.

## Coding / process standards

- Author for all commits: `Ingo Ruhnke <grumbel@gmail.com>`
- Trailer on every commit: `Co-authored-by: Grok <grok@x.ai>`
- Code is delivered as **git bundles** that stack cleanly.
- Bundle naming: `supertux-001-…`, `supertux-002-…`, … (never reuse numbers).
- Bundles use `HEAD` as the ref.
- Prefer small, task-focused commits; one large bundle at the end of a work unit.
- Prefer clean code over quick hacks. If complexity grows, refactor.
- Nix store is read-only; after copying from the store, fix permissions.
- Double-check Nix string quoting and escaping.
- Do not reinvent the wheel: copy and adapt Pingus/Windstille `mk/` and `nix/`
  files, then adjust for SuperTux specifics (CMake options, data layout,
  GLES2/OpenGL 3.3 targets, library set).

## Graphics targets

| Platform          | API          |
|-------------------|--------------|
| Desktop Linux     | OpenGL 3.3   |
| Android / R36S    | GLES2        |
| WebAssembly       | WebGL (ES2)  |
| Windows MinGW     | OpenGL (desktop path) |

## Library overlap summary (see PORTING.md for detail)

**Common across SuperTux / Pingus / Windstille**

- SDL2 (+ image / ttf variants)
- OpenAL
- OpenGL / GLES2 (+ GLEW or equivalent on desktop)
- FreeType
- GLM
- libogg / libvorbis (and extra codecs in Windstille)
- zlib
- squirrel (scripting)
- sexpcpp / tinycmmc ecosystem (logmich, strutcpp, priocpp, …)

**SuperTux-specific or heavier**

- PhysFS
- libcurl (add-ons)
- SDL2_ttf, SDL2_image (system or grumnix-win32)
- Boost (legacy bits; Origins moves toward tinycmmc)
- libraqm / harfbuzz / fribidi (complex text)
- Bundled savepng, partio_zip, tinygettext, obstack

**Windstille / Pingus portable path**

- Header-only **stb_image** instead of system libjpeg/libpng for constrained
  targets (Android, R36S, wasm).
- Custom `wstdisplay` / `wstinput` / `wstsound` layers with explicit GLES2
  switches.
- Hybrid aarch64 toolchain + ArkOS sysroot for R36S.
- Emscripten flags: exceptions, GROWABLE_ARRAYBUFFERS=0, FULL_ES2, FORCE_FILESYSTEM.

When porting SuperTux, prefer the same patterns: conditional system codecs,
GameController-first input, clean EGL defaults, flake outputs that are only
derivations under `packages`/`checks`.

## Working practices

1. Keep PORTING.md updated with every non-obvious fix.
- **PORTS.md** — high-level packaging map and flake outputs.
2. Keep TODO.md current.
3. After each logical unit of work, produce a numbered git bundle.
4. Compare CMake options, data install paths, and controller profiles with
   the corresponding Pingus/Windstille files before inventing new ones.
