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

### WASM: no USE_SDL_IMAGE under nix sandbox

`-sUSE_SDL_IMAGE=2` pulls the zlib emscripten port from the network
(DNS fails offline). Compile with `-sUSE_SDL=2` only and ship
`mk/emscripten/SDL_image.h` as a minimal stub include.

### WASM: seed EM_CACHE with SDL2 port zip

Nix build sandbox has no network. `pkgs.fetchurl` (fixed-output) downloads
SDL release-2.32.10.zip at eval/build of that derivation; preBuild copies it
into `$EM_CACHE/ports/` and `$EM_CACHE/downloads/` so emscripten ports/sdl2.py
does not call urlopen.


### R36S: static libstdc++ + _dl_find_object stub

Compiling with GCC 15 libstdc++ headers emits references to symbols not present
in ArkOS's older libstdc++.so (`std::to_chars` for floats via std::format,
`std::__throw_bad_array_new_length`, `__cxa_call_terminate`). Linking the
sysroot libstdc++ therefore fails at the final executable link.

Fix: `-static-libstdc++ -static-libgcc` from the cross toolchain so those
symbols come from GCC 15 archives. Static libgcc_eh still wants `_dl_find_object`
(glibc 2.35+); provide a weak stub in `mk/r36s/dl_find_object_stub.c` that
returns -1 (fallback unwind path).

