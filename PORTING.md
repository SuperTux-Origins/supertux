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


### R36S: priocpp PROJECT_VERSION empty under add_subdirectory

**Problem.** SuperTux passes `-DPROJECT_VERSION_FULL=<game version>`.  When
`external/priocpp` is added as a subdirectory, `project(prio)` without a
VERSION argument leaves `PROJECT_VERSION` defined but empty, so
`write_basic_package_version_file()` errors with "No VERSION specified".

**Solution.** In `external/priocpp/CMakeLists.txt`, always read the local
`VERSION` file for package metadata when embedded
(`CMAKE_SOURCE_DIR != CMAKE_CURRENT_SOURCE_DIR`), and guard the
write_basic call against an empty version string.


### external/squirrel is packaging-only

**Problem.** `external/squirrel` contains only a nested flake pointing at
`github:albertodemichelis/squirrel`; there is no `CMakeLists.txt` in-tree.
`ProvideSquirrel.cmake` expects sources at `SQUIRREL_SOURCE_DIR`.

**Solution.** Flake input `squirrel-src` fetches the upstream tree; wasm (and
later R36S) pass `-DSQUIRREL_SOURCE_DIR=${squirrel-src}` /
`-DUSE_SYSTEM_SQUIRREL=OFF`.

### Android: do not redeclare SDL2 under jni/src

**Problem.** App `jni/Android.mk` listed `LOCAL_SRC_FILES := SDL2/lib/$(ABI)/…`
relative to `jni/src/`, while prebuilts are provided by sibling `jni/SDL/`
(absolute store path from `sdlPrebuiltAndroidMk`). ndk-build looked for
`jni/src/SDL2/lib/...` and aborted.

**Solution.** Match Pingus: only `LOCAL_SHARED_LIBRARIES := SDL2` in the
game module; SDL2 prebuilt lives solely in `jni/SDL/Android.mk`.


### R36S: PhysFS / squirrel sources same as wasm

ArkOS system PhysFS may lack `PHYSFS_getPrefDir`, so ProvidePhysfs falls
back to sources. `external/physfs` is empty in-tree; pass
`-DPHYSFS_SOURCE_DIR` from flake input `physfs-src`. Same for squirrel via
`squirrel-src`.

### Android: re-copy Android.mk after GAME_SRC_DIR

`cp -r "$GAME_SRC_DIR"/. src/jni/src/` runs after the module Android.mk is
installed. Re-copy `APP_DIR/jni/Android.mk` afterward so a stray file cannot
restore an old PREBUILT SDL2 stanza.


### Android launcher icon

`aapt` failed with missing `@mipmap/ic_launcher`. Added mdpi–xxxhdpi
`ic_launcher.png` under `mk/android/app/res/` (temporary assets from the
Pingus Android scaffold; replace with SuperTux branding later).

### wasm: ProvideSavePNG needs libpng/zlib ports

`find_package(PNG REQUIRED)` fails under emcmake. Use `-sUSE_LIBPNG=1
-sUSE_ZLIB=1` in USE_FLAGS and build LibSavePNG without system PNG.

### R36S: curl not in ArkOS sysroot

`ProvideCurl.cmake` skipped for `SUPERTUX_R36S` (same as EMSCRIPTEN);
`HAVE_LIBCURL` stays false and the LibCurl link is gated on that flag.


### R36S: SDL2_ttf required in sysroot

ProvideSDL2_ttf now searches via pkg-config and explicit
`${CMAKE_SYSROOT}/usr/...` paths. If ArkOS sysroot was built without
`libsdl2-ttf-dev`, configure still fails — extend the sysroot tarball.

### wasm: version.cmake + emscripten asset names

`lib.cleanSource` drops `.git`, so BuildVersion cannot git-describe.
It now synthesizes `version.cmake` from `-DPROJECT_VERSION_FULL`.
BuildInstall copies `mk/emscripten/supertux.png` and `supertux_bkg.png`
(not the old `supertux-origins_*.png` names).

### Android: placeholder main exits immediately

The APK currently links `placeholder.cpp` only (`SDL_main` returns 0), so
the activity starts and stops at once. Full game sources must be wired via
`supertux_sources.mk` / deps before a playable build.


### wasm: USE_FREETYPE must be boolean on Emscripten 4+/6+

`-sUSE_FREETYPE=2` fails with "expects bool but got int". Use
`-sUSE_FREETYPE=1`. Do not forward parent `CMAKE_C_FLAGS` containing
emscripten port flags into ExternalProject (physfs/squirrel) — they
inherit broken flags and old cmake_minimum_required needs
`-DCMAKE_POLICY_VERSION_MINIMUM=3.5`.

### R36S: build SDL2_ttf from source when sysroot lacks it

Pass `-DSDL2_TTF_SOURCE_DIR` from flake input `sdl2-ttf-src`. ProvideSDL2_ttf
builds a shared lib into the build prefix when system/pkg-config search fails.


### R36S: libpng for SavePNG

ArkOS sysroot may lack CMake PNG config; ProvideSavePNG searches
`${CMAKE_SYSROOT}/usr/...` for `png.h` / `libpng.so` explicitly.

### wasm: squirrel static-only under ExternalProject

Upstream squirrel builds shared libs + `sq` by default; emscripten then
fails linking `sq.js` against `libsquirrel.so`. Pass
`-DDISABLE_DYNAMIC=ON -DDISABLE_EXECUTABLES=ON -DBUILD_SHARED_LIBS=OFF`.

### Android placeholder logging

`placeholder.cpp` now SDL_Inits, creates a window, logs display mode via
`__android_log_print`, and stays up ~3s so logcat is readable.


### wasm: no network for emscripten ports in nix sandbox

`-sSDL2_IMAGE_FORMATS='["jpg"]'` forces a libjpeg port download at compile
time → `Temporary failure in name resolution` under the sandbox. Use **png
only**. Split `USE_COMPILE_FLAGS` vs `USE_LINK_FLAGS` so linker-only
settings are not passed to every compile step.

### R36S: SavePNG stub without libpng

If libpng is absent from the sysroot, ProvideSavePNG installs a no-op stub
so configure can finish (screenshots return -1).

### Android: SDL_Log for placeholder

logcat filter `SDL` should show `supertux:` lines; window stays open 15s.


### wasm: ports only on final link (nix sandbox)

Any `-sUSE_LIBPNG` / `-sUSE_ZLIB` / `-sUSE_FREETYPE` on **global**
`CMAKE_C_FLAGS` makes every object compile try to download ports → DNS
failure. Keep those flags on `CMAKE_EXE_LINKER_FLAGS` only. Intermediate
targets (prio, wstsound, SavePNG stub) compile without ports.

### Android: placeholder only (not the game)

`mk/android/app/jni/Android.mk` uses `placeholder.cpp` when present.
ndk-build finishes in seconds because the full `src/` tree is **not** in
`LOCAL_SRC_FILES`. Wiring `supertux_sources.mk` + deps is the next large
Android task.


### wstsound: MPG123 / EFX / LOOPBACK (WASM + R36S)

Synced SuperTux `external/wstsound` sources with Windstille so that:

- `sound_file.cpp` only includes `mp3_sound_file.hpp` under `WSTSOUND_WITH_MPG123`
- `openal_sound_source.cpp` only includes `<AL/efx.h>` under `WSTSOUND_WITH_EFX`
- `openal_system.cpp` only includes `<alext.h>` under `WSTSOUND_WITH_LOOPBACK`

CMake forces these OFF for `EMSCRIPTEN | ANDROID | SUPERTUX_R36S`. Do not
reinvent — keep in lockstep with Windstille’s wstsound.


