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

## Windows (MinGW)

- [x] Force in-tree squirrel for mingw (USE_SYSTEM_SQUIRREL=OFF + squirrel-src);
      ProvideSquirrel always STATIC imported targets (no missing IMPORTED_IMPLIB).
- [x] Drop glm/ext.hpp (packing.inl → endian.h) for MinGW; use glm.hpp + gtx/io.
- [x] Clean WASM/Android log_warn debug residue; keep AL_NONE + relative SFX.
- [x] colorspace_oklab.cpp: #include <cfloat> for FLT_MAX (MinGW).
- [x] R36S: wire freetypeSrc + ProvideSDL2_ttf FreeType resolution (ft2build.h).
- [x] FreeType -U HARFBUZZ/PNG/BROTLI; MinGW postFixup copies real DLLs.
- [x] nix run Wine apps: supertux-win32 / mingw64 (Pingus mkWineApp).
- [x] R36S FreeType: custom ftmodule_min.h + ftgzip (link errors).
- [x] Magnification can zoom out (auto + menu) for R36S/small screens.
- [ ] Complete cross graph for remaining C++ deps; produce .exe + DLLs.
- [ ] Unbreak `supertux-origins-win32` / zip once mingw64 package succeeds.

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
- [x] Prefer stb_image (or existing image path) over system jpeg/png.
- [x] Produce complete `packages.x86_64-linux.supertux-wasm` and serve app.
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
- [x] jni links physfs / squirrel / wstsound / full external stack
      (fix FreeType SDF ftbsdf.c + keep ogg_sound_file.cpp when Vorbis staged).
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
- [x] Link `-lopenal` on WASM (runtime confirmation pending)

## WASM performance / IDBFS (needs cleanup)

- [ ] **IDBFS sync strategy** — was calling `FS.syncfs` every frame (severe
      slowdown + "operations in flight" spam). Mitigated in bundle 020 with
      ~10s C++ throttle + JS single-flight/8s debounce. Still needs a proper
      design: dirty-bit on writes only, coalesce on level exit / options save,
      optional disable during gameplay. See PORTING.md.

## WASM audio

- [x] Trace path + boot probes (OpenAL dummy?, PhysFS `sounds/coin.wav` /
      `/music/misc/theme.ogg`); log play/play_music failures.
- [x] `st_emscripten_audio_resume` / `pause` + shell unlock on first user gesture.
- [x] `nix/wasm.nix` `WSTSOUND_WITH_VORBIS=ON` (was OFF, conflicting with
      CMakeLists FORCE-ON for EMSCRIPTEN).
- [ ] **Confirm at runtime** in browser console after rebuild (user feedback).
      Expected: OpenAL opened, PhysFS probes OK, Web Audio resumed after click.

## Desktop GLES2 validation

- SuperTux Origins selects GLES2 via **compile-time** `USE_OPENGLES2`
  (`ENABLE_OPENGLES2`), not a separate runtime `--renderer gles2`.
  `--renderer` is only `auto` / `opengl` / `null` (opengl33 core path).
- A dedicated `.#supertux-origins-gles2` package is useful to **validate the
  mobile shader path on desktop** (Windstille/Pingus style), not required for
  players who only use OpenGL 3.3.
- [ ] Add optional `packages.supertux-origins-gles2` with `-DENABLE_OPENGLES2=ON`
      once desktop GLES2 context creation is verified (SDL_GL_CONTEXT_PROFILE_ES).

## WASM performance (investigation)

See PORTING.md "WASM performance analysis". Mitigations in bundle 026:
NPOT on WebGL, assert_gl no-op, no SDL_Delay in rAF loop.
Still open: lightmap cost, draw-call batching, exception overhead, audio.



## Upstream external/ changes (multi-step)

Goal: collect, merge, and upstream the porting-related fixes that landed in
`external/` across SuperTux-Origins, Pingus and Windstille so that the
individual library repositories (and ultimately a cleaned `external/` tree)
benefit. Project-specific hacks should be generalised or moved into the
consuming project's CMake / flake; only library-generic changes go upstream.

### Inventory of shared externals

