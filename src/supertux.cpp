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
#include "text.h"
#include <string.h>

/** Case-insensitive suffix match; ext includes the dot (e.g. ".stl"). */
static bool
path_has_ext(const char* path, const char* ext)
{
  if (!path || !ext)
    return false;
  size_t n = strlen(path);
  size_t m = strlen(ext);
  if (n < m)
    return false;
  for (size_t i = 0; i < m; ++i)
    {
      char a = path[n - m + i];
      char b = ext[i];
      if (a >= 'A' && a <= 'Z') a = (char)(a + ('a' - 'A'));
      if (b >= 'A' && b <= 'Z') b = (char)(b + ('a' - 'A'));
      if (a != b)
        return false;
    }
  return true;
}

/** Basename of path (last component); may be empty. */
static const char*
path_basename(const char* path)
{
  if (!path)
    return "";
  const char* s = strrchr(path, '/');
  const char* b = strrchr(path, '\\');
  if (b && (!s || b > s))
    s = b;
  return s ? s + 1 : path;
}

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
      /* Direct jump by extension:
           .stl  → level session
           .stwm → worldmap
           .txt  → story/credits text scroller
         Unknown extensions keep the historical level-file behaviour. */
      if (path_has_ext(level_startup_file, ".stwm"))
        {
          WorldMapNS::WorldMap worldmap;
          worldmap.loadmap(level_startup_file);
          worldmap.display();
        }
      else if (path_has_ext(level_startup_file, ".txt"))
        {
          const char* base = path_basename(level_startup_file);
          bool credits = false;
          if (base && base[0]
              && (strncmp(base, "CREDITS", 7) == 0
                  || strncmp(base, "credits", 7) == 0))
            credits = true;
          const char* bg = credits
            ? "/images/background/oiltux.jpg"
            : "/images/background/arctis2.jpg";
          float speed = credits ? SCROLL_SPEED_CREDITS : SCROLL_SPEED_MESSAGE;
          display_text_file(level_startup_file, bg, speed);
        }
      else
        {
          /* .stl or any other path treated as a level file. */
          GameSession session(level_startup_file, 1, ST_GL_LOAD_LEVEL_FILE);
          session.run();
        }
    }
  else
    {
      /* Unified frame pump: title ↔ worldmap ↔ session (no nested busy loops).
         Emscripten: does not return; desktop: returns for teardown below. */
      app_run();
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
