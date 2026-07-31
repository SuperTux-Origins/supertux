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

# SDL2_image: stb-only backend for PNG/JPG/BMP/GIF/TGA (no libpng/jpeg/webp).
# Compile only IMG.c + IMG_stb.c and force every optional codec off so IMG.c
# does not reference IMG_LoadAVIF_RW / IMG_LoadJXL_RW / etc.
ifneq ($(wildcard $(LOCAL_PATH)/SDL2_image/src/IMG.c),)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/SDL2_image/include
LOCAL_SRC_FILES += \
	SDL2_image/src/IMG.c \
	SDL2_image/src/IMG_stb.c
LOCAL_CFLAGS += \
	-DUSE_STBIMAGE \
	-DLOAD_BMP=1 -DLOAD_GIF=1 -DLOAD_JPG=1 -DLOAD_PNG=1 -DLOAD_TGA=1 \
	-DLOAD_AVIF=0 -DLOAD_JXL=0 -DLOAD_LBM=0 -DLOAD_PCX=0 -DLOAD_PNM=0 \
	-DLOAD_QOI=0 -DLOAD_SVG=0 -DLOAD_TIF=0 -DLOAD_WEBP=0 \
	-DLOAD_XCF=0 -DLOAD_XPM=0 -DLOAD_XV=0 -DLOAD_XXX=0
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
