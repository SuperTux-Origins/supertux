# PORTS.md — SuperTux multi-platform packaging

High-level packaging map. Platform quirks live in [PORTING.md](PORTING.md).
Process and standards live in [AGENTS.md](AGENTS.md). Tasks in [TODO.md](TODO.md).

Recipes adapted from [Pingus](https://github.com/Pingus/pingus) and
[Windstille](https://github.com/WindstilleTeam/windstille).

## Graphics targets

| Platform       | API            | Notes                          |
|----------------|----------------|--------------------------------|
| Desktop Linux  | OpenGL 3.3     | GLEW via `ProvideOpenGL.cmake` |
| Windows MinGW  | OpenGL         | grumnix prebuilts              |
| WebAssembly    | WebGL (ES2)    | `ENABLE_OPENGLES2=ON`          |
| Android        | GLES2          | NDK, SDLActivity               |
| R36S / ArkOS   | GLES2          | Hybrid toolchain + sysroot     |

## Flake outputs (planned / current)

| Output | Status | Path |
|--------|--------|------|
| `supertux-origins` | Working (desktop) | `supertux-origins.nix` |
| `supertux-origins-win32` / `-zip` | Working (packaging) | `flake.nix` |
| `supertux-wasm` | Scaffold (`broken`) | `nix/wasm.nix` |
| `supertux-android` / APK | Scaffold | `nix/android.nix` + `mk/android/` |
| `arkos-sysroot` | Scaffold (placeholder URL) | `nix/r36s.nix` |
| `supertux-r36s` / PortMaster | Scaffold | `nix/r36s.nix` + `mk/r36s/` |

## Directory map

```
mk/
  cmake/SuperTux/     # Provide* modules, version, install
  emscripten/         # template.html.in, icons, patches
  wasm/               # build-app.sh, offline SDL scripts, serve.sh
  android/            # ndk-build scripts (app/ tree still TODO)
  r36s/               # toolchain cmake, sysroot helpers, cxxabi shim
nix/
  wasm.nix            # emscriptenStdenv package
  wasm-pingus-reference.nix
  android.nix         # reusable mkApk / SDL lib builders
  r36s.nix            # hybrid aarch64 + ArkOS sysroot
```

## Dependency strategy by platform

- **Desktop / Windows**: system or grumnix packages (SDL2, image, ttf, physfs,
  curl, glew, squirrel, tinycmmc family, wstsound).
- **WASM**: Emscripten `-sUSE_SDL=2` / image / freetype ports for the core;
  static wasm builds still required for physfs, squirrel, wstsound, tinycmmc
  (or CMake soft-disable). See PORTING.md.
- **Android**: NDK-built SDL2 (+ image), GLES2, stb_image if needed; NDK r27+.
- **R36S**: ArkOS sysroot libs (SDL2, GLES, …) + static libgcc; header-only
  glm from nixpkgs; no desktop Mesa/GLEW.

## Build examples (once green)

```bash
nix build .#supertux-origins
nix build .#supertux-origins-win32-zip
nix build .#supertux-wasm          # currently broken meta
nix build .#supertux-r36s          # needs published sysroot tarball
```