### wstsound: why MPG123 still compiled after "OFF"

1. SuperTux sources lacked `#if WSTSOUND_WITH_*` around includes (fixed from Windstille).
2. CMake `option()` can reset -D values under CMP0077 OLD — set normal vars
   *before* `option()` and FORCE CACHE before `add_subdirectory(wstsound)`.
3. Prefer `#if WSTSOUND_WITH_FOO` over `#if defined(WSTSOUND_WITH_FOO)` so that
   a leaked `-DWSTSOUND_WITH_FOO=0` still disables the include.

Look for `wstsound codecs: ... MPG123=OFF EFX=OFF` in the configure log.

### Android: build-apk.sh aborted at source listing

`set -euo pipefail` + `find … | head -20` → find gets SIGPIPE and exits
non-zero → whole script aborts before `ndk-build`. Never pipe find into
head under pipefail; use `wc -l` only (or `head … || true`).

### R36S: STDERR_FILENO in error_handler.cpp

`backtrace_symbols_fd(..., STDERR_FILENO)` needs `<unistd.h>`. Desktop
glibc often pulls it transitively; ArkOS sysroot does not. Guard stays
under `__GLIBC__`.

### WASM: effect.hpp still pulled in

`sound_manager.cpp` must only `#include "effect.hpp"` under
`WSTSOUND_WITH_EFX`. The effect.cpp sources are filtered out by CMake,
but the header include still requires `AL/efx.h`. Match Windstille.

### Android: cp -a from nix store stays read-only

After `cp -a $SQUIRREL_SOURCE_DIR/include` the staged tree is still 0555.
Further `cp` into that include/ fails with Permission denied. Always
`chmod -R u+rwX` immediately after each store copy before writing more.

### Android: config.h, GLM_ENABLE_EXPERIMENTAL, SDL_image

CMake writes `config.h` into the build dir; NDK has no cmake step, so
`mk/android/app/jni/config.h` is staged into `jni/src/`.

