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
	gameobjs.cpp \
	globals.cpp \
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
	type.cpp \
	world.cpp \
	worldmap.cpp \
	img_stb_min.c

# Unmodified stb_image.h is placed next to sources by the build script (STB_IMAGE_H).
LOCAL_C_INCLUDES += $(LOCAL_PATH)

ifeq ($(SUPER_TUX_ENABLE_SOUND),1)
LOCAL_SRC_FILES += \
	music_manager.cpp \
	musicref.cpp \
	sound.cpp
else
LOCAL_CFLAGS += -DNOSOUND
LOCAL_CPPFLAGS += -DNOSOUND
endif

LOCAL_SHARED_LIBRARIES := SDL2
LOCAL_LDLIBS := -llog -landroid -lz

LOCAL_CFLAGS += -DUSE_SDL2 -DNOOPENGL
LOCAL_CPPFLAGS += -DUSE_SDL2 -DNOOPENGL -std=c++98
LOCAL_CFLAGS += -DSUPERTUX_MILESTONE1_VERSION=\"0.1.5-dev\"
LOCAL_CPPFLAGS += -DSUPERTUX_MILESTONE1_VERSION=\"0.1.5-dev\"
LOCAL_CFLAGS += -DDATA_PREFIX=\".\"
LOCAL_CPPFLAGS += -DDATA_PREFIX=\".\"

LOCAL_CPP_FEATURES := exceptions rtti

include $(BUILD_SHARED_LIBRARY)
