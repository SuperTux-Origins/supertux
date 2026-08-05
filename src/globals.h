// SPDX-FileCopyrightText: 2004 Bill Kendrick <bill@newbreedsoftware.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SUPERTUX_GLOBALS_H
#define SUPERTUX_GLOBALS_H

#include <string>
#include "platform_config.h"
#include "text.h"
#include "menu.h"
#include "mousecursor.h"

#ifdef GP2X
#define GP2X_BUTTON_UP              (0)
#define GP2X_BUTTON_DOWN            (4)
#define GP2X_BUTTON_LEFT            (2)
#define GP2X_BUTTON_RIGHT           (6)
#define GP2X_BUTTON_UPLEFT          (1)
#define GP2X_BUTTON_UPRIGHT         (7)
#define GP2X_BUTTON_DOWNLEFT        (3)
#define GP2X_BUTTON_DOWNRIGHT       (5)
#define GP2X_BUTTON_CLICK           (18)
#define GP2X_BUTTON_A               (12)
#define GP2X_BUTTON_B               (13)
#define GP2X_BUTTON_X               (15)
#define GP2X_BUTTON_Y               (14)
#define GP2X_BUTTON_L               (10)
#define GP2X_BUTTON_R               (11)
#define GP2X_BUTTON_START           (8)
#define GP2X_BUTTON_SELECT          (9)
#define GP2X_BUTTON_VOLUP           (16)
#define GP2X_BUTTON_VOLDOWN         (17)
#endif

extern std::string datadir;

struct JoystickKeymap
{
#ifndef GP2X
  int a_button;
  int b_button;
  int start_button;

  int x_axis;
  int y_axis;
  
  int dead_zone;

  JoystickKeymap();
#else
  int a_button;
  int b_button;
  int start_button;
  int up_button;
  int down_button;
  int left_button;
  int right_button;
  int volup_button;
  int voldown_button;

  JoystickKeymap();
#endif
};

extern JoystickKeymap joystick_keymap;

#ifdef USE_SDL2
/** Logical actions → SDL_GameControllerButton values (remappable).
 *  D-Pad movement uses left/right/up/duck buttons.
 *  Analog stick movement always uses LEFTX/LEFTY with analog_dead_zone.
 *  Up is for worldmap / menu navigation; bind D-Pad Up to Jump if you want
 *  that button to jump in-level. */
struct GameControllerKeymap
{
  int jump;
  int fire;
  int duck;   /* D-Pad down */
  int left;   /* D-Pad left */
  int right;  /* D-Pad right */
  int up;     /* D-Pad up (worldmap / menu nav) */
  int menu;
  /** Half-range stick dead zone for LEFTX/LEFTY (0..32767). Default 16000. */
  int analog_dead_zone;

  GameControllerKeymap();
};
extern GameControllerKeymap gamecontroller_keymap;
#endif


extern SDL_Surface * screen;
extern Text* black_text;
extern Text* gold_text;
extern Text* silver_text;
extern Text* white_text;
extern Text* white_small_text;
extern Text* white_big_text;
extern Text* blue_text;
extern Text* red_text;
extern Text* green_text;
extern Text* yellow_nums;

extern MouseCursor * mouse_cursor;

extern bool use_gl;
extern bool use_joystick;
extern bool use_fullscreen;
/** OpenGL: linear filtering when scaling sprites / the 640×480 frame. */
extern bool use_texture_filtering;
extern bool debug_mode;
extern bool verbose_mode;
extern bool show_fps;
extern bool show_mouse;

/** Always-on log: SDL_Log on SDL2 (logcat on Android), fprintf on SDL 1.2. */
void st_log(const char* fmt, ...);
/** SDL_Delay, or no-op when the Emscripten app_loop owns pacing. */
void st_frame_delay(unsigned int ms);

/** Verbose diagnostics (no-op unless verbose_mode); routes through st_log. */
void st_vlog(const char* fmt, ...);

/** The number of the joystick that will be use in the game */
extern int joystick_num;
extern char* level_startup_file;
extern bool launch_leveleditor_mode;

/* XDG paths (desktop):
 *   st_dir      — $XDG_CONFIG_HOME/supertux-milestone1  (config, user levels)
 *   st_save_dir — $XDG_STATE_HOME/supertux-milestone1   (save slots)
 * --userdir overrides both under one root (saves in <userdir>/save). */
extern char* st_dir;
extern char* st_save_dir;

extern float game_speed;
#ifdef USE_SDL2
/** Open gamepad via SDL_GameController (SDL2 only). */
extern SDL_GameController* game_controller;
#else
extern SDL_Joystick * js;
#endif

int wait_for_event(SDL_Event& event,unsigned int min_delay = 0, unsigned int max_delay = 0, bool empty_events = false);

#endif /* SUPERTUX_GLOBALS_H */
