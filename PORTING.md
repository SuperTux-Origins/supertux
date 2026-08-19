# Porting quirks and workarounds — SuperTux

This document records **platform-specific problems** encountered while
porting SuperTux (SuperTux-Origins) to desktop Linux, GLES2, WebAssembly,
Android, Windows (MinGW), and R36S/ArkOS — and **what we changed** to make
them work.

High-level packaging lives in the flake and [PORTS.md](PORTS.md) (to be
added). Open tasks are in [TODO.md](TODO.md). Recipes are adapted from
[Pingus](https://github.com/Pingus/pingus) and
[Windstille](https://github.com/WindstilleTeam/windstille) (`mk/`, `nix/`).

See also [AGENTS.md](AGENTS.md) for project standards and the library
overlap summary.

---

## Library breakdown and overlap

### SuperTux (Origins / upstream)

| Category        | Libraries / components |
|-----------------|------------------------|
| Window / input  | SDL2, SDL2_image, SDL2_ttf |
| Graphics        | OpenGL 3.3 + GLEW (desktop); GLES2 (Android / planned handheld) |
| Audio           | OpenAL, libogg, libvorbis |
| Filesystem      | PhysFS |
| Network         | libcurl (add-ons) |
| Text / fonts    | FreeType, (libraqm / harfbuzz / fribidi for complex scripts) |
| Math            | GLM |
| Compression     | zlib, libpng |
| Scripting       | squirrel (+ sqstdlib) |
| Bundled         | tinygettext, sexp-cpp / sexpcpp, SDL_SavePNG, partio_zip, obstack |
| Build helpers   | tinycmmc, logmich, strutcpp, priocpp, miniswig, wstsound, xdgcpp (Origins flake) |
| Windows cross   | grumnix prebuilts: SDL2, SDL2_image, freetype, SDL2_ttf, physfs, curl, glew |

### Pingus

- SDL2, OpenAL, FreeType, GLM, ogg/vorbis, GLEW or GLES2
- tinycmmc family, sexpcpp, logmich, …
- Explicit `mk/{android,wasm,r36s}` and `nix/{android,wasm,r36s}.nix`
- stb_image path for constrained targets
- Same controller / EGL / hybrid-toolchain lessons as Windstille

### Windstille

- Full monorepo of tinycmmc-based libs: surfcpp (stb_image by default),
  wstdisplay (desktop GL or GLES2), wstinput, wstsound (OpenAL + opus/
  vorbis/modplug/mpg123), wstgui, argpp, geomcpp, babyxml, …
- SDL2, GLEW/libGL or libglvnd+GLES, FreeType, GLM, squirrel
- Identical packaging surface (flake, mk/, nix/) and documented PORTING.md
  that this file inherits from.

### Overlap and porting strategy

- **High overlap**: SDL2, OpenAL, OpenGL/GLES, FreeType, GLM, zlib, squirrel,
  sexpcpp/tinycmmc ecosystem.
- **SuperTux extras**: PhysFS, curl, SDL2_ttf/image, richer text stack.
- **Portable path preferred by Pingus/Windstille**: header-only stb_image
  instead of system libjpeg/libpng on Android / R36S / wasm; explicit GLES2
  CMake switches; GameController-first input; hybrid R36S toolchain;
  Emscripten exception + ArrayBuffer flags.

Do **not** invent a second packaging stack. Copy `mk/` and `nix/` from
Pingus/Windstille, then adjust CMake flags, data install paths, and
controller profiles for SuperTux.

---

## Shared themes (inherited from Windstille PORTING.md)

### Image codecs: prefer `stb_image` on constrained targets

**Problem.** Cross sysroots (ArkOS, Android NDK, Emscripten) often lack
usable libjpeg/libpng, or shipping static wasm copies is slow and fragile.

**Solution.** For Android / R36S / wasm builds of SuperTux, enable a
stb_image path (or ensure SDL2_image is built with stb / minimal deps).
Keep system JPEG/PNG optional for desktop. Any exported CMake config must
not unconditionally `find_dependency(JPEG|PNG)`.

### GLES vs desktop OpenGL

**Problem.** Ports need OpenGL ES 2.0 / WebGL; SuperTux historically uses
desktop OpenGL + GLEW.

**Solution.** Introduce / honour `ENABLE_OPENGLES2` (already present in
upstream SuperTux CMake) and ensure the GLES2 code path does not pull
desktop libGL/GLEW. Target OpenGL 3.3 on desktop, GLES2 on Android/R36S,
WebGL on wasm.

### Controller input: GameController first

**Problem.** Menus often listen only to DPAD buttons, not stick axes.
R36S profiles may map D-pad to hat axes; SDL_GameController exposes DPAD
as buttons 11–14. Duplicate JOY*/HAT events cause double steps. Deadzone 0
fires on noise. Loading multiple controller scm files stacks bindings.

**Solution.**

- Profiles use **SDL_GameControllerButton** layout (A=0 … DPAD=11–14).
- Prefer GameController; ignore raw `JOYAXIS` / `JOYBUTTON` / `JOYHAT` when
  the instance is owned by a GameController.
- Pure-joystick hats can synthesize DPAD button indices 11–14.
- Stick drives movement only, not menu navigation.
- Raise axis deadzone (e.g. 8000).
- SDL init must include `JOYSTICK` + `GAMECONTROLLER`.
- Do not load a secondary gamepad scm when a primary controller file is set.

---

### libmodplug required under EMSCRIPTEN

**Problem.** `external/wstsound/CMakeLists.txt` forces
`WSTSOUND_WITH_MODPLUG=ON` when `EMSCRIPTEN OR ANDROID`. Configure then
runs `find_package(ModPlug REQUIRED)` and fails if libmodplug is absent.

**Solution.** Build static `libmodplug-wasm` (same recipe as Pingus
`nix/wasm.nix`) and pass:

- `-DMODPLUG_DIR=…` / `-DMODPLUG_INCLUDE_DIRECTORY=…` / `-DMODPLUG_LIBRARY=…`
- `-DWSTSOUND_WITH_VORBIS=OFF` `-DWSTSOUND_WITH_OPUS=OFF`
  `-DWSTSOUND_WITH_MPG123=OFF` `-DWSTSOUND_WITH_EFX=OFF`
- `PKG_CONFIG_PATH` / `CMAKE_PREFIX_PATH` pointing at the modplug prefix

Do **not** put native (x86_64) flake-input `wstsound` / `squirrel` packages
into the emscripten derivation `buildInputs` — wrong architecture and
confuses `find_package`. Prefer in-tree `external/` under `emcmake`.

### tinycmmc under emscripten

`find_package(tinycmmc)` may miss the flake package; SuperTux falls back to
`external/tinycmmc/modules/` (message: "tinycmmc module path: …"). That is
acceptable for wasm.


## R36S / ArkOS

### R36S: glm and prio/logmich under FIND_ROOT ONLY

**Problem.** `CMAKE_FIND_ROOT_PATH_MODE_{INCLUDE,PACKAGE}=ONLY` hides host
nixpkgs glm; `ProvideGlm.cmake` then fatal-errors. `find_package(prio)` /
`logmich` also miss and there was no `external/` fallback (unlike wstsound).

**Solution.**

- Pass `-DSUPERTUX_GLM_INCLUDE_DIR=${glm}/include` (and `GLM_ROOT_DIR`);
  `ProvideGlm.cmake` honours that path first (Pingus-style).
- `build_dependencies()` now adds `external/sexpcpp`, `external/logmich`,
  `external/priocpp` when the imported targets are missing (order: sexp →
  logmich → prio so priocpp sees existing targets).
- `-DPRIO_USE_JSONCPP=OFF` for R36S (no jsoncpp in sysroot).

 (to implement)

### Hybrid toolchain: modern GCC + old sysroot

Same pattern as Windstille/Pingus:

| Piece            | Source                                      |
|------------------|---------------------------------------------|
| Compiler         | nixpkgs cross GCC (modern)                  |
| C++ headers      | same GCC (C++20 / format if used)           |
| libc headers     | ArkOS sysroot only                          |
| libstdc++.so     | ArkOS (absolute; `-nostdlib++`)             |
| libgcc           | static from toolchain (`-static-libgcc`) or matching shared |
| Dynamic linker   | `/lib/ld-linux-aarch64.so.1`                |

Wrappers enforce include order and `--sysroot`. Do not put GCC
`include-fixed` ahead of the sysroot (pthread / jmp_buf_tag mismatch).

### C++ exceptions

Match libgcc / libstdc++ ages so throw/catch works on device. Prefer
`RelWithDebInfo` for readable on-device gdb.

### EGL window creation

Drop hand-tuned `SDL_GL_*` color/buffer/stencil/MSAA attributes; defaults
work better on the handheld GLES stack.

### Other

- Skip fragile set_icon / texture paths that throw into broken unwind.
- stb_image (or minimal image loader).
- Launcher with valid SuperTux options only; force 640×480 / non-resizable
  for the handheld profile.

---

## WebAssembly (Emscripten) (to implement)

- FreeType may need `-lz` at final link even if ZLIB_ROOT was set at configure.
- stb_image / minimal image path; avoid heavy static jpeg/png.
- Exceptions: `-fexceptions -sDISABLE_EXCEPTION_CATCHING=0 -sEXCEPTION_STACK_TRACES=1`.
- `-sGROWABLE_ARRAYBUFFERS=0` (TextDecoder vs growable buffers).
- `-sFULL_ES2=1`, WebGL range, `-sFORCE_FILESYSTEM=1`, optional preload of data/.
- OpenAL when sound enabled.

---

## Android (to implement)

- NDK r27+ for solid `std::format` / modern STL if used.
- do/while error macros must be semicolon-safe under NDK clang.
- GLES2; stb_image or SDL2_image with limited deps.
- Lifecycle on SDLActivity singleTask; controller mappings for touch/pad.

---

## Desktop Linux / flake hygiene

- `packages` and `checks` must contain only derivations (no helper functions
  or string attributes).
- Cross packages (Windows) need correct `meta.platforms`.
- Conditional JPEG/PNG find_dependency after any stb switch.

---

## Windows (MinGW)

Origins already pulls many grumnix win32 packages. Prefer those over
pulling ffmpeg-heavy openal from nixpkgs cross. Packaging: flat exe + DLLs
+ data zip. Full external graph under pkgsCross.mingwW64 remains WIP.

---

## Quick reference: where fixes will live

| Area                    | Primary locations (to adapt)              |
|-------------------------|-------------------------------------------|
| stb_image / codec policy| CMakeLists / external image helpers, flake |
| R36S toolchain + sysroot| `nix/r36s.nix`, `mk/r36s/`                |
| R36S runtime guards     | main / video code, `RelWithDebInfo`       |
| GL attributes           | video / OpenGL window setup               |
| GameController / menu   | controller profiles, input code           |
| wasm link / flags       | `mk/wasm/scripts/build-app.sh`, `nix/wasm.nix` |
| Android NDK / APK       | `nix/android.nix`, `mk/android/`          |
| Flake check surface     | `flake.nix` packages / checks             |

---

## Principles that keep working

1. **Match the device ABI** (sysroot, linker, libstdc++) even if the compiler
   is newer — then narrow the compiler if exceptions still disagree.
2. **One input path** for pads (GameController); do not stack joystick + GC
   events or duplicate scm profiles.
3. **Header-only codecs** for constrained targets; keep optional system
   JPEG/PNG for desktop.
4. **Flake outputs that CI evaluates must be derivations**; keep functions
   under `apps` or private `let` bindings.
5. Prefer **clean defaults** (EGL attributes, resolution) over clever
   platform-specific GL/window setup unless a device proves it necessary.

When in doubt, compare with the corresponding Pingus or Windstille file
under `mk/` or `nix/` and adapt rather than inventing a second stack.

### WASM packaging (Origins)

- CMake already detects `EMSCRIPTEN` and injects USE_FLAGS (updated in the
  first porting commit). Prefer those over duplicating every `-s` flag in
  the outer nix derivation.
- `mk/emscripten/template.html.in` is the SuperTux shell; Pingus
  `mk/wasm/shell.html` remains as a secondary reference.
- `nix/wasm.nix` builds under `emscriptenStdenv` with `ENABLE_OPENGLES2=ON`.
  Package is marked `broken = true` until physfs / squirrel / wstsound /
  tinycmmc-family have static wasm builds (or CMake soft-disables them).
- `nix/wasm-pingus-reference.nix` keeps the full Pingus static-SDL approach
  for when we need offline ports instead of `-sUSE_SDL=2`.
- `mk/wasm/scripts/build-app.sh` defaults renamed to `supertux-origins`.

### R36S handheld defaults (Origins)

- CMake option `-DSUPERTUX_R36S=ON` defines `SUPERTUX_R36S`, forces
  `ENABLE_OPENGLES2`, and is passed from `nix/r36s.nix`.
- `gameconfig.cpp`: default window 640×480, non-resizable, fullscreen.
- `main.cpp`: skips window icon under `SUPERTUX_R36S` (already).
- Launcher: `mk/r36s/scripts/SuperTux.sh` (`--fullscreen --geometry 640x480`).

### EMSCRIPTEN SDL2_ttf

- `ProvideSDL2_ttf.cmake` creates an INTERFACE `LibSDL2_ttf` under EMSCRIPTEN
  so configure succeeds when FreeType comes from `-sUSE_FREETYPE=2`.

### Android app skeleton

- `mk/android/app/` has AndroidManifest (SDLActivity singleTask, GLES2),
  jni/Android.mk + Application.mk + placeholder main. Full source list and
  static deps still TODO.

### Controller input (Origins status)

SuperTux already matches the Windstille/Pingus GameController-first policy:

- `JoystickConfig::m_use_game_controller` defaults to **true**
- When GC mode is on, raw `JOYAXIS` / `JOYBUTTON` / `JOYHAT` are ignored
- Axis deadzone is **8000** in both `GameControllerManager` and `JoystickConfig`
- SDL init includes `SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER`

No separate `*.scm` controller profiles (unlike Windstille); bindings live in
the user config / in-game joystick menu.

### PhysFS on constrained targets

- Prefer system PhysFS with `PHYSFS_getPrefDir` on desktop.
- Under EMSCRIPTEN / Android, system PhysFS is skipped; build from
  `external/physfs` or `-DPHYSFS_SOURCE_DIR=…`.
- Flake input `physfs-src` (icculus physfs 3.2.0) is available for packaging
  to pass as `PHYSFS_SOURCE_DIR` on cross builds.

### OpenGL ES on Android

- `ProvideOpenGL.cmake` does not `pkg_check_modules(glesv2)` on Android /
  Emscripten (SDL/NDK provide GLES). Desktop GLES2 and R36S still use pkg-config.

### GLES window attributes (Origins)

`GLVideoSystem::create_gl_window()` under `USE_OPENGLES2` only sets ES 2.0
profile + double-buffer. Color/depth sizes are left to the driver — avoids
EGL surface creation failures on ArkOS (Windstille PORTING.md lesson).

### Squirrel cross builds

`-DSQUIRREL_SOURCE_DIR=` mirrors PhysFS: build from a source tree when
`external/squirrel` is not checked out. Toolchain file is forwarded to
ExternalProject.

### EMSCRIPTEN system-package overrides

Under `if(EMSCRIPTEN)`, CMake forces:

- `USE_SYSTEM_PHYSFS=OFF`
- `USE_SYSTEM_SQUIRREL=OFF`
- `USE_SYSTEM_FMT=OFF`

so packaging must supply `PHYSFS_SOURCE_DIR` / `SQUIRREL_SOURCE_DIR` /
`FMT_SOURCE_DIR` (flake inputs `physfs-src`, `fmt-src`).

### libfmt → std::format

fmt was removed in favour of C++20 `std::format` / `std::vformat`:

- `src/util/format.hpp` — `supertux::format` (compile-time) and
  `supertux::format_rt` (runtime / gettext strings)
- `src/util/print.hpp` — C++23 `std::print` or format+fwrite polyfill (Pingus)
- R36S: `mk/r36s/cxxabi_shim.cpp` provides floating `std::to_chars` used by
  libstdc++ `std::format` on older ArkOS libstdc++ (from Pingus)

Require C++20 (`<format>`). NDK r27+ and modern desktop toolchains OK.

### Vendored flake dependencies (`external/`)

C++ libraries used by the game are vendored under `external/` via
`git subtree --squash` so they can be patched in-tree:

| Directory | Upstream |
|-----------|----------|
| tinycmmc | github:grumbel/tinycmmc |
| logmich | github:logmich/logmich |
| sexpcpp | github:lispparser/sexp-cpp |
| strutcpp | github:grumbel/strutcpp |
| priocpp | github:grumbel/priocpp |
| xdgcpp | github:grumbel/xdgcpp |
| miniswig | github:WindstilleTeam/miniswig |
| wstsound | github:WindstilleTeam/wstsound |
| squirrel | github:grumnix/squirrel |

Win32 prebuilt packages remain flake inputs (grumnix/*-win32).

**priocpp + jsoncpp:** SuperTux does not need JSON. Build with
`-DPRIO_USE_JSONCPP=OFF`. ReaderDocument tests only instantiate `.json`
params when `PRIO_USE_JSONCPP` is enabled.

### std::format formatters for game types

After dropping libfmt, types passed to logmich (`std::vformat`) need
`std::formatter` specializations. Added for:

- `Direction` (`src/supertux/direction.hpp`)
- `worldmap::Direction` (`src/worldmap/direction.hpp`)
- `Vector` / `glm::vec2` (`src/math/vector.hpp`)
- `Size` (`src/math/size.hpp`)

If the build fails with `basic_format_arg` / no conversion from `SomeType`,
add a formatter (or pass `.x`/`.y`/string conversion at the call site).

Additional formatters (scripting / math / input):

- `UID` (`src/util/uid.hpp`) — used by `SCRIPT_GUARD` dead-object logs
- `Sizef`, `Rect`, `Rectf`
- `Control` (`src/control/controller.hpp`)

### stream_str helper

`src/util/stream_format.hpp` provides `supertux::stream_str(value)` for types
that only implement `operator<<` and are not yet given a `std::formatter`.
Use at call sites as a last resort: `log_info("{}", stream_str(x))`.

### strtok / glibc headers (GCC 15)

`squirrel_virtual_machine.cpp` must `#include <cstring>` for `strtok`.
Newer libstdc++ / C++ modes no longer pull it in transitively.

### Android flake outputs

```
nix build .#supertux-android   # APK (meta.broken until full jni link)
nix build .#android-sdl-libs   # SDL2 .so prebuilts only
```

Requires `nixpkgs.config.allowUnfree` and `android_sdk.accept_license = true`.
Debug keystore: `mk/android/keystore/debug.keystore` (store/key pass: android).

### Flake package aliases for vendored deps

| Attribute | Source |
|-----------|--------|
| logmich-pkg | external/logmich |
| sexpcpp-pkg | external/sexpcpp |
| strutcpp-pkg | external/strutcpp |
| priocpp-pkg | external/priocpp (JSON off) |

Android NDK: `mk/android/scripts/build-external-static.sh` builds one
`external/` CMake project for a given ABI.

### Unblocking wasm / android evaluation (2026-08-19)

`meta.broken` was removed from `supertux-wasm` and `supertux-android` so
`nix build .#supertux-wasm` and `nix build .#supertux-android` report real
builder errors instead of evaluation refusal.

WASM deps: `logmich`, `sexpcpp`, `strutcpp` are built under `emscriptenStdenv`
from `external/`. Squirrel and wstsound still use native flake packages until
static wasm builds exist — expect link or wrong-arch errors; that is the next
porting step.

### R36S package outputs

```
nix build .#arkos-sysroot              # needs real sysroot URL or sysrootSrc=
nix build .#supertux-r36s
nix build .#supertux-r36s-portmaster
nix build .#supertux-r36s-portmaster-zip
```

Until a permanent sysroot tarball is hosted, either:

1. Replace `url` / `hash` in `nix/r36s.nix`, or
2. Pass `sysrootSrc = /path/to/arkos-sysroot4.tar.gz;` into the
   `import ./nix/r36s.nix { ... }` call in `flake.nix`, or
3. Build a Buster sysroot with
   `sudo mk/r36s/scripts/make-sysroot-debootstrap.sh /opt/arkos-sysroot arm64`
   and pack it (`tar czf arkos-sysroot4.tar.gz -C /opt/arkos-sysroot .`).

`SUPERTUX_R36S=ON` forces GLES2, 640×480 defaults, and skips fragile icon load.

### Vendored external/argpp (stable)

Android packaging stages `external/argpp/include`. The tree is the **stable**
branch of https://github.com/Grumbel/argpp (not master).

Also vendored for the same Android staging loop: `geomcpp`, `tinygettext`.

WASM package attribute: `packages.supertux-wasm` (not `supertux-origins-wasm`).

### EmscriptenStdenv + CMake

`pkgs.emscriptenStdenv` defaults to autotools (`emconfigure ./configure`).
SuperTux and its CMake deps must set `dontConfigure = true` and run
`emcmake cmake …` in `preBuild` (see `nix/wasm.nix` `mkWasmCmake`).


### Android: no SDL2_mixer for SuperTux

**Problem.** `nix/android.nix` generated a prebuilt `jni/SDL/Android.mk` that
always listed `libSDL2_mixer.so`. SuperTux sets `sdlMixerSrc = null` (sound is
OpenAL + wstsound/modplug), so ndk-build aborted on the missing `.so`.

**Solution.** Only emit the `SDL2_mixer` prebuilt stanza when
`sdlMixerSrc != null`. Soften the build-apk.sh warning about missing
`SDL_mixer.h` (expected without mixer).

### Android: minSdkVersion vs APP_PLATFORM

**Problem.** NDK warned that `APP_PLATFORM android-22` was higher than
`android:minSdkVersion 1` in the manifest.

**Solution.** Add `<uses-sdk android:minSdkVersion="22" android:targetSdkVersion="22" />`
to `mk/android/app/AndroidManifest.xml` (match Pingus / packagePlatform).

### Android: version env name

**Problem.** `build-apk.sh` still echoed/exported `PINGUS_VERSION` while
`mkApk` sets `SUPERTUX_VERSION`.

**Solution.** Prefer `SUPERTUX_VERSION`, keep `PINGUS_VERSION` as an alias for
shared script compatibility.

