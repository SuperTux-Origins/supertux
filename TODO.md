# TODO — SuperTux multi-platform port

Tip continues from 09bc574; next bundle after Android include / R36S link / WASM offline SDL2 fixes. Library breakdown lives in
PORTING.md / AGENTS.md. Packaging surface (mk/, nix/, flake outputs)
adapted from Pingus and Windstille.

## Immediate / hygiene

- [x] Library dependency matrix written (PORTING.md / AGENTS.md).
- [x] Create AGENTS.md and PORTING.md documenting process and
      Windstille/Pingus lessons.
- [x] Formatters for Direction, Vector, Size, UID, Control, Color,
      MenuId, BonusType, sexp::Value (std::format / logmich).
- [x] cstring/cstdio includes for GCC 15; strtok fix.
- [x] priocpp external, JSON tests gated; PRIO_USE_JSONCPP default OFF.
- [x] Vendor argpp, geomcpp, tinygettext into external/.
- [x] Wire R36S / Android / wasm flake package stubs.
- [ ] Audit flake.nix against current Pingus / Windstille patterns
      (stb_image, GLES packages, linuxPorts hygiene, full outputs).
- [ ] Inventory remaining gaps in mk/ and nix/ vs Pingus/Windstille
      (wasm.nix still shorter; SDL static builds incomplete).
- [ ] Continuous numbered git bundles (`supertux-001-…`).

## Desktop Linux

- [ ] Ensure OpenGL 3.3 + GLEW path builds and runs under the flake.
- [ ] Flake `packages` / `checks` only contain derivations (no attrs of attrs).
- [ ] Optional: desktop GLES2 validation build (like Windstille `windstille-gles2`).

## WebAssembly (Emscripten)

- [x] Skeleton `nix/wasm.nix` under emscriptenStdenv; skip checkPhase.
- [x] libmodplug-wasm + cmake flags so in-tree wstsound configures under EMSCRIPTEN.
- [ ] Expand wasm deps further: SDL2 / SDL2_image / zlib static builds
      (see Pingus `mk/wasm/scripts/build-*.sh`).
- [ ] CMake / link flags: `-fexceptions`, `-sDISABLE_EXCEPTION_CATCHING=0`,
      `-sGROWABLE_ARRAYBUFFERS=0`, `-sFULL_ES2=1`, `-sFORCE_FILESYSTEM=1`,
      OpenAL, preload data.
- [ ] Prefer stb_image (or existing image path) over system jpeg/png.
- [ ] Produce complete `packages.x86_64-linux.supertux-wasm` and serve app.
- [x] Drop native wstsound/squirrel from wasm buildInputs (in-tree + modplug).
- [x] Build squirrel under emscripten: fix multiarch IMPORTED path (ProvideSquirrel).

## Android

- [x] logmich → logcat tag SuperTux; boot/PhysFS diagnostics.

- [x] PHYSFS_init via PHYSFS_AndroidInit (JNIEnv + Activity); assets/data.zip mount.


- [x] Adapt `mk/android/` and `nix/android.nix` from Pingus (SDLActivity,
      jni, keystore, NDK scaffold).
- [x] flake packages.supertux-android + android-sdl-libs.
- [x] Conditional SDL2_mixer prebuilt Android.mk (null mixer for SuperTux).
- [x] uses-sdk minSdkVersion=22; SUPERTUX_VERSION env in build-apk.sh.
- [ ] GLES2 only; GameController / touch via controller scm or Android maps.
- [ ] Macro hygiene for do/while error helpers (NDK clang).
- [ ] stb_image staging into jni include path if needed.
- [ ] jni links physfs / squirrel / wstsound / full external stack.
- [ ] APK packaging verified (user feedback).

## R36S / ArkOS

- [x] Hybrid toolchain skeleton in `nix/r36s.nix` and `mk/r36s/`.
- [x] flake packages.supertux-r36s + portMaster + arkos-sysroot.
- [ ] `-nostdlib++` + ArkOS libstdc++, static libgcc; exceptions on device.
- [ ] Drop custom SDL_GL_* attributes that break EGL surface creation.
- [ ] Force 640×480 / non-resizable handheld profile; controller profile
      using SDL_GameControllerButton layout (DPAD 11–14).
- [ ] stb_image; skip set_icon paths that throw into broken unwind.
- [ ] Launcher script with valid SuperTux flags only.
- [ ] Optional sysrootSrc override documented and tested.

## Windows (MinGW)

- [x] grumnix / win32 inputs present in flake (SDL2, image, ttf, freetype,
      physfs, curl, glew).
- [x] Flat `exe` + DLL layout and zip packaging (supertux-origins-win32*).
- [ ] Full external graph under `pkgsCross.mingwW64` still WIP.
- [ ] `meta.platforms = [ "x86_64-windows" ]` for cross packages.
- [ ] Verify runtime (user feedback).

## Cross-cutting

- [ ] Controller input: GameController first; ignore raw JOY* when GC owns
      the instance; DPAD as buttons 11–14; deadzone ~8000.
- [ ] Image codecs: stb_image preference for Android / R36S / wasm.
- [ ] CMake: conditional find_dependency for JPEG/PNG in exported config.
- [ ] Keep PORTING.md updated with every platform quirk.
- [ ] Document all flake outputs in PORTS.md.

## Done in prior batches (reference)

- [x] Initial AGENTS.md / TODO.md / PORTING.md.
- [x] std::formatter suite for game types.
- [x] GCC 15 include hygiene.
- [x] external/ priocpp, logmich, sexpcpp, strutcpp, argpp, geomcpp,
      tinygettext, squirrel, wstsound, miniswig, xdgcpp, tinycmmc.
- [x] emscriptenStdenv CMake force; Android NDK C++ flags; R36S stubs.

## Audio
- [x] Enable Vorbis (libogg/libvorbis) for Android + WASM so stock `.ogg` music plays
- [ ] Confirm OpenAL link on WASM (`-lopenal`) at runtime