`GLM_ENABLE_EXPERIMENTAL` is required for glm/gtx/* (vector.hpp).

SDL2_image is not in the Android prebuilts yet — `img_stb_min.c` + a
minimal `SDL_image.h` provide `IMG_Load_RW` via stb_image.

### WASM: SavePNG stub must not include SDL.h

Emscripten's fake `SDL.h` errors without `-sUSE_SDL=2` on that TU.
Use opaque struct declarations in the stub.

### R36S: strut + xdgcpp

ArkOS sysroot has neither. `build_dependencies()` now falls back to
`external/strutcpp` and `external/xdgcpp` when `find_package` misses them.
`strut/numeric_less.hpp` is required by `levelset.cpp`; `xdgcpp/xdg.h` by
`main.cpp` (not guarded for R36S).

### Android: std::format needs -fexperimental-library

NDK r26 libc++ still treats parts of `<format>` as experimental. Without
`-fexperimental-library`, `std::vformat` / `std::formatter` are missing
even with `-std=c++20`.

### R36S: xdgcpp include layout

In-tree xdgcpp has `include/xdg.h` but SuperTux includes `<xdgcpp/xdg.h>`.
CMake copies the header to `$build/xdg_inc/xdgcpp/xdg.h` and adds that
include path. strut is exposed via `external/strutcpp/include`.

### R36S: SDL2_ttf installs to lib64/

On aarch64 CMake defaults `CMAKE_INSTALL_LIBDIR` to `lib64`. The imported
`LibSDL2_ttf` target expected `lib/libSDL2_ttf.so`. Force
`-DCMAKE_INSTALL_LIBDIR=lib` in ProvideSDL2_ttf.cmake.

### WASM: -sUSE_SDL=2 must be compile flags

Emscripten `fakesdl/*.h` #error unless every TU is compiled with
`-sUSE_SDL=2` (and image/freetype/ES2 ports). Put them on
`CMAKE_C_FLAGS` / `CMAKE_CXX_FLAGS`, not only the final link line.

### Android: SDL_ttf needs FreeType

Do not compile upstream `SDL_ttf.c` without FreeType. Stage header + a
minimal stub until FreeType is packaged for NDK.

### WASM: cmakeFlags with spaces

`-DCMAKE_C_FLAGS=-sUSE_SDL=2 -sUSE_…` must be a single argv. Use
`lib.escapeShellArgs cmakeFlags` when expanding into the emcmake
command line, not `concatStringsSep " "`.

### Android: argpp private includes

argpp sources `#include "argpp.hpp"` which lives under
`include/argpp/`. Stage public headers into `external_includes/argpp/`
and add that path (plus `deps/argpp` for prettyprinter.hpp) to
`LOCAL_C_INCLUDES`.

### R36S: physfs ExternalProject libdir

Same as SDL2_ttf: aarch64 CMake may install to lib64/. Force
`-DCMAKE_INSTALL_LIBDIR=lib` and `-DLIB_SUFFIX=` so
`physfs/lib/libphysfs.a` matches BUILD_BYPRODUCTS / IMPORTED_LOCATION.


### Android: sstream + formatter<T, char>

NDK libc++ needs `#include <sstream>` for `std::ostringstream`. Custom
`std::formatter` specializations must use the two-parameter form
`formatter<T, char>` or make_format_args rejects the type.

### WASM: offline static SDL2 (no emscripten ports)

`-sUSE_SDL=2` / `-sUSE_SDL_IMAGE=2` / `-sUSE_FREETYPE=1` trigger emscripten
port downloads at **compile** time when those flags are on CMAKE_C/CXX_FLAGS.
Nix sandbox has no DNS → `Temporary failure in name resolution`.

Match Pingus: build static SDL2 offline via `mk/wasm/scripts/build-sdl2.sh`
from the `sdl2-src` flake input, expose it through pkg-config / CMAKE_PREFIX_PATH,
and do **not** pass `-sUSE_SDL*` on compile or link flags. SDL_image is provided
by `mk/emscripten/SDL_image.h` + `sdl_image_stub.c` (IMG_* return NULL until
stb_image is wired).

### Android: priocpp / strut flat includes

priocpp and strutcpp sources use `#include "reader_collection.hpp"` (no
`prio/` prefix) because CMake adds `include/prio/` as a private include dir.
NDK Android.mk must list `external_includes/prio`, `external_includes/strut`,
`deps/priocpp`, `deps/strutcpp`, `deps/wstsound`, etc. under LOCAL_C_INCLUDES
or the staged sources fail with "file not found".

### R36S: sysroot libstdc++ + cxxabi_shim (not static GCC libstdc++)

Compiling with GCC 15 libstdc++ headers emits references to symbols not present
in ArkOS's older libstdc++.so (`std::to_chars` for floats, `__throw_bad_array_new_length`,
`__cxa_call_terminate`). Statically linking GCC 15's libstdc++ **also** fails:
the archive pulls modern glibc symbols (`__isoc23_strtoul`, `__libc_single_threaded`,
`arc4random`) absent from ArkOS glibc ~2.30.

Match Pingus: `-nostdlib++`, link sysroot `libstdc++.so` by absolute path, and
compile `mk/r36s/cxxabi_shim.cpp` into the executable (`-DSUPERTUX_CXXABI_SHIM=…`)
to supply the missing ABI symbols plus `_dl_find_object`. Keep `-static-libgcc`
only (for libgcc_eh without shared GLIBC_2.35 from libgcc_s).

### Android: do not define PRIO_USE_JSONCPP=0

priocpp gates JSON with `#ifdef PRIO_USE_JSONCPP` (now also
`#if defined(...) && PRIO_USE_JSONCPP`). Defining the macro to `0` still
makes `#ifdef` true, so `<json/reader.h>` is included and NDK fails.
Leave the macro **undefined** on Android (CMake only defines it when the
option is ON). JSON source files are already filtered out of the NDK build.

### Android: TINYGETTEXT_WITH_SDL (no libc iconv)

Android NDK has no `<iconv.h>`. tinygettext's iconv.hpp inline wrappers call
`::iconv_open` when neither TINYGETTEXT_WITH_SDL nor TINYGETTEXT_UTF8_ONLY is
set; those names then resolve to the wrappers themselves → compile error.
Define `-DTINYGETTEXT_WITH_SDL=1` so conversion goes through SDL_iconv.

### WASM: sstream includes

libc++ under emscripten does not transitively pull `<sstream>` via other
headers; translation units that use `std::ostringstream` need an explicit
`#include <sstream>` (e.g. src/math/anchor_point.cpp).

### R36S PortMaster screenshot path

The project icon lives at `data/images/engine/icons/supertux.png`, not
`data/images/engine/supertux.png`. Wrong path fails flake eval with
"Path … does not exist in Git repository".

### Android: std::formatter must be formatter<T, char>

NDK r26 libc++ `std::format` / `make_format_args` does not pick up
single-parameter `formatter<T>` specializations reliably; `__determine_arg_t`
falls through and treats the type as a string-like object (looks for
`.data()` / `.size()`). Use the two-parameter form:

```cpp
template<>
struct std::formatter<UID, char> { ... };
```

UID was the failure in `scripting/dispenser.cpp` via
`log_fatal("...{}", m_uid)`.

### WASM: EMSCRIPTEN vs __EMSCRIPTEN__

emcc always defines `__EMSCRIPTEN__`. Source that uses bare `EMSCRIPTEN`
(e.g. `#if !defined(EMSCRIPTEN)` for xdgcpp) needs either that macro as a
compile definition or `__EMSCRIPTEN__` in the source. CMakeLists now does
`add_compile_definitions(EMSCRIPTEN=1)` under `if(EMSCRIPTEN)`.

### WASM: EM_ASM needs emscripten.h

`Config::load` / `Main` use `EM_ASM({...})`. Without `#include <emscripten.h>`
the preprocessor leaves the block as raw C and `supertux_loadFiles` is an
undeclared identifier.

### WASM: offline SDL_ttf stub

emscripten's `fakesdl/SDL_ttf.h` `#error`s unless `-sUSE_SDL=2` (network port).
Use `mk/emscripten/SDL_ttf.h` + `sdl_ttf_stub.c` instead of the FreeType port
until a full offline SDL2_ttf+freetype build is wired.

### R36S: SIGABRT on bare `./supertux-origins`

Seen as `Error: signal 6` from ErrorHandler. Common causes:

1. **GLES context creation fails**, then `assert_gl()` / `glGetError()` runs
   without a context and the Mali driver aborts (same class of bug Pingus
   fixed by not asserting on failed `set_video_mode`). SuperTux now throws
   if `SDL_GL_CreateContext` returns null and sets `SDL_HINT_OPENGL_ES_DRIVER`.

2. **`std::filesystem::canonical(datadir)` throws** when the computed datadir
   does not exist (baked install prefix / wrong cwd). Uncaught + fragile
   exception unwind → `std::terminate` → abort. Prefer `./data` next to the
   binary; only `canonical` existing paths.

3. **PortMaster launcher must not pass `--renderer sdl`**: Origins has no
   `VIDEO_SDL` backend (only auto/opengl/null). Invalid renderer throws at
   startup. Use default auto (GLES2 when built with `USE_OPENGLES2`).

Always launch via `SuperTux.sh` (sets `--datadir`, `--userdir`, env) or:
```
./supertux-origins --datadir ./data --userdir ./conf --fullscreen
```

### WASM: use __EMSCRIPTEN__, not EMSCRIPTEN

Modern emscripten marks the bare `EMSCRIPTEN` macro deprecated. Prefer
`__EMSCRIPTEN__` in all source guards. Origins has no `VIDEO_SDL` backend —
WASM uses `VIDEO_AUTO` → GLES2/WebGL. Include `port/emscripten.hpp` for
`init_emscripten()`.

### WASM: sstream includes

libc++ under emscripten needs an explicit `#include <sstream>` for
`std::ostringstream` (menus: debug, contrib_levelset, joystick).

### R36S: never -static-libgcc with shared libstdc++

GDB on device showed:

```
#2 uw_init_context_1 ()
#3 _Unwind_Resume ()
#4 IFileStreambuf::IFileStreambuf(...)
#5 Config::load()
```

`Config::load()` throws when the config file is missing; the throw should be
caught higher up. With `-static-libgcc`, static `libgcc_eh` cannot unwind
through the **shared** ArkOS `libstdc++.so` → `abort` / signal 6. Same class
of bug as Windstille; fix is to drop `-static-libgcc` and rely on device
`libgcc_s.so.1` (already present on ArkOS). Keep `-fexceptions` and the
cxxabi_shim for missing `to_chars` symbols.

### Android: format UID via get_value() in SCRIPT_GUARD

NDK r26 libc++ still rejects `std::make_format_args(UID&)` even with
`formatter<UID, char>` (`__determine_arg_t` deleted). SCRIPT_GUARD macros
now log `m_uid.get_value()` (uint32_t). The formatter remains for desktop
paths and inherits `formatter<uint32_t>`.

### R36S: broken texture paths (`file.sprite/image.png`)

Symptom: hundreds of
`Couldn't load texture '/images/.../foo.sprite/bar.png': not found`
(and `tiles.stts/...`). The game runs with dummy textures.

Cause: `prio::ReaderDocument::get_directory()` used
`std::filesystem::path(filename).parent_path()`. Under the R36S ABI mix
(GCC 15 headers, `_GLIBCXX_USE_CXX11_ABI=0`, ArkOS libstdc++ ~9),
`parent_path` can fail to strip the leaf, so joins become
`dir/file.sprite/image.png`. Not a GLES2 NPOT/mipmap issue.

Fix: string-based dirname in `get_directory()` (same approach as
`FileSystem::dirname`).

### WASM: port/emscripten.hpp without AddonManager

Origins has no `src/addon/`. Stub the download callbacks in
`port/emscripten.hpp` instead of including `addon/addon_manager.hpp`.

### Android/R36S: skip xdgcpp

`#include <xdgcpp/xdg.h>` must be excluded for Android and R36S (no xdgcpp
in the NDK / ArkOS sysroot). Guard with `ANDROID` / `__ANDROID__` /
`SUPERTUX_R36S` as well as `__EMSCRIPTEN__` and `WIN32`.

### prio get_directory: copy optional string

`ReaderDocumentImpl::get_filename()` returns `std::optional<std::string>` **by
value**. Binding `std::string const&` to `*get_filename()` dangles and
segfaults in `test_prio` (`get_directory/0`). Always copy into a local
`std::string`.

### Android: Size in log_info / make_format_args

NDK libc++ rejects custom `formatter<Size, char>` in `make_format_args`
(same class as UID). Expand `Size` to `.width`/`.height` ints at log call
sites.





### Android: SDL_ttf stub / FreeType

Title screen failed with:

```
Couldn't load TTFFont: fonts/SuperTux-Medium.ttf: SDL_ttf stub (FreeType not linked)
```

`build-apk.sh` only staged `SDL_ttf.h` + a stub. Real path:

- Flake inputs `sdl2-ttf-src` + `freetype-src`
- Stage FreeType under `jni/freetype/` and `SDL_ttf.c` under `jni/`
- `freetype_Android.mk` + static `SDL2_ttf_static` linked into `libmain`
- Stub only if sources are missing

Default Android log level is DEBUG (tag SuperTux).

### Android: black screen / no logcat output

`logmich` writes to `std::cerr`. On Android that often appears under the
`SDL/stderr` tag (not `SDL`), so `adb logcat -s SDL:*` shows lifecycle only.

- Mirror all logmich lines to `__android_log_write(..., "SuperTux", ...)`.
- Default log level on Android is INFO.
- Early `main()` markers via the `SuperTux` tag.
- PhysFS search path is logged at WARN so it is always visible.

Useful filter:

```
adb logcat -s SuperTux:V SDL:V DEBUG:V
```

If the search path has no data archive, shaders/images fail and the frame
stays black — ensure `build-apk.sh` packaged `assets/data.zip`.

### Android: PHYSFS_init crash (PHYSFS_AndroidInit)

On Android, `PHYSFS_init(argv0)` does **not** take a C-string path. PhysFS
treats `argv0` as a `PHYSFS_AndroidInit*` (`jnienv` + `context`). Passing
`argv[0]` crashes inside `__PHYSFS_platformCalcBaseDir` when it casts the
pointer and calls JNI methods on garbage.

Fix in `PhysfsSubsystem`:
- Fill `PHYSFS_AndroidInit` via `SDL_AndroidGetJNIEnv()` / `SDL_AndroidGetActivity()`
  (valid once `SDL_main` is entered from `SDLActivity.nativeRunMain`, even before
  `SDL_Init`).
- Pass that struct to `PHYSFS_init`, then `DeleteLocalRef` the Activity.
- Mount `PHYSFS_getBaseDir()` (APK path) and `assets/data.zip` so data paths are
  `images/`, `levels/`, … without an `assets/` prefix.
- `build-apk.sh` packs `assets/data.zip` from the data tree for that mount.




### WASM: nix run / website packaging

`nix run .#supertux-wasm` failed because the derivation had no
`bin/` mainProgram (emscripten outputs lived only under
`share/…`). Match Pingus:

- Install `supertux-origins.{html,js,wasm,data}` at `$out/`
- `$out/bin/supertux-wasm` runs `mk/wasm/scripts/serve.sh` (local HTTP +
  browser, no-store cache headers)
- `meta.mainProgram = "supertux-wasm"`
- Optional HTML shell via `-DSUPERTUX_WASM_SHELL=` (mk/wasm/shell.html)

Serve env: `SUPERTUX_WASM_PORT` (default 8765), `APP_NAME`, `PKG`.

### WASM: file_packager preload path

`--preload-file ${BUILD_CONFIG_DATA_DIR}@/data` pointed at
`CMAKE_BINARY_DIR/data`, which is never populated →

```
file_packager: error: …/build/data@/data does not exist
```

Use `${BUILD_DATA_DIR}` (source `data/`) for the preload, and at runtime
mount the VFS path `/data` (not the host cmake binary path).

Also mark all JS-exported helpers in `src/port/emscripten.hpp` with
`EMSCRIPTEN_KEEPALIVE` so EXPORTED_FUNCTIONS resolve under LTO / GC.

### WASM: EXTRA_EXPORTED_RUNTIME_METHODS removed

Modern emscripten rejects `-sEXTRA_EXPORTED_RUNTIME_METHODS=...` ("No longer
supported, use EXPORTED_RUNTIME_METHODS"). Keep only
`-sEXPORTED_RUNTIME_METHODS=['ccall','cwrap']` (and EXPORTED_FUNCTIONS) on the
`supertux-origins` link line in CMakeLists.txt. Prefer `SHELL:-s...` form so
CMake does not mangle the brackets.

### WASM / cross: squirrel ExternalProject multiarch path

`ProvideSquirrel.cmake` used `CMAKE_LIBRARY_ARCHITECTURE` (e.g.
`wasm32-emscripten`) under `CMAKE_CROSSCOMPILING` for
`IMPORTED_LOCATION` / `BUILD_BYPRODUCTS`. The ExternalProject always
passes `-DCMAKE_INSTALL_LIBDIR=lib`, so squirrel installs to
`squirrel/ex/lib/libsquirrel_static.a` (no multiarch subdir). Result:

```
No rule to make target 'squirrel/ex/lib/wasm32-emscripten/libsquirrel_static.a'
```

Fix: force `SQUIRREL_MULTIARCH_DIR` empty while `CMAKE_INSTALL_LIBDIR=lib`
is set. Same issue would hit Android / R36S cross builds.

### Missing <sstream> on NDK / emscripten

libstdc++ often pulls  transitively; NDK libc++ and emscripten
do not. Any TU using  needs .
Batch-fixed across src/ for Android and WASM.

### Android: undefined format_error (linker)

NDK 26 with `-fexperimental-library` compiles `std::format` but
`libc++_shared` may lack `std::format_error` key functions (typeinfo,
vtable, destructor). Provide `mk/android/app/jni/format_error_stub.cpp`
(picked up by Android.mk RWILDCARD).

### WASM: userdir create_directory fails

`FileSystem::mkdir` used `create_directory` (single level). Emscripten VFS
lacked `/home/web_user/.local/share/` → exception on userdir setup.
Use `create_directories` and `FS.mkdirTree` before IDBFS mount.

### Android: undefined TTF_* at link

Separate `SDL2_ttf_static` was not pulled into `libmain` (NDK GC). Compile
`SDL_ttf.c` into `main` and link FreeType with `LOCAL_WHOLE_STATIC_LIBRARIES`.

### WASM: supertux_loadFiles is not defined

C++ calls `supertux_loadFiles` / `supertux_syncfs` via EM_ASM (config load /
frame loop). Custom `mk/wasm/shell.html` must define them (Origins userdir +
localStorage config mirror). Upstream template is under
`mk/emscripten/template.html.in`.

### Android: TTF symbols still undefined after FreeType staging

Module Android.mk is loaded as **jni/src/Android.mk** (`LOCAL_PATH=jni/src`),
not `jni/`. FreeType/SDL_ttf must be staged under `jni/src/freetype` and
`jni/src/SDL_ttf.c`, and `freetype_Android.mk` copied next to the module
makefile. Staging under `jni/` alone never matches the wildcards.

### Audit follow-ups (bundle 011)

- WASM link: `-lopenal`, `-sERROR_ON_UNDEFINED_SYMBOLS=1` (was 0).
- Android APK: after packing `assets/data.zip`, prune loose asset files so the
  APK does not double-ship the data tree.
- Env: `SUPERTUX_ORIGINS_DATA_DIR` / `SUPERTUX_ORIGINS_USER_DIR` preferred;
  legacy typo `SUPERTUX_ORIGNS_*` still accepted.
- FreeType: dropped non-standard `FT_CONFIG_OPTION_DISABLE_STREAM_SUPPORT`.
- Music: Android/WASM/R36S still force **modplug only** (vorbis/opus/mpg123
  off). Stock SuperTux music is mostly Ogg — expect silent BGM until Vorbis is
  enabled for those targets.

### WASM: images and fonts were offline stubs

Runtime failed with `SDL_ttf stub: font rendering not implemented offline` and
`IMG_Load_RW` returning null (SDL_GetError showed unrelated noise).

- `mk/emscripten/sdl_image_stub.c` now decodes via **stb_image**
  (`mk/emscripten/stb_image.h`).
- `ProvideSDL2_ttf.cmake` builds real **SDL_ttf + FreeType** when
  `-DSDL2_TTF_SOURCE_DIR=` and `-DFREETYPE_SOURCE_DIR=` are set (flake inputs).

### Follow-up vs Pingus/Windstille (bundle 013)

- **freetypeWasm**: Windstille-style `emcmake` FreeType in `nix/wasm.nix`
  (`FT_DISABLE_*`), passed as `FREETYPE_INCLUDE_DIRS` / `FREETYPE_LIBRARY`.
- PhysFS: strip leading `/` in `get_physfs_SDLRWops`.
- Image errors: prefer `IMG_GetError()` over stale `SDL_GetError()`.
- Link flags: `MIN_WEBGL_VERSION`/`MAX_WEBGL_VERSION`, `EXIT_RUNTIME=0` (Pingus).

### Vorbis on Android / WASM (bundle 014)

Stock SuperTux music is mostly `.ogg`. Enable `WSTSOUND_WITH_VORBIS`:

- **WASM:** `oggWasm` + `vorbisWasm` (emconfigure), `OGG_DIR` / `VORBISFILE_DIR`,
  `WSTSOUND_WITH_VORBIS=ON`.
- **Android:** `build-audio-libs.sh` builds static ogg/vorbis/vorbisfile per ABI;
  `Android.mk` prebuilts + `-DWSTSOUND_WITH_VORBIS=1` when `.a` present.
- **R36S:** still OFF until sysroot has vorbis.

### Vorbis static link order (bundle 015)

`FindVorbisfile.cmake` now also resolves `libvorbis` and `libogg` into
`Vorbisfile::vorbisfile` INTERFACE_LINK_LIBRARIES. Emscripten `USE_LINK_FLAGS`
adds `-lvorbisfile -lvorbis -logg` so static archives resolve under
`ERROR_ON_UNDEFINED_SYMBOLS=1`.

### Android libogg config_types.h (bundle 016)

Generated `config_types.h` must define `ogg_uint64_t` (and the usual
int16/32/64 types). Missing `ogg_uint64_t` breaks `framing.c` on NDK.

### WASM TTF_GetError + win32 packaging (bundle 018)

- **TTF_GetError**: real SDL_ttf defines it as `SDL_GetError` macro. Putting
  `mk/emscripten/SDL_ttf.h` on the global include path made the compiler emit
  calls to a non-existent `TTF_GetError` symbol. Stub header moved to
  `SDL_ttf_stub.h`; only the offline stub target exposes a shim `SDL_ttf.h`.
- **win32**: `supertux-origins-win32` must not package the native Linux build.
  Flat package now depends on a MinGW cross build (`supertux-origins-mingw64`).
  Marked `meta.broken` until the full Windows dep graph (sexpcpp/wstsound/…)
  links cleanly — better than shipping a Linux binary renamed as `.exe`.

### WASM IDBFS sync spam (bundle 020)

`ScreenManager::loop_iter` called `supertux_syncfs()` **every frame**, starting
overlapping `FS.syncfs` (IndexedDB) work → "N FS.syncfs operations in flight"
and severe slowdown. Now: C++ at most ~every 10s; JS single-flight + 8s
throttle; force sync on save / hide / unload only.

### Android FreeType missing modules (bundle 021)

Link needed `FT_Gzip_Uncompress` / `FT_Stream_OpenGzip|LZW` and sdf/svg
renderer classes. `freetype_Android.mk` now builds gzip/lzw/sdf/svg units and
uses `FT_CONFIG_OPTION_SYSTEM_ZLIB` (main already links `-lz`).

### OggSoundFile on Android

When Vorbis prebuilts exist, force `deps/wstsound/ogg_sound_file.cpp` onto
`LOCAL_SRC_FILES` if the recursive wildcard missed it.

### Desktop GLES2 package

`supertux-origins.nix` already supports `useGLES2 ? false`. Flake exposes
`packages.supertux-origins-gles2` with `useGLES2 = true`. Runtime `--renderer`
cannot select GLES2 independently of that compile flag.

### WASM music "Unknown file format" (bundle 022)

`external/wstsound/CMakeLists.txt` forced `WSTSOUND_WITH_VORBIS=OFF` under
EMSCRIPTEN/ANDROID (Pingus-style). SuperTux needs Vorbis. Removed that force.
Also accept any `OggS` page as Vorbis when Opus is not matched (packet offset
for "vorbis" is not fixed at +29).

### Android ogg_sound_file path

`build-apk.sh` was deleting `ogg_sound_file.cpp` then Android.mk force-added a
missing path. Keep the staged file; filter only when no libvorbisfile.

### Canvas resize

Shell called Pingus `_st_emscripten_canvas_resize`; Origins exports
`_set_resolution`. Wire that + postRun resize notify.

### WASM performance analysis (bundle 026)

Likely contributors to "incredible slow" + 100% CPU:

1. **Forced POT textures on GLES2** (`gl_needs_power_of_two() == true`) —
   every upload padded to next power-of-two with expensive CPU blits.
   WebGL does not need this for our clamp/non-mipmap path → **disabled on
   `__EMSCRIPTEN__`**.

2. **`assert_gl()` → `glGetError()` after many GL calls** — forces pipeline
   flush; very expensive on WebGL. **No-op on Emscripten** unless
   `SUPERTUX_GL_DEBUG`.

3. **`SDL_Delay` inside `loop_iter`** while using `emscripten_set_main_loop`
   (rAF). Redundant and can fight the browser scheduler → **skipped on
   Emscripten**.

4. **Structural costs still present**: lightmap full-scene pass, many small
   draw calls, `-fexceptions`, OpenAL stream refill, large data preload.
   Profile with browser Performance tab after (1)–(3).

5. **GL_NEAREST appearance**: default sampler is LINEAR; some assets may
   request nearest; stretched canvas (pre-resize fix) also looks blocky.


## Android linker: missing ft_bitmap_sdf_raster (FreeType SDF)

**Symptom** (ndk-build, armeabi-v7a / arm64-v8a):

```
ld.lld: error: undefined symbol: ft_bitmap_sdf_raster
>>> referenced by ftsdfrend.c
>>> ftsdfrend.o:(ft_bitmap_sdf_renderer_class) in archive .../libfreetype.a
```

**Cause:** `mk/android/app/jni/freetype_Android.mk` listed the SDF module
sources incompletely. FreeType’s `bsdf` (bitmap → SDF) renderer in
`ftsdfrend.c` references `ft_bitmap_sdf_raster`, which is defined in
`src/sdf/ftbsdf.c` (the `bsdf_raster_*` helpers). The Android.mk had
`ftsdf.c`, `ftsdfrend.c`, and `ftsdfcommon.c` but omitted `ftbsdf.c`.

**Fix:** Add `src/sdf/ftbsdf.c` to `LOCAL_SRC_FILES` in
`freetype_Android.mk`. No other SDF files are required for the current
FreeType version staged by the Android recipe.

## Android linker: missing OggSoundFile constructor (wstsound)

**Symptom:**

```
ld.lld: error: undefined symbol: wstsound::OggSoundFile::OggSoundFile(...)
>>> referenced by .../sound_file.o (make_unique<OggSoundFile>)
```

**Cause:** Two pieces were out of sync:

1. `mk/android/scripts/build-apk.sh` staged all of `external/wstsound/src/*.cpp`
   then **unconditionally** deleted `ogg_sound_file.cpp` (along with opus/mp3/EFX
   TUs), with a comment “wav + modplug only”.
2. When OpenAL + modplug + libogg/libvorbis prebuilts are present,
   `Android.mk` sets `SUPERTUX_HAVE_VORBIS := 1` and adds
   `-DWSTSOUND_WITH_VORBIS=1`, so `sound_file.cpp` compiles the
   `make_unique<OggSoundFile>` branch. The corresponding TU was already
   removed from the tree, so the constructor never linked.

SuperTux Origins ships `.ogg` music; the in-tree wstsound CMake already
keeps `WSTSOUND_WITH_VORBIS` on for Android (unlike Opus/MP3).

**Fix:** Stop deleting `ogg_sound_file.cpp` in `build-apk.sh`. Leave the
other always-off codecs removed. The existing `ifndef SUPERTUX_HAVE_VORBIS`
filter in `Android.mk` still drops the TU when Vorbis libs are not staged,
and the compile definitions stay consistent either way.


## WASM audio path (OpenAL → Web Audio) and silent failures

### Call chain

1. `main.cpp` constructs `SoundManager` after PhysFS mounts `/data`.
2. SuperTux `SoundManager` wraps `wstsound::SoundManager` with an
   `open_func` that opens files via `IFileStream` → PhysFS.
3. `wstsound::SoundManager` ctor calls `OpenALSystem::open_real_device()`
   → `alcOpenDevice(nullptr)` + context. On failure it nulls `m_openal`
   (dummy mode) and only prints to **stderr**.
4. SFX: `SoundManager::play` → `m_sound_mgr.sound().play(path)` →
   `SoundChannel::prepare` → load WAV/Ogg into static buffer.
5. Music: `play_music` → stream source from `.ogg` or `.music` descriptor.
6. Decode: `SoundFile::from_stream` magic-detects WAV / Ogg Vorbis /
   ModPlug (when `WSTSOUND_WITH_*` is on at compile time).
7. Emscripten OpenAL is Web Audio under the hood (`AL.currentCtx.audioCtx`).

### Why “no sound and no errors”

| Cause | Symptom | Fix / probe |
|-------|---------|-------------|
| OpenAL device failed | dummy mode; all play() no-ops | stderr “Couldn't initialize audio device”; now also `log_warning` at boot |
| `WSTSOUND_WITH_VORBIS=OFF` | `.ogg` music throws / dummy | `nix/wasm.nix` must pass `-DWSTSOUND_WITH_VORBIS=ON` (CMakeLists FORCE-ON for EMSCRIPTEN) |
| PhysFS path miss | load fails | probe `sounds/coin.wav` and `/music/misc/theme.ogg` at boot |
| Browser autoplay policy | device opens, buffers load, **still silent** until gesture | `st_emscripten_audio_resume` + shell unlock on pointer/key |
| `sound_enabled`/`music_enabled` false | intentional mute | config; logged on `enable_*` |

### Sanity checks added

- Boot: log OpenAL dummy vs real; PhysFS probe open of stock SFX + theme.ogg.
- `play` / `play_music`: log path, success, failures (not only music catch).
- `open_func`: log PhysFS open errors before rethrow.
- `is_audio_enabled()`: reflects dummy + enable flags (was hard-coded `true`).
- WASM shell: first `pointerdown`/`touchstart`/`keydown`/`mousedown` calls
  `_st_emscripten_audio_resume` (resumes `AL.currentCtx.audioCtx`).
- Exported: `_st_emscripten_audio_resume`, `_st_emscripten_audio_pause`.

### Expected console after a working load + click

```
SoundManager: OpenAL device opened successfully
SoundManager: WASM build — browser autoplay policy may mute until first click/tap
SoundManager: PhysFS probe open of sounds/coin.wav succeeded
SoundManager: PhysFS probe open of /music/misc/theme.ogg succeeded
…
SuperTux: audio unlock requested after user gesture
SuperTux: Web Audio context resumed (running)
SoundManager::play_music('…') started
```

If probes fail, check `--preload-file …@/data` and `PHYSFS_mount("/data")`.


## WASM shell URL flags → SuperTux argv

Shell (`mk/wasm/shell.html`) maps query params to `Module.arguments`:

| Query | Argv | Effect |
|-------|------|--------|
| `?verbose=1` | `--verbose` | logmich INFO |
| `?debug=1` | `--debug` | logmich DEBUG |
| `?developer=1` | `--developer` | developer_mode |

**Do not** use Pingus short flags here: in SuperTux `-v` is **version**
(exits after printing), and `-D` / `--developer-mode` are **unknown** and
abort `parse_args` before `SoundManager` is constructed — which looked like
“no audio logs at all”.

Default log level is WARNING. Audio boot probes use `log_warn` so they show
without query flags; `play`/`play_music` detail stays at info/debug.


## Android ndk-build was effectively serial (`-j` dropped)

**Symptom:** APK native build took forever despite `NIX_BUILD_CORES` being set.

**Cause:** In `mk/android/scripts/build-apk.sh` the `ndk-build` command was
broken across a blank line after a trailing `\`. Bash treated that as end of
the command, so the real invocation was only:

```bash
ndk-build NDK_PROJECT_PATH=… APP_BUILD_SCRIPT=…
```

without `NDK_APPLICATION_MK` and without `-j${NIX_BUILD_CORES}`. The following
lines (`NDK_APPLICATION_MK=… -jN`) were a separate statement, not arguments to
ndk-build. Result: single-job compile of the whole tree (and three ABIs in
`Application.mk`: armeabi-v7a, arm64-v8a, x86_64).

**Fix:** Run diagnostics first, then one continuous `ndk-build` line including
`-j${NIX_BUILD_CORES:-$(nproc)}`.

**Note:** ndk-build still walks ABIs one after another; `-j` parallelizes
within each ABI. For faster iteration, temporarily set `APP_ABI` to a single
ABI (e.g. `arm64-v8a` only) in `jni/Application.mk` / nix `targetAbis`.


## Build-system audit (2026-08-20)

### Functional Pingus leftovers fixed

| Location | Issue | Fix |
|----------|--------|-----|
| `mk/wasm/scripts/build-app.sh` | `-DPINGUS_USE_GLES` / `-DPINGUS_ENABLE_SOUND` / `-DDATA_PREFIX` no-ops for SuperTux | `-DENABLE_OPENGLES2=…`; drop dead flags |
| same | `-DEMSCRIPTEN_LINK_FLAGS=…` never read by CMakeLists | Removed; link flags come from `if(EMSCRIPTEN)` `USE_LINK_FLAGS` |
| `mk/r36s/toolchain-arkos-aarch64.cmake` | Forced `PINGUS_USE_GLES` / `PINGUS_ENABLE_SOUND` | `ENABLE_OPENGLES2` + `SUPERTUX_R36S` |
| `mk/wasm/shell.html` | Footer “Pingus”, `pingus-backup` JSON | SuperTux Origins + `supertux-backup` (still accepts old format) |
| `CMakeLists.txt` | EMSCRIPTEN did not force GLES2 option | Force `ENABLE_OPENGLES2` like Android |
| `mk/android/scripts/build-apk.sh` | Passed unused `PINGUS_VERSION` into ndk-build | Dropped (alias only for env default) |

### Intentional remaining references

- **flake inputs** `WindstilleTeam/*` — real upstream of tinycmmc family / wstsound.
- **Comments** “adapted from Pingus/Windstille” in scripts/docs — attribution.
- **`PORTS.md` / `AGENTS.md` / `PORTING.md`** — design notes.
- **`src/util/*` copyright headers** copied with code (line_iterator, currenton).
- **`serve.sh`**: `PINGUS_WASM_PORT` as env alias for port number — harmless fallback.
- **`?debug=1` still accepts legacy backup format `pingus-backup`** for old files.

### Quoting / shell pitfalls (watch list)

1. **Nix `''${…}`** — correct escape for `${` inside `''` strings; do not “simplify” to `$` or the outer Nix interpolates.
2. **`lib.escapeShellArgs cmakeFlags`** in `nix/wasm.nix` — prefer this over hand-joined strings when flags contain spaces.
3. **Bash `array[*]` vs `[@]`** — `"${arr[@]}"` preserves words; `"${arr[*]}"` joins with first char of IFS (was used for dead EMSCRIPTEN_LINK_FLAGS).
4. **Line continuations** — never leave a blank line after `\`; that already cost Android `-j` (see above).
5. **CMake `SHELL:-s…`** options must be separate `target_link_options` entries or carefully spaced in `CMAKE_EXE_LINKER_FLAGS`; nested quotes in `EXPORTED_FUNCTIONS=['_a','_b']` are fragile — keep the existing single-quoted form.

### Other obvious issues (not all fixed here)

- **Triple APP_ABI** on Android still multiplies compile time (parallel within ABI only).
- **`build-app.sh` LINK_FLAGS / PRELOAD arrays** are largely redundant with CMakeLists EMSCRIPTEN block when using the nix path (`nix/wasm.nix` calls `emcmake` directly).
- **R36S `arkos-sysroot` URL** is still `localhost:8888` placeholder — expected until a permanent host exists.
- **Default log level WARNING** — use `?verbose=1` / `?debug=1` (long options only).


## Android per-ABI flake packages

| Attribute | APP_ABI |
|-----------|--------|
| `.#supertux-android` | armeabi-v7a arm64-v8a x86_64 (default / full) |
| `.#supertux-android-arm64-v8a` | arm64-v8a only |
| `.#supertux-android-armeabi-v7a` | armeabi-v7a only |
| `.#supertux-android-x86_64` | x86_64 only |

SDL/audio prebuilts are still built for **all** ABIs once; only `libmain.so`
ndk-build is restricted. Use a single-ABI attr for day-to-day iteration.

`ndk-build` is invoked via a bash array so `-j${NIX_BUILD_CORES}` cannot
detach into a separate command (`-j12: command not found`).


## WASM silent audio despite OpenAL "opened" and play() logs

Observed: device opens, PhysFS probes OK, Web Audio state=running after
click, SoundManager::play logs — still no sound.

Likely causes:

1. 3D distance model — wstsound inverse distance (ref=128), SFX in pixel
   coords, listener at (camera, z=-300). Emscripten OpenAL attenuation can
   collapse gain. Mitigation: alDistanceModel(AL_NONE) + relative SFX at origin.
2. Decode path returns DummySoundSource (SoundChannel catches load errors).
3. Must link Emscripten -lopenal (Web Audio). Log AL_VENDOR/VERSION/RENDERER.
4. Autoplay — context running before alSourcePlay (st_emscripten_audio_resume).


## Android black screen + slow startup

Log stopped after SoundManager on a Fire tablet (KFAUWI). Video is created
*before* audio; so the hang is almost certainly later:

1. Resources::load() — multiple TTF faces from assets/data.zip (slow flash I/O)
2. TitleScreen — GameSession("levels/misc/menu.stlv") loads a full level
   (tiles, sprites, scripts) before the first frame

Black screen is expected until ScreenManager::run() draws; on slow devices
that can be tens of seconds with no log if progress messages are missing.

Mitigations added: log_warn checkpoints through init / TTF / TitleScreen;
skip WASM-style full decode audio probe on Android.


## Android multi-minute hang on TTF load

Cause: data is `APK (zip) → assets/data.zip (zip) → fonts/*.ttf`. FreeType
opens fonts via PhysFS SDL_RWops and **seeks constantly**. Each seek through
a nested zip re-walks the outer APK entry → extreme I/O on Fire/eMMC.

Fix: `TTFFont` now reads the entire font into memory and uses
`SDL_RWFromConstMem` + `TTF_OpenFontRW`. Menu level load still hits many
small files through the nested zip (slower than desktop, but sequential).

Desktop: wrap `supertux-origins` with `LD_LIBRARY_PATH` including
`xorg.libSM` / `libICE` (fixes `libSM.so.6: cannot open shared object file`).


## Android: drop nested assets/data.zip

Previously build-apk packed the whole data tree into `assets/data.zip` and
pruned loose files. PhysFS mounted APK then mounted that zip → every open/seek
went through two zip layers (multi-minute stalls on Fire eMMC).

Now:
- APK ships **loose** files under `assets/`
- `PHYSFS_mount(apk)` + `PHYSFS_setRoot(apk, "assets")` (PhysFS 3.2+) so
  game paths stay `images/`, `levels/`, …
- Legacy `assets/data.zip` still mounts if `setRoot` fails (old APKs)


## Android on-screen controls default On

`Config::mobile_controls` defaults to `true` on `__ANDROID__` (constructor
and config-file fallback). `SDL_GetNumTouchDevices()` is unreliable before
the window exists. `ENABLE_TOUCHSCREEN_SUPPORT` is set in the NDK flags so
the ANDROID_TV opt-out path compiles. Existing config files that already
saved `mobile_controls #f` keep that until the user toggles Options.


## Desktop GL: GLEW replaced by GLAD

Desktop OpenGL 3.3 uses vendored GLAD (`external/glad`, core 3.3 only).
`glewInit` → `gladLoadGL(SDL_GL_GetProcAddress)`. GLEW is no longer linked,
so the game binary should not pick up GLEW's forced `-lX11`. SDL may still
use X11 at runtime for the window; that is independent of the GL loader.


## Windows MinGW (`.#supertux-origins-mingw64`) vs Pingus

Pingus pattern (`mkPingus`):
- `pkgsCross.mingwW64.callPackage` for the game
- SDL2/SDL2_image from `SDL2-win32.packages.${system}."SDL2-win64"`
- C++ deps (logmich, strutcpp, wstsound, …) built with the **same** `pkgs'`
- Flat package + zip as a separate `runCommand`

SuperTux today:
- Same SDL2-* win64 flake inputs via `pickWinFlakePkg`
- **Still passes host Linux** sexpcpp/squirrel/wstsound/… into the cross
  `callPackage` — that will fail at build or link until those are mingw builds
- GLAD replaces GLEW (no glew-win32 required)
- `supertux-origins-win32` is only a flat re-layout of the mingw result

Nix note: never write `x or throw "msg"` without parentheses around
`(throw "msg")` — application binds tighter than `or`.


## Flake: no tinycmmc eachSystemWithPkgs; no libcurl

- Outputs use `flake-utils.lib.eachDefaultSystem` + explicit `pkgs = import nixpkgs { inherit system; … }` so system/pkgs are under our control (no bare `system` surprises).
- libcurl removed: unused in src; ProvideCurl always sets HAVE_LIBCURL false; curl-win32 input dropped (avoids broken MinGW curl build).


## Windows: wstsound + OpenAL (Pingus-style)

`wstsound` must be built with `pkgsCross.mingwW64`, not the host flake
package (host cmake config looks for Linux OpenAL). OpenAL Soft and
libmodplug come from `openal-soft-win32` / `libmodplug-win32` as
`openal-soft-win64` / `libmodplug-win64`. C++ deps (logmich, sexpcpp,
strutcpp, priocpp) are also cross-built for mingw64.


## Windows: squirrel IMPORTED_IMPLIB / static in-tree (2026-08)

CMake error under `pkgsCross.mingwW64`:

```
IMPORTED_IMPLIB not set for imported target "squirrel::sqstdlib"
configuration "Release".
```

Root cause: flake passed `squirrel.packages.${system}.default` (host Linux
grumnix/squirrel) into the MinGW `callPackage`. That package's cmake
config exports `squirrel::squirrel` / `squirrel::sqstdlib` without
Windows import-lib properties.

Fix (aligned with R36S / Android / wasm):

1. Pass `squirrel = null` for the mingw package.
2. `cmakeFlags += [ "-DUSE_SYSTEM_SQUIRREL=OFF" "-DSQUIRREL_SOURCE_DIR=${squirrel-src}" ]`
3. `ProvideSquirrel.cmake`: always create **STATIC** imported targets
   (`LibSquirrel` / `LibSqstdlib`) because ExternalProject is forced with
   `-DDISABLE_DYNAMIC=ON -DBUILD_SHARED_LIBS=OFF`. The old WIN32 branch
   assumed shared DLLs + `IMPORTED_IMPLIB` and pointed at non-existent
   paths under that configuration.
4. `supertux-origins.nix`: `squirrel ? null`, omit from `buildInputs`
   when null, guard the postFixup `*.dll` symlink.

Static linking means no squirrel DLL to ship in the flat win32 package.

## Windows: GLM endian.h / packing.inl (2026-08)

MinGW build fails with:

```
glm/gtc/packing.inl:14:10: fatal error: endian.h: No such file or directory
```

Cause: SuperTux (and vendored geomcpp) included `<glm/ext.hpp>`, which pulls
`gtc/packing`. GLM 1.0.x packing.inl includes Linux `<endian.h>` for
big-endian bitfield layout. MinGW has no endian.h.

Fix: include only `<glm/glm.hpp>` + `<glm/gtx/io.hpp>` (and
`gtx/rotate_vector` in geomcpp). Core geometric ops (length, normalize,
dot, distance) come from glm.hpp; packing is unused by SuperTux.

## Debug noise cleanup (2026-08)

Porting left several `log_warn` progress paths that drown real warnings:

- **WASM audio** (`e81d460`): keep functional mitigations
  (`alDistanceModel(AL_NONE)`, relative SFX at origin). Dropped per-play
  `log_warn`, PhysFS/decode success probes, and boot vendor spam →
  `log_debug` only. Failure paths stay `log_warning`.
- **Android init** (`f2ad40d`): `init: …` milestones and PhysFS search-path
  dump demoted to `log_debug` (raise log level when diagnosing boot hangs).
- **R36S**: already `CMAKE_BUILD_TYPE=Release` in `nix/r36s.nix` — not a
  full debug build.

## MinGW: FLT_MAX / cfloat

`colorspace_oklab.cpp` uses `FLT_MAX` without `<cfloat>`. Linux headers often
pull it transitively; MinGW does not → add the include.

## R36S: SDL2_ttf needs FreeType (ft2build.h)

Building SDL_ttf from `SDL2_TTF_SOURCE_DIR` without FreeType headers fails:

```
SDL_ttf.c:28:10: fatal error: ft2build.h: No such file or directory
```

ArkOS sysroot may lack SDL2_ttf; we compile SDL_ttf.c in-tree. FreeType was
not wired: `freetypeSrc` existed on the flake but was not passed into
`mkSuperTuxR36s`, and ProvideSDL2_ttf only resolved FreeType via
`find_package` (often empty under the hybrid sysroot).

Fix:
- `mkSuperTuxR36s` accepts `freetypeSrc` → `-DFREETYPE_SOURCE_DIR=…`
- ProvideSDL2_ttf (non-Emscripten SOURCE_DIR path): try explicit
  FREETYPE_*, FindFreetype, sysroot `ft2build.h` + `libfreetype`, then
  compile a minimal FreeType static lib from FREETYPE_SOURCE_DIR (same
  source list as the Emscripten path).

## FreeType: FT_CONFIG_OPTION_USE_HARFBUZZ is #ifdef, not value

Defining `FT_CONFIG_OPTION_USE_HARFBUZZ=0` still **defines** the macro, so
`ft-hb.h` includes `<hb.h>` and the R36S in-tree FreeType build fails.
Use `-UFT_CONFIG_OPTION_USE_HARFBUZZ` (same for PNG/BROTLI).

## MinGW result/bin/*.dll symlinks

`postFixup` used `ln -sf` into the Nix store. Fine for `nix run`, but
`result/bin/*.dll` looks like broken/odd links when listed. Now `cp -L`
real files into `$out/bin`. Flat package `.#supertux-origins-win32` already
used `cp --dereference` and is no longer marked broken.

## MinGW postFixup: cp same-file vs nixpkgs DLL links

Nixpkgs Windows `fixupPhase` already links runtime DLLs into `$out/bin`.
A second `cp -L src $out/bin/` then fails with `are the same file` when the
destination is a hardlink to the same store object. Materialize via
temp file + `mv -f` instead.

## Wine apps (nix run .#supertux-win32)

Pingus-style `mkWineApp` on Linux hosts:

```bash
nix run .#supertux-win32            # flat package (exe + DLLs + data/) under Wine
nix run .#supertux-origins-win32    # alias
nix run .#supertux-mingw64          # store layout (bin/*.exe)
```

Uses `wineWow64Packages.stable`, fresh `WINEPREFIX`, `WINEARCH=win64`, and
native SDL2* DLL overrides so the bundled MinGW DLLs are preferred.

## FreeType minimal build: ftmodule.h + ftgzip

Linking in-tree FreeType failed with undefined `t1_driver_class`,
`pcf_driver_class`, `FT_Gzip_Uncompress`, etc. Default
`include/freetype/config/ftmodule.h` registers every driver; we only
compile TTF/CFF/smooth/raster sources.

Fix: `mk/cmake/SuperTux/ftmodule_min.h` + `-DFT_CONFIG_MODULES_H="…"` and
add `src/gzip/ftgzip.c` for `FT_Gzip_Uncompress` (sfnt compressed tables).

## Win32 Wine: missing ogg.dll (transitive audio DLLs)

`libvorbis-0.dll` / `libopusfile-0.dll` need `ogg.dll` from libogg, which is
only a transitive dep of wstsound. postFixup used to copy only top-level
package `bin/*.dll`. Now scans `lib.closePropagation` of buildInputs for
all `*.dll` so the flat Wine package is complete.

## Wine app: writeShellScript must use buildPackages (not Windows host)

`nix run .#supertux-origins-win32` failed with bash not available on
`hostPlatform = x86_64-windows` when the wrapper was built with a Windows
`pkgs.writeShellScript`. Wine apps are Linux-only: gate on
`buildPlatform.isLinux && hostPlatform.isLinux`, and create the script with
`pkgs.buildPackages.writeShellScript`.

## Magnification zoom-out (R36S / small panels)

Design virtual max is ~1368x800; R36S is typically 640x480. Auto magnification
only scaled *up* when the window exceeded max_size, and `mobile_controls`
forced `scale = max(scale, 1)`. Sub-100% menu entries were also hidden under
`HIDE_NONMOBILE_OPTIONS` (on for R36S).

Now: auto uses the same fill formula in both directions (scale < 1 on small
panels), no mobile floor at 1.0, and 40–80% options always listed.

## Flake: Linux-only systems; helpers not in packages

- `eachSystem [ "x86_64-linux" "aarch64-linux" ]` — no Darwin (nixpkgs 26.11
  dropped x86_64-darwin; we do not ship macOS).
- `pickWinFlakePkg` lives in the per-system `let`, not under `packages`
  (`nix flake check` requires every packages.* to be a derivation).
- Wine apps only on `x86_64-linux` (wineWow64).
