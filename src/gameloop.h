// SPDX-FileCopyrightText: 2004 Bill Kendrick <bill@newbreedsoftware.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SUPERTUX_GAMELOOP_H
#define SUPERTUX_GAMELOOP_H

#ifndef NOSOUND
#include "sound.h"
#endif
#include "type.h"
#include "level.h"
#include "world.h"

/* GameLoop modes */

#define ST_GL_PLAY 0
#define ST_GL_TEST 1
#define ST_GL_LOAD_GAME 2
#define ST_GL_LOAD_LEVEL_FILE  3
#define ST_GL_DEMO_GAME  4

extern int game_started;

class World;

/** The GameSession class controlls the controll flow of a World, ie.
    present the menu on specifc keypresses, render and update it while
    keeping the speed and framerate sane, etc. */
class GameSession
{
 private:
  Timer frame_timer;
  Timer endsequence_timer;
  /** Non-blocking intro / end / result overlays (shared by app_loop + run()). */
  enum OverlayKind { OVERLAY_NONE, OVERLAY_INTRO, OVERLAY_ENDSCREEN, OVERLAY_RESULT };
  OverlayKind overlay;
  Timer overlay_timer;
  unsigned int overlay_min_ms;
  int pending_exit; /* ExitStatus */
  World* world;
  int st_gl_mode;
  int levelnb;
  unsigned int last_update_time;
  unsigned int update_time;
  int pause_menu_frame;
  int debug_fps;
#ifdef TSCONTROL
  int old_mouse_y;
#endif

  /** If true the end_sequence will be played, user input will be
      ignored while doing that */
  enum EndSequenceState {
    NO_ENDSEQUENCE,
    ENDSEQUENCE_RUNNING, // tux is running right
    ENDSEQUENCE_WAITING  // waiting for the end of the music
  };
  EndSequenceState end_sequence;
  float last_x_pos;

  bool game_pause;

  // FIXME: Hack for restarting the level
  std::string subset;

 public:
  enum ExitStatus { ES_NONE, ES_LEVEL_FINISHED, ES_GAME_OVER, ES_LEVEL_ABORT };
 private:
  ExitStatus exit_status;
 public:

  Timer time_left;

  GameSession(const std::string& subset, int levelnb, int mode);
  ~GameSession();

  /** Enter the busy loop (calls frame() until exit). */
  ExitStatus run();

  /** Setup before a frame() pump (used by app_loop / run). */
  void begin_run();

  /** One iteration of the game loop. Returns true while still running. */
  bool frame();

  ExitStatus get_exit_status() const { return exit_status; }

  void draw();
  void action(double frame_ratio);

  Level* get_level() { return world ? world->get_level() : 0; }
  World* get_world() { return world; }

  static GameSession* current() { return current_; }
 private:
  static GameSession* current_;

  void restart_level();

  void check_end_conditions();
  void start_timers();
  void process_events();

  void levelintro();
  void draw_levelintro();
  void draw_endscreen_content();
  void draw_resultscreen_content();
  /** True while an overlay still owns the frame. */
  bool process_overlay();
  void drawstatus();
  void drawendscreen();
  void drawresultscreen(void);

 private:
  void on_escape_press();
  void process_menu();
};

std::string slotinfo(int slot);

bool rectcollision(base_type* one, base_type* two);
void bumpbrick(float x, float y);

#endif /*SUPERTUX_GAMELOOP_H*/

