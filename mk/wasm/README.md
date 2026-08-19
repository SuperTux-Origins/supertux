# WebAssembly packaging (SuperTux Origins)

Origins already has:

- `if(EMSCRIPTEN)` USE_FLAGS in `CMakeLists.txt` (FULL_ES2, exceptions,
  GROWABLE_ARRAYBUFFERS=0, FORCE_FILESYSTEM, preload `@/data`)
- `mk/emscripten/template.html.in` shell

This directory adds Pingus-style helper scripts for optional offline static
SDL builds and a simple static file server.

## Primary path

```bash
nix build .#supertux-wasm
```

Uses `emscriptenStdenv` + CMake EMSCRIPTEN path. Currently marked broken
until static wasm builds of physfs / squirrel / wstsound / tinycmmc land
(or CMake soft-disables them under EMSCRIPTEN).

## Scripts

- `scripts/build-app.sh` — emcmake orchestration (defaults: `supertux-origins`)
- `scripts/build-sdl2*.sh` / `build-zlib.sh` — offline static ports
- `scripts/serve.sh` — local HTTP server for the HTML/JS/WASM output
