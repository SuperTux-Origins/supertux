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

# Optional Vorbis (stock .ogg music)
ifneq ($(wildcard $(LOCAL_PATH)/../audio/$(TARGET_ARCH_ABI)/lib/libvorbisfile.a),)
include $(CLEAR_VARS)
LOCAL_MODULE := ogg
LOCAL_SRC_FILES := ../audio/$(TARGET_ARCH_ABI)/lib/libogg.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../audio/include
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := vorbis
LOCAL_SRC_FILES := ../audio/$(TARGET_ARCH_ABI)/lib/libvorbis.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../audio/include
LOCAL_STATIC_LIBRARIES := ogg
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE := vorbisfile
LOCAL_SRC_FILES := ../audio/$(TARGET_ARCH_ABI)/lib/libvorbisfile.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../audio/include
LOCAL_STATIC_LIBRARIES := vorbis ogg
include $(PREBUILT_STATIC_LIBRARY)

SUPERTUX_HAVE_VORBIS := 1
endif

endif


# ---------------------------------------------------------------------------
# FreeType + SDL_ttf
# ndk-build loads this file as jni/src/Android.mk → LOCAL_PATH = jni/src.
# Stage FreeType at jni/src/freetype/ and SDL_ttf.c next to this Makefile.
# ---------------------------------------------------------------------------
ifneq ($(wildcard $(LOCAL_PATH)/freetype/include/ft2build.h),)
  FREETYPE_SAVE_PATH := $(LOCAL_PATH)
  LOCAL_PATH := $(FREETYPE_SAVE_PATH)/freetype
  include $(FREETYPE_SAVE_PATH)/freetype_Android.mk
  LOCAL_PATH := $(FREETYPE_SAVE_PATH)
  SUPERTUX_HAVE_FREETYPE := 1
endif

ifneq ($(wildcard $(LOCAL_PATH)/SDL_ttf.c),)
  SUPERTUX_SDL_TTF_SRC := SDL_ttf.c
endif

ifdef SUPERTUX_SDL_TTF_SRC
  ifdef SUPERTUX_HAVE_FREETYPE
    SUPERTUX_HAVE_SDL_TTF := 1
  endif
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
# PhysFS: only Android + POSIX platform backends (others are empty or fail).
LOCAL_SRC_FILES := $(filter-out %/physfs_platform_windows.c %/physfs_platform_winrt.cpp \
	%/physfs_platform_os2.c %/physfs_platform_qnx.c %/physfs_platform_haiku.cpp \
	%/physfs_platform_unix.c %/physfs_platform_apple.m,$(LOCAL_SRC_FILES))
# FreeType sources live under jni/freetype (built as static lib); never into main.
# Skip ogg decoder TU when Vorbis libs were not staged.
ifndef SUPERTUX_HAVE_VORBIS
LOCAL_SRC_FILES := $(filter-out %/ogg_sound_file.cpp,$(LOCAL_SRC_FILES))
endif
LOCAL_SRC_FILES := $(filter-out freetype/% %/freetype/%,$(LOCAL_SRC_FILES))
# Real SDL_ttf is a static module; drop the stub when present.
# Always provide TTF symbols: real SDL_ttf.c or explicit stub path.
ifdef SUPERTUX_HAVE_SDL_TTF
LOCAL_SRC_FILES := $(filter-out %/sdl_ttf_stub.c sdl_ttf_stub.c,$(LOCAL_SRC_FILES))
LOCAL_SRC_FILES += $(SUPERTUX_SDL_TTF_SRC)
LOCAL_CFLAGS += -DTTF_USE_HARFBUZZ=0
LOCAL_CPPFLAGS += -DTTF_USE_HARFBUZZ=0
else
# Permanent fallback next to Android.mk (also may exist under src/ from build-apk).
LOCAL_SRC_FILES := $(filter-out %/sdl_ttf_stub.c sdl_ttf_stub.c,$(LOCAL_SRC_FILES))
LOCAL_SRC_FILES += sdl_ttf_stub.c
endif

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
LOCAL_WHOLE_STATIC_LIBRARIES += openal
LOCAL_STATIC_LIBRARIES += modplug
LOCAL_LDLIBS += -lOpenSLES
ifdef SUPERTUX_HAVE_VORBIS
LOCAL_STATIC_LIBRARIES += vorbisfile vorbis ogg
endif
endif

ifdef SUPERTUX_HAVE_SDL_TTF
LOCAL_WHOLE_STATIC_LIBRARIES += freetype
LOCAL_C_INCLUDES += $(LOCAL_PATH)/freetype/include
endif

LOCAL_CPPFLAGS := -std=c++20 -frtti -fexceptions -fexperimental-library \
	-DANDROID -DUSE_OPENGLES2 -DUSE_SDL2 \
	-DGLM_ENABLE_EXPERIMENTAL \
	-DWSTSOUND_WITH_MODPLUG=1 \
	-DWSTSOUND_WITH_MPG123=0 \
	-DWSTSOUND_WITH_OPUS=0 \
	-DWSTSOUND_WITH_EFX=0 \
	-DPRIO_USE_SEXPCPP=1 \
	-DTINYGETTEXT_WITH_SDL=1

ifdef SUPERTUX_HAVE_VORBIS
LOCAL_CPPFLAGS += -DWSTSOUND_WITH_VORBIS=1
LOCAL_CFLAGS += -DWSTSOUND_WITH_VORBIS=1
else
LOCAL_CPPFLAGS += -DWSTSOUND_WITH_VORBIS=0
LOCAL_CFLAGS += -DWSTSOUND_WITH_VORBIS=0
endif

LOCAL_CFLAGS := -DANDROID -DUSE_OPENGLES2 -DUSE_SDL2 \
	-DGLM_ENABLE_EXPERIMENTAL \
	-DWSTSOUND_WITH_MODPLUG=1 \
	-DWSTSOUND_WITH_MPG123=0 \
	-DPRIO_USE_SEXPCPP=1 \
	-DTINYGETTEXT_WITH_SDL=1

include $(BUILD_SHARED_LIBRARY)
