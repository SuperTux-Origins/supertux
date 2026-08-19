# SuperTux Origins — top-level jni Android.mk (WIP).
# SDL2 prebuilts are expected under jni/SDL2 and jni/SDL2_image (see
# mk/android/scripts/build-sdl-libs.sh / install-sdl-libs.sh).
#
# This file is a placeholder until the full source list and static deps
# (physfs, squirrel, tinycmmc, wstsound, …) are wired.  See mk/android/README.md.

LOCAL_PATH := $(call my-dir)

# SDL2 prebuilt (installed by install-sdl-libs.sh)
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

# Game shared library — source list TBD
include $(CLEAR_VARS)
LOCAL_MODULE := main
LOCAL_SRC_FILES := placeholder.cpp
LOCAL_SHARED_LIBRARIES := SDL2 SDL2_image
LOCAL_LDLIBS := -lGLESv2 -llog -landroid
LOCAL_CPPFLAGS := -std=c++17 -DANDROID -DENABLE_OPENGLES2
include $(BUILD_SHARED_LIBRARY)