| Library      | Present in          | Upstream (approx.)                  | Deps (minimal first) |
|--------------|---------------------|-------------------------------------|----------------------|
| tinycmmc     | all three           | grumbel / tinycmmc family           | none (CMake modules) |
| logmich      | Pingus, SuperTux, Windstille | grumbel/logmich                  | none / std            |
| strutcpp     | all                 | grumbel/strutcpp                    | logmich?              |
| geomcpp      | all                 | grumbel/geomcpp                     | none                  |
| argpp        | all                 | grumbel/argpp                       | none                  |
| sexpcpp      | all                 | lispparser/sexp-cpp or grumbel      | none                  |
| priocpp      | Pingus, SuperTux    | grumbel/priocpp                     | sexpcpp?              |
| xdgcpp       | Pingus, SuperTux    | grumbel/xdgcpp                      | none                  |
| tinygettext  | Pingus, SuperTux    | tinygettext upstream + local        | none                  |
| wstsound     | all                 | grumbel/wstsound                    | OpenAL, codecs        |
| miniswig     | SuperTux, Windstille| grumbel/miniswig                    | none                  |
| squirrel     | SuperTux            | upstream squirrel + local flake     | none                  |
| glad / obstack / SDL_SavePNG | SuperTux only | various                             | n/a                   |

Order for upstreaming: **tinycmmc → logmich / geomcpp / argpp / sexpcpp / xdgcpp → strutcpp / priocpp → wstsound / tinygettext / miniswig**.

### Observed change categories (to merge)

- C++20 `std::format` / `std::print` polyfills and Win32 / MinGW compatibility
  (Pingus "Use std::print polyfill for Win32 too").
- Exception type cleanups (`std::runtime_error` → `std::invalid_argument` in
  argpp parsers) – prefer the stricter type if it does not break callers.
- EMSCRIPTEN / ANDROID / R36S CMake flags (force static, disable optional
  codecs, MODPLUG paths, no system find_package for wrong arch).
- Include hygiene for GCC 15 / MinGW (`<cfloat>`, `<cstdlib>`, etc.).
- logmich → Android logcat tag plumbing (generalise to a compile-time tag
  or callback, not hard-code "SuperTux").
- FreeType / HarfBuzz / PNG / Brotli disable flags for constrained targets.
- flake.nix / *.nix updates that are library-generic (not project packaging).

### Steps (this will take multiple bundles)

1. [~] Per-library: checkout each library's upstream (or the newest common
   base from Pingus/Windstille/SuperTux), apply the union of non-conflicting
   diffs, drop pure-project code (e.g. SuperTux-specific log tags, Windstille
   VirtualGamepad mentions that leaked into a lib).
   - tinycmmc: aligned Export/FindDependency with Windstille (top-level-only
     install + multi-candidate external/ search + double-add guard). Kept
     SuperTux FindOgg/FindVorbisfile vanished-path + static-link hygiene and
     VERSION-first GetProjectVersion. Still needs real upstream repo round-trip.
2. [ ] Resolve conflicts in favour of the most general fix; document in
   PORTING.md why a change stayed project-local.
3. [ ] Produce one git bundle **per library** (or small dependency group)
   that can be applied to the library's own repository. Naming:
   `libname-001-…` or continue the global `supertux-0NN-upstream-libname-…`
   sequence so numbers never collide.
4. [ ] After upstream PRs / merges, re-vendor the cleaned sources into
   SuperTux / Pingus / Windstille `external/` (final step, not yet).
5. [ ] Keep `external/` in SuperTux temporarily as a staging area; do not
   delete local patches until the upstream round-trip is verified.

### Notes

- Some "external/" trees contain their own flake.nix / flake.lock; those
  should also be brought into line with the library's real upstream flake.
- Chained deps mean tinycmmc and logmich first: everything else may
  `find_package` them.
- Prefer clean, minimal patches over large rewrites. If a change is only
  needed for one platform, guard it with `#if defined(EMSCRIPTEN)` etc.
  rather than forking the library.

See also PORTING.md for the concrete compiler / linker errors that drove
many of these patches.
