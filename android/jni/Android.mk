LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../SDL/include \
	$(LOCAL_PATH)

LOCAL_SRC_FILES := \
	badguy.cpp \
	button.cpp \
	collision.cpp \
	configfile.cpp \
	gameloop.cpp \
	game_file.cpp \
	gameobjs.cpp \
	globals.cpp \
	gles2_renderer.cpp \
	high_scores.cpp \
	intro.cpp \
	level.cpp \
	leveleditor.cpp \
	lispreader.cpp \
	menu.cpp \
	mousecursor.cpp \
	particlesystem.cpp \
	physic.cpp \
	platform_sdl2.cpp \
	player.cpp \
	resources.cpp \
	scene.cpp \
	screen.cpp \
	setup.cpp \
	special.cpp \
	sprite.cpp \
	sprite_manager.cpp \
	supertux.cpp \
	text.cpp \
	texture.cpp \
	tile.cpp \
	timer.cpp \
	title.cpp \
	touch_controls.cpp \
	type.cpp \
	world.cpp \
	worldmap.cpp \
	img_stb_min.c

# Unmodified stb_image.h is placed next to sources by the build script (STB_IMAGE_H).
LOCAL_C_INCLUDES += $(LOCAL_PATH)

# Audio on by default (SDL2_mixer prebuilt with the SDL layer).
# Pass SUPER_TUX_ENABLE_SOUND=0 to force NOSOUND.
SUPER_TUX_ENABLE_SOUND ?= 1
ifeq ($(SUPER_TUX_ENABLE_SOUND),1)
LOCAL_SRC_FILES += \
	music_manager.cpp \
	musicref.cpp \
	sound.cpp
LOCAL_SHARED_LIBRARIES := SDL2 SDL2_mixer
else
LOCAL_CFLAGS += -DNOSOUND
LOCAL_CPPFLAGS += -DNOSOUND
LOCAL_SHARED_LIBRARIES := SDL2
endif

# GLES2 is the default accelerated path on Android. ES 2.0 has been required
# by the Android CDD for many years; API 22 / Fire OS 5 devices all expose it.
LOCAL_LDLIBS := -llog -landroid -lz -lGLESv2

LOCAL_CFLAGS += -DUSE_SDL2 -DUSE_GLES2
LOCAL_CPPFLAGS += -DUSE_SDL2 -DUSE_GLES2 -std=c++98
# SUPERTUX_VERSION injected by nix/scripts/build-android-apk.sh (VERSION+g<rev>).
ifndef SUPERTUX_VERSION
SUPERTUX_VERSION := 0.1.5-dev
endif
LOCAL_CFLAGS += -DSUPERTUX_MILESTONE1_VERSION=\"$(SUPERTUX_VERSION)\"
LOCAL_CPPFLAGS += -DSUPERTUX_MILESTONE1_VERSION=\"$(SUPERTUX_VERSION)\"
LOCAL_CFLAGS += -DDATA_PREFIX=\".\"
LOCAL_CPPFLAGS += -DDATA_PREFIX=\".\"

LOCAL_CPP_FEATURES := exceptions rtti

include $(BUILD_SHARED_LIBRARY)
