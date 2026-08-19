# TODO — SuperTux multi-platform port

## Immediate

- [ ] Finish obtaining a complete, usable source tree at
      856a932f513ec69ffbd7132a1a60fd89c79442dc (fuse FS / large data/ makes
      clone+checkout extremely slow; tarball extract also partial).
- [ ] Library dependency matrix written (done in PORTING.md / AGENTS.md).
- [x] Create AGENTS.md and PORTING.md skeleton documenting process and
      Windstille/Pingus lessons.
- [ ] Audit existing SuperTux-Origins `flake.nix` against current Pingus /
      Windstille flake patterns (stb_image, GLES packages, linuxPorts hygiene,
      R36S/Android/wasm outputs).
- [ ] Inventory `mk/` and `nix/` that already exist in Origins vs what must be
      copied/adapted from Pingus/Windstille.

## Desktop Linux

- [ ] Ensure OpenGL 3.3 + GLEW path builds and runs under the flake.
- [ ] Flake `packages` / `checks` only contain derivations.
- [ ] Optional: desktop GLES2 validation build (like Windstille `windstille-gles2`).

## WebAssembly (Emscripten)

- [ ] Adapt `mk/wasm/` scripts from Pingus/Windstille (build-app.sh, SDL
      static builds, zlib, shell.html).
- [ ] CMake / link flags: `-fexceptions`, `-sDISABLE_EXCEPTION_CATCHING=0`,
      `-sGROWABLE_ARRAYBUFFERS=0`, `-sFULL_ES2=1`, `-sFORCE_FILESYSTEM=1`,
      OpenAL, preload data.
- [ ] Prefer stb_image (or existing image path) over system jpeg/png.
- [ ] Produce `packages.x86_64-linux.supertux-wasm` (or equivalent) and a
      simple serve target.

## Android

- [ ] Adapt `mk/android/` (SDLActivity, jni, build-apk.sh, NDK r27+).
- [ ] GLES2 only; GameController / touch via existing controller scm or
      Android mappings.
- [ ] Macro hygiene for do/while error helpers (NDK clang).
- [ ] stb_image staging into jni include path if needed.
- [ ] APK packaging under flake.

## R36S / ArkOS

- [ ] Hybrid toolchain: modern aarch64 GCC + ArkOS sysroot (see
      `nix/r36s.nix`, `mk/r36s/` from Pingus/Windstille).
- [ ] `-nostdlib++` + ArkOS libstdc++, static libgcc where required;
      exceptions working on device.
- [ ] Drop custom SDL_GL_* attributes that break EGL surface creation.
- [ ] Force 640×480 / non-resizable handheld profile; controller profile
      using SDL_GameControllerButton layout (DPAD 11–14).
- [ ] stb_image; skip set_icon paths that throw into broken unwind.
- [ ] Launcher script with valid SuperTux flags only.

## Windows (MinGW)

- [ ] Extend existing grumnix / win32 inputs already present in Origins flake.
- [ ] Flat `exe` + DLL layout; zip packaging.
- [ ] Full external graph under `pkgsCross.mingwW64` still WIP — prefer
      prebuilt MinGW SDL2 / OpenAL / etc. over heavy nixpkgs cross.
- [ ] `meta.platforms = [ "x86_64-windows" ]` for cross packages.

## Cross-cutting

- [ ] Controller input: GameController first; ignore raw JOY* when GC owns
      the instance; raise axis deadzone; do not stack duplicate scm profiles.
- [ ] Image codecs: document and implement stb_image preference for
      constrained targets; keep system JPEG/PNG optional for desktop.
- [ ] CMake: conditional find_dependency for JPEG/PNG in any exported config.
- [ ] Keep PORTING.md updated with every platform quirk.
- [ ] Continuous numbered git bundles (`supertux-001-…`).

## Done

- [x] Initial AGENTS.md / TODO.md / PORTING.md (library breakdown + shared themes).

- 2026-08-19: SuperTux-specific `nix/wasm.nix` + flake `packages.supertux-wasm`
  (marked broken pending static wasm deps). build-app.sh adapted for
  SuperTux naming. Pingus wasm.nix kept as wasm-pingus-reference.nix.

- 2026-08-19 (batch): mk/android + mk/r36s scaffolding from Pingus; SuperTux
  renames in nix/android.nix and nix/r36s.nix; PORTS.md; flake exposes r36s
  helpers (sysroot URL still placeholder); platform READMEs.

- 2026-08-19 (batch 2): SUPERTUX_R36S cmake + 640x480 defaults; Android app
  skeleton; EMSCRIPTEN LibSDL2_ttf INTERFACE; SuperTux.sh launcher; r36s
  cmakeFlags include SUPERTUX_R36S.

- 2026-08-19 (batch 3): ProvidePhysfs PHYSFS_SOURCE_DIR + skip system on
  EMSCRIPTEN/Android; ProvideOpenGL skip glesv2 pkg on Android; physfs-src
  flake input; document controller defaults (already GC-first, deadzone 8000).

- 2026-08-19 (batch 4): GLES default pixel format; physfs-src → wasm cmake;
  SQUIRREL_SOURCE_DIR; Windows meta.platforms; Android forces GLES2.

- 2026-08-19 (batch 5): Android source list (376 cpp) + generator script;
  EMSCRIPTEN forces in-tree physfs/squirrel/fmt; FMT_SOURCE_DIR; fmt-src
  flake input.

- 2026-08-19: Removed libfmt; use std::format + format_rt + print polyfill.

- 2026-08-19: strtok cstring fix; flake outputs supertux-android + android-sdl-libs
  (broken until jni links external deps).
