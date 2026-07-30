LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

# Prebuilt SDL2 headers live in ../SDL/include (sibling under jni/).
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../SDL/include \
	$(LOCAL_PATH)

# Game sources are copied next to this Android.mk by the Nix build script.
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
	worldmap.cpp

# SDL2_image (stb backend) when the build script placed sources here.
ifneq ($(wildcard $(LOCAL_PATH)/SDL2_image/src/IMG.c),)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/SDL2_image/include
LOCAL_SRC_FILES += \
	SDL2_image/src/IMG.c \
	SDL2_image/src/IMG_stb.c \
	SDL2_image/src/IMG_bmp.c \
	SDL2_image/src/IMG_gif.c \
	SDL2_image/src/IMG_jpg.c \
	SDL2_image/src/IMG_lbm.c \
	SDL2_image/src/IMG_pcx.c \
	SDL2_image/src/IMG_png.c \
	SDL2_image/src/IMG_pnm.c \
	SDL2_image/src/IMG_svg.c \
	SDL2_image/src/IMG_tga.c \
	SDL2_image/src/IMG_tif.c \
	SDL2_image/src/IMG_webp.c \
	SDL2_image/src/IMG_xcf.c \
	SDL2_image/src/IMG_xpm.c \
	SDL2_image/src/IMG_xv.c \
	SDL2_image/src/IMG_qoi.c
LOCAL_CFLAGS += -DUSE_STBIMAGE -DLOAD_BMP -DLOAD_PNG -DLOAD_JPG -DLOAD_GIF
endif

# Sound optional — need SDL_mixer.h from a full SDL2_mixer install.
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
