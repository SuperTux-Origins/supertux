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
