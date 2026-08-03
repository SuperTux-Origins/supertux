// SPDX-FileCopyrightText: 2004 Tobias Glaesser <tobi.web@gmx.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <sys/types.h>
#include <ctype.h>

#include "defines.h"
#include "globals.h"
#include "setup.h"
#include "platform_config.h"
#include "intro.h"
#include "title.h"
#include "app_loop.h"
#include "gameloop.h"
#include "leveleditor.h"
#include "screen.h"
#include "worldmap.h"
#include "resources.h"
#include "texture.h"
#include "tile.h"

int main(int argc, char * argv[])
{
  st_log("SuperTux Milestone 1 %s starting", VERSION);
  /* Path options must be applied before directory/config setup. */
  parse_path_args(argc, argv);
  st_directory_setup();
  parseargs(argc, argv);

  /* One SDL_Init for all subsystems before any window/audio work.
     Video first matters for GLX on some drivers; matches working GL tests. */
  st_sdl_init();
  st_video_setup();
  st_audio_setup();
  st_joystick_setup();
  st_general_setup();
  st_print_init_status();
  st_menu();
  loadshared();

  if (launch_leveleditor_mode && level_startup_file)
    {
    leveleditor(level_startup_file);
    }
  else if (level_startup_file)
    {
      GameSession session(level_startup_file, 1, ST_GL_LOAD_LEVEL_FILE);
      session.run();
    }
  else
    {
#ifdef __EMSCRIPTEN__
      /* Single frame pump: title ↔ worldmap ↔ session (no nested busy loops). */
      app_run();
      /* app_run() uses emscripten_set_main_loop and does not return. */
#else
      title();
#endif
    }

#ifndef __EMSCRIPTEN__
  clearscreen(0, 0, 0);
  updatescreen();

  unloadshared();
  st_general_free();
  TileManager::destroy_instance();
#ifdef DEBUG
  Surface::debug_check();
#endif
  st_shutdown();
#endif

  return 0;
}
