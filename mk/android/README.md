# Android packaging (SuperTux Origins)

Scaffolding adapted from [Pingus](https://github.com/Pingus/pingus) `mk/android/`.

## Layout (to be completed)

- `scripts/build-sdl-libs.sh` — static/shared SDL2 (+ image) via NDK
- `scripts/build-audio-libs.sh` — OpenAL / related
- `scripts/build-apk.sh` — package APK from `app/` tree
- `scripts/install-sdl-libs.sh` — install prebuilts into jni tree

## Project-specific work still needed

1. Create `mk/android/app/` with `AndroidManifest.xml`, `jni/Android.mk`,
   resources, and SDLActivity wiring (singleTask lifecycle).
2. Point `Android.mk` at SuperTux sources + tinycmmc / squirrel / wstsound /
   physfs static or shared objects.
3. Force GLES2 (`ENABLE_OPENGLES2=ON`); NDK r27+ for modern STL / `std::format`.
4. GameController / touch mappings (see PORTING.md).
5. Wire `nix/android.nix` + flake `packages.supertux-android` / APK output.

Until the app tree exists, `nix/android.nix` remains a reusable library of
helpers imported by the flake, not a complete APK derivation.

## Current skeleton

`app/AndroidManifest.xml` — SDLActivity, singleTask, landscape, GLES2 feature  
`app/jni/Android.mk` — prebuilt SDL2/SDL2_image stubs + placeholder `main`  
`app/jni/Application.mk` — c++_shared, armeabi-v7a + arm64-v8a, API 22  

Next: expand `LOCAL_SRC_FILES` for SuperTux, add physfs/squirrel/wstsound,
stage stb_image if needed, wire `nix/android.nix` `mkApk`.

## Source list

`app/jni/supertux_sources.list` / `.mk` list all 376 `src/**/*.cpp` files.
Regenerate after adding sources:

```bash
./mk/android/scripts/generate-source-list.sh
```

Uncomment `include …/supertux_sources.mk` and set `LOCAL_SRC_FILES` in
`Android.mk` once physfs/squirrel/wstsound/tinycmmc are available as NDK
modules or prebuilts.

## Toolchain notes

- `Application.mk` targets API 22+, C++20, GLES2 defines.
- Prebuilt SDL2 / SDL2_image: `scripts/build-sdl-libs.sh` then `install-sdl-libs.sh`.
- Full game binary still needs physfs, squirrel, logmich, sexpcpp, priocpp
  (sexp), wstsound, tinycmmc as NDK static/shared modules or prebuilts under
  `app/jni/`. Prefer building those from `external/` with the NDK toolchain.

## Building against `external/`

Once SDL prebuilts are in place, the next dependencies to stage under
`app/jni/` (static `.a` or shared `.so` per ABI) are, in order:

1. logmich, sexpcpp, strutcpp, tinycmmc headers
2. priocpp (`-DPRIO_USE_JSONCPP=OFF -DPRIO_USE_SEXPCPP=ON`)
3. physfs, squirrel, wstsound

Use the NDK standalone toolchain / cmake with
`-DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake`.
