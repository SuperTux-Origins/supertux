APP_STL := c++_shared
# Default local/dev template: one ABI. Nix flake overrides APP_ABI via a
# generated Application.mk (all ABIs for .#supertux-android, one for
# .#supertux-android-arm64-v8a etc.).
APP_ABI := arm64-v8a
APP_PLATFORM := android-22
APP_CPPFLAGS := -std=c++20 -frtti -fexceptions -DUSE_OPENGLES2 -DANDROID
