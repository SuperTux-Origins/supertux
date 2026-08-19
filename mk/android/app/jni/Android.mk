# SuperTux Origins — jni module Android.mk
# SDL2 prebuilts live in sibling jni/SDL/ (see nix/android.nix sdlPrebuiltAndroidMk).
# Do not redeclare PREBUILT SDL2 here with relative paths under jni/src/ — that
# is what made ndk-build look for jni/src/SDL2/lib/... and fail.

LOCAL_PATH := $(call my-dir)

# ---------------------------------------------------------------------------
# Prebuilt OpenAL Soft + libmodplug (per-ABI static libs from AUDIO_ANDROID_LIBS)
# ---------------------------------------------------------------------------
ifeq ($(ENABLE_ANDROID_SOUND),1)

include $(CLEAR_VARS)
LOCAL_MODULE := openal
LOCAL_SRC_FILES := ../audio/$(TARGET_ARCH_ABI)/lib/libopenal.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../audio/include $(LOCAL_PATH)/../audio/include/AL
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := modplug
LOCAL_SRC_FILES := ../audio/$(TARGET_ARCH_ABI)/lib/libmodplug.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../audio/include
include $(PREBUILT_STATIC_LIBRARY)

endif

# ---------------------------------------------------------------------------
# libmain — placeholder until full SUPERTUX_SOURCES + deps are wired
# ---------------------------------------------------------------------------
include $(CLEAR_VARS)

LOCAL_MODULE := main

# Placeholder so ndk-build validates the prebuilt / packaging pipeline.
# When ready, switch to RWILDCARD over this tree (game + deps/*) like Pingus.
ifneq ($(wildcard $(LOCAL_PATH)/placeholder.cpp),)
LOCAL_SRC_FILES := placeholder.cpp
else
# Collect any staged .cpp (deps + game) once sources are copied in.
RWILDCARD = $(foreach d,$(wildcard $1*),$(call RWILDCARD,$d/,$2) $(filter $(subst *,%,$2),$d))
LOCAL_SRC_FILES := $(patsubst $(LOCAL_PATH)/%,%,$(call RWILDCARD,$(LOCAL_PATH)/,%.cpp))
LOCAL_SRC_FILES += $(patsubst $(LOCAL_PATH)/%,%,$(wildcard $(LOCAL_PATH)/*.c))
LOCAL_SRC_FILES := $(filter-out %/win32/% win32/%,$(LOCAL_SRC_FILES))
LOCAL_SRC_FILES := $(filter-out %/json_reader_impl.cpp %/json_writer_impl.cpp %/jsonpretty_writer_impl.cpp,$(LOCAL_SRC_FILES))
endif

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../SDL/include \
	$(LOCAL_PATH)/../SDL/include/SDL2 \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/../external_includes \
	$(LOCAL_PATH)/deps

ifeq ($(ENABLE_ANDROID_SOUND),1)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../audio/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../audio/include/AL
endif

LOCAL_SHARED_LIBRARIES := SDL2

LOCAL_LDLIBS := -llog -landroid -lz -lGLESv2 -lEGL

ifeq ($(ENABLE_ANDROID_SOUND),1)
LOCAL_WHOLE_STATIC_LIBRARIES := openal
LOCAL_STATIC_LIBRARIES := modplug
LOCAL_LDLIBS += -lOpenSLES
endif

LOCAL_CPPFLAGS := -std=c++20 -frtti -fexceptions -DANDROID -DUSE_OPENGLES2 -DUSE_SDL2
LOCAL_CFLAGS := -DANDROID -DUSE_OPENGLES2 -DUSE_SDL2

include $(BUILD_SHARED_LIBRARY)
