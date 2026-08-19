# SuperTux Origins — jni Android.mk (WIP).
# SDL2 prebuilts expected under jni/SDL2 and jni/SDL2_image (see
# mk/android/scripts/install-sdl-libs.sh).
#
# Full source list is in supertux_sources.mk (376 files).  Until static
# deps (physfs, squirrel, tinycmmc, wstsound, …) are staged as prebuilts
# or built in-tree, keep LOCAL_SRC_FILES minimal so ndk-build validates.

LOCAL_PATH := $(call my-dir)

# --- prebuilt SDL2 ----------------------------------------------------------
include $(CLEAR_VARS)
LOCAL_MODULE := SDL2
LOCAL_SRC_FILES := SDL2/lib/$(TARGET_ARCH_ABI)/libSDL2.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/SDL2/include
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := SDL2_image
LOCAL_SRC_FILES := SDL2_image/lib/$(TARGET_ARCH_ABI)/libSDL2_image.so
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/SDL2_image/include
LOCAL_SHARED_LIBRARIES := SDL2
include $(PREBUILT_SHARED_LIBRARY)

# Optional: include full source list once deps are ready
# include $(LOCAL_PATH)/supertux_sources.mk

# --- game shared library ----------------------------------------------------
include $(CLEAR_VARS)
LOCAL_MODULE := main
# Placeholder until SUPERTUX_SOURCES + deps are wired:
LOCAL_SRC_FILES := placeholder.cpp
# When ready:
# LOCAL_SRC_FILES := $(SUPERTUX_SOURCES)
LOCAL_C_INCLUDES := \
  $(LOCAL_PATH)/SDL2/include \
  $(LOCAL_PATH)/SDL2_image/include \
  $(LOCAL_PATH)/../../../../src \
  $(LOCAL_PATH)/../../../../external
LOCAL_SHARED_LIBRARIES := SDL2 SDL2_image
LOCAL_LDLIBS := -lGLESv2 -llog -landroid -lEGL
LOCAL_CPPFLAGS := -std=c++17 -frtti -fexceptions -DANDROID -DUSE_OPENGLES2
LOCAL_CFLAGS := -DANDROID -DUSE_OPENGLES2
include $(BUILD_SHARED_LIBRARY)
