# SuperTux Origins — jni module Android.mk
# SDL2 prebuilts live in sibling jni/SDL/ (see nix/android.nix sdlPrebuiltAndroidMk).

LOCAL_PATH := $(call my-dir)

# ---------------------------------------------------------------------------
# Prebuilt OpenAL Soft + libmodplug
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
# libmain — full game + staged deps (build-apk.sh copies src/ + external into here)
# ---------------------------------------------------------------------------
include $(CLEAR_VARS)

LOCAL_MODULE := main

# Recursive collect of staged .cpp/.c under jni/src (game tree + deps/*).
RWILDCARD = $(foreach d,$(wildcard $1*),$(call RWILDCARD,$d/,$2) $(filter $(subst *,%,$2),$d))
LOCAL_SRC_FILES := $(patsubst $(LOCAL_PATH)/%,%,$(call RWILDCARD,$(LOCAL_PATH)/,%.cpp))
LOCAL_SRC_FILES += $(patsubst $(LOCAL_PATH)/%,%,$(call RWILDCARD,$(LOCAL_PATH)/,%.c))

# Never ship the Android scaffold placeholder once real sources are present.
LOCAL_SRC_FILES := $(filter-out placeholder.cpp,$(LOCAL_SRC_FILES))
# Platform / optional backends not built on Android.
LOCAL_SRC_FILES := $(filter-out %/win32/% win32/%,$(LOCAL_SRC_FILES))
LOCAL_SRC_FILES := $(filter-out %/json_reader_impl.cpp %/json_writer_impl.cpp %/jsonpretty_writer_impl.cpp,$(LOCAL_SRC_FILES))
# squirrel ships an interpreter tool we do not need.
LOCAL_SRC_FILES := $(filter-out %/sq/sq.c %/sq/sq.cpp deps/squirrel/sq/%,$(LOCAL_SRC_FILES))
# Prefer static squirrel objects only (exclude shared-only stubs if any).
LOCAL_SRC_FILES := $(filter-out %/sqstdlib/%/sqstdlib.cpp,$(LOCAL_SRC_FILES))

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../SDL/include \
	$(LOCAL_PATH)/../SDL/include/SDL2 \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/../external_includes \
	$(LOCAL_PATH)/../external_includes/argpp \
	$(LOCAL_PATH)/../external_includes/strut \
	$(LOCAL_PATH)/../external_includes/prio \
	$(LOCAL_PATH)/../external_includes/wstsound \
	$(LOCAL_PATH)/../external_includes/logmich \
	$(LOCAL_PATH)/../external_includes/sexp \
	$(LOCAL_PATH)/../external_includes/geom \
	$(LOCAL_PATH)/deps \
	$(LOCAL_PATH)/deps/argpp \
	$(LOCAL_PATH)/deps/priocpp \
	$(LOCAL_PATH)/deps/wstsound \
	$(LOCAL_PATH)/deps/strutcpp \
	$(LOCAL_PATH)/deps/logmich \
	$(LOCAL_PATH)/deps/sexpcpp \
	$(LOCAL_PATH)/deps/squirrel/include \
	$(LOCAL_PATH)/deps/physfs/src \
	$(LOCAL_PATH)/deps/SDL_SavePNG \
	$(LOCAL_PATH)/deps/obstack

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

LOCAL_CPPFLAGS := -std=c++20 -frtti -fexceptions -fexperimental-library \
	-DANDROID -DUSE_OPENGLES2 -DUSE_SDL2 \
	-DGLM_ENABLE_EXPERIMENTAL \
	-DWSTSOUND_WITH_MODPLUG=1 \
	-DWSTSOUND_WITH_MPG123=0 \
	-DWSTSOUND_WITH_VORBIS=0 \
	-DWSTSOUND_WITH_OPUS=0 \
	-DWSTSOUND_WITH_EFX=0 \
	-DPRIO_USE_SEXPCPP=1

LOCAL_CFLAGS := -DANDROID -DUSE_OPENGLES2 -DUSE_SDL2 \
	-DGLM_ENABLE_EXPERIMENTAL \
	-DWSTSOUND_WITH_MODPLUG=1 \
	-DWSTSOUND_WITH_MPG123=0 \
	-DPRIO_USE_SEXPCPP=1

include $(BUILD_SHARED_LIBRARY)
