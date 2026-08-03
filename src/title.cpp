// SPDX-FileCopyrightText: 2000 Bill Kendrick <bill@newbreedsoftware.com>
// SPDX-FileCopyrightText: 2004 Tobias Glaesser <tobi.web@gmx.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "platform_config.h"
#include <SDL_image.h>

#ifndef WIN32
#include <sys/types.h>
#include <ctype.h>
#endif

#include "defines.h"
#include "globals.h"
#include "title.h"
#include "screen.h"
#include "high_scores.h"
#include "menu.h"
#include "texture.h"
#include "timer.h"
#include "setup.h"
#include "game_file.h"
#include "level.h"
#include "gameloop.h"
#include "leveleditor.h"
#include "scene.h"
#include "player.h"
#include "math.h"
#include "tile.h"
#include "resources.h"
#include "worldmap.h"
#include "touch_controls.h"
#ifndef NOSOUND
#include "sound.h"
#endif

static Surface* bkg_title;
static Surface* logo;
static Surface* img_choose_subset;

static bool walking;
static Timer random_timer;

static int frame;
static unsigned int last_update_time;
static unsigned int update_time;

static std::vector<LevelSubset*> contrib_subsets;
static std::string current_contrib_subset;

static string_list_type worldmap_list;
static GameSession* demo_session = 0;
static SDL_Event title_last_event;

void free_contrib_menu()
{
  for(std::vector<LevelSubset*>::iterator i = contrib_subsets.begin();
      i != contrib_subsets.end(); ++i)
    delete *i;

  contrib_subsets.clear();
  contrib_menu->clear();
}

void generate_contrib_menu()
{
#ifdef RES320X240
  fadeout();
#endif

  string_list_type level_subsets = dsubdirs("/levels", "info");

  free_contrib_menu();

  contrib_menu->additem(MN_LABEL,"Bonus Levels",0,0);
  contrib_menu->additem(MN_HL,"",0,0);

  for (int i = 0; i < level_subsets.num_items; ++i)
    {
      LevelSubset* subset = new LevelSubset();
      subset->load(level_subsets.item[i]);
      contrib_menu->additem(MN_GOTO, subset->title.c_str(), i,
          contrib_subset_menu, i);
      contrib_subsets.push_back(subset);
    }

  for(int i = 0; i < worldmap_list.num_items; i++)
    {
    WorldMapNS::WorldMap worldmap;
    worldmap.loadmap(worldmap_list.item[i]);
    contrib_menu->additem(MN_ACTION, worldmap.get_world_title(),0,0, i + level_subsets.num_items);
    }

  contrib_menu->additem(MN_HL,"",0,0);
  contrib_menu->additem(MN_BACK,"Back",0,0);

  string_list_free(&level_subsets);
}

void check_contrib_menu()
{
  int index = contrib_menu->check();
  if (index == -1)
    return;

  if (index < (int)contrib_subsets.size())
    {
      // FIXME: This shouln't be busy looping
      LevelSubset& subset = * (contrib_subsets[index]);

      current_contrib_subset = subset.name;
 
      contrib_subset_menu->clear();
 
      contrib_subset_menu->additem(MN_LABEL, subset.title, 0,0);
      contrib_subset_menu->additem(MN_HL,"",0,0);

      for (int i = 0; i < subset.levels; ++i)
        {
        /** get level's title */
        Level level;
        level.load(subset.name, i+1);
        contrib_subset_menu->additem(MN_ACTION, level.name, 0,0,i+1);
        }

      contrib_subset_menu->additem(MN_HL,"",0,0);      
      contrib_subset_menu->additem(MN_BACK, "Back", 0, 0);
      }
    else if(index < worldmap_list.num_items + (int)contrib_subsets.size())
      {
      // Loading fade
      fadeout();

      WorldMapNS::WorldMap worldmap;
      worldmap.loadmap(worldmap_list.item[index - contrib_subsets.size()]);
//      worldmap.set_levels_as_solved();
      std::string savegame = worldmap_list.item[index - contrib_subsets.size()];
      // remove .stwm...
      savegame = savegame.substr(0, savegame.size()-5);
      savegame = std::string(st_save_dir) + "/" + savegame + ".stsg";
      std::cout << "SaveGameName: " << savegame << "\n";
      worldmap.loadgame(savegame.c_str());

      worldmap.display();

      Menu::set_current(main_menu);
      }
}

void check_contrib_subset_menu()
{
  int index = contrib_subset_menu->check();
  if (index != -1)
    {
      if (contrib_subset_menu->get_item_by_id(index).kind == MN_ACTION)
        {
          std::cout << "Starting level: " << index << std::endl;
          GameSession session(current_contrib_subset, index, ST_GL_PLAY);
          session.run();
          player_status.reset();
          Menu::set_current(main_menu);
        }
    }  
}

void draw_background()
{
  /* Draw the title background: */

  bkg_title->draw_bg();
}

void draw_demo(GameSession* session, double frame_ratio)
{
  World* world  = session->get_world();
  World::set_current(world);
  Level* plevel = session->get_level();
  Player* tux = world->get_tux();

#ifndef NOSOUND
  world->play_music(LEVEL_MUSIC);
#endif
  
  global_frame_counter++;
  tux->key_event((SDLKey) keymap.right,DOWN);
  
  if(random_timer.check())
    {
      if(walking)
        tux->key_event((SDLKey) keymap.jump,UP);
      else
        tux->key_event((SDLKey) keymap.jump,DOWN);
    }
  else
    {
      random_timer.start(rand() % 3000 + 3000);
      walking = !walking;
    }

  // Wrap around at the end of the level back to the beginnig
  if((plevel->width * 32) - 320 < tux->base.x)
    {
      tux->level_begin();
      scroll_x = 0;
    }

  tux->can_jump = true;
  float last_tux_x_pos = tux->base.x;
  world->action(frame_ratio);
  

  // disabled for now, since with the new jump code we easily get deadlocks
  // Jump if tux stays in the same position for one loop, ie. if he is
  // stuck behind a wall
  if (last_tux_x_pos == tux->base.x)
    {
      walking = false;
    }

  world->draw();
}

/* --- TITLE SCREEN --- */
static void
title_init(void)
{
  random_timer.init(true);

  walking = true;

  st_pause_ticks_init();

  delete demo_session;
  demo_session = new GameSession(datadir + "/levels/misc/menu.stl", 0, ST_GL_DEMO_GAME);

  clearscreen(0, 0, 0);
  updatescreen();

  /* Load images: */
  bkg_title = new Surface(datadir + "/images/title/background.jpg", IGNORE_ALPHA);
  logo = new Surface(datadir + "/images/title/logo.png", USE_ALPHA);
  img_choose_subset = new Surface(datadir + "/images/status/choose-level-subset.png", USE_ALPHA);

  /* Contrib / bonus worldmaps: every .stwm under levels/worldmaps/ except the
     main campaign map. On Android, directory listing may miss zip-injected
     assets — probe known names that open_game_file can still read. */
  string_list_init(&worldmap_list);

  string_list_type files = dfiles("levels/worldmaps/", ".stwm", NULL);
  for(int i = 0; i < files.num_items; ++i) {
    if(strcmp(files.item[i], "world1.stwm") == 0)
      continue;
    string_list_add_item(&worldmap_list, files.item[i]);
  }
  string_list_free(&files);

  /* Fallback probes when listing returned nothing (or missed a map). */
  {
    static const char* known_bonus_stwm[] = {
      "bonusisland.stwm",
      "bonus_island.stwm",
      "bonus.stwm",
      "world2.stwm",
      0
    };
    for (int k = 0; known_bonus_stwm[k]; ++k)
      {
        int already = 0;
        for (int i = 0; i < worldmap_list.num_items; ++i)
          if (strcmp(worldmap_list.item[i], known_bonus_stwm[k]) == 0)
            { already = 1; break; }
        if (already)
          continue;
        std::string probe = datadir + "/levels/worldmaps/" + known_bonus_stwm[k];
        if (game_file_exists(probe))
          {
            st_vlog("[data] worldmap probe hit: %s\n", known_bonus_stwm[k]);
            string_list_add_item(&worldmap_list, known_bonus_stwm[k]);
          }
      }
  }
  st_vlog("[data] bonus worldmap_list count=%d\n", worldmap_list.num_items);

  frame = 0;

  /* Draw the title background: */
  bkg_title->draw_bg();

  update_time = last_update_time = st_get_ticks();
  random_timer.start(rand() % 2000 + 2000);

  memset(&title_last_event, 0, sizeof(title_last_event));
  Menu::set_current(main_menu);
}

static void
title_shutdown(void)
{
  free_contrib_menu();
  string_list_free(&worldmap_list);
  delete bkg_title;
  bkg_title = 0;
  delete logo;
  logo = 0;
  delete img_choose_subset;
  img_choose_subset = 0;
  delete demo_session;
  demo_session = 0;
}

bool
title_frame(void)
{
  if (!Menu::current())
    return false;

  // if we spent to much time on a menu entry
  if( (update_time - last_update_time) > 1000)
    update_time = last_update_time = st_get_ticks();

  // Calculate the movement-factor
  double frame_ratio = ((double)(update_time-last_update_time))/((double)FRAME_RATE);
  if(frame_ratio > 1.5) /* Quick hack to correct the unprecise CPU clocks a little bit. */
    frame_ratio = 1.5 + (frame_ratio - 1.5) * 0.85;
  /* Lower the frame_ratio that Tux doesn't jump to hectically throught the demo. */
  frame_ratio /= 2;

  SDL_Event event;
  while (SDL_PollEvent(&event))
    {
      title_last_event = event;
      bool want_escape = false;
      if (touch_controls_process_event(event, &want_escape))
        continue;
      /* Title always has a menu; want_escape while none is unused. */
      (void)want_escape;
     // FIXME: QUIT signal should be handled more generic, not locally
      if (event.type == SDL_QUIT)
        Menu::set_current(0);
    }

  if (!Menu::current())
    return false;

  /* Draw the background: */
  draw_demo(demo_session, frame_ratio);

  if (Menu::current() == main_menu)
    logo->draw( 160, 30);


#ifndef RES320X240
  white_small_text->draw(" SuperTux " VERSION "\n"
                         "Copyright (c) 2003 SuperTux Devel Team\n"
                         "This game comes with ABSOLUTELY NO WARRANTY. This is free software, and you\n"
                         "are welcome to redistribute it under certain conditions; see the file COPYING\n"
                         "for details.\n",
                         0, 420, 0);
#else
  white_small_text->draw(" SuperTux " VERSION "\n"
                         "Copyright (c) 2003 SuperTux Devel Team\n"
                         "This game comes with ABSOLUTELY NO \n"
                         "WARRANTY. This is free software, and\n"
                         "you are welcome to redistribute it\n"
                         "under certain conditions; see the file\n"
                         "COPYING for details.\n",
                         0, 360, 0);
#endif
#ifndef NOSOUND
#ifdef GP2X
  updateSound();
#endif
#endif

  /* Don't draw menu, if quit is true */
  Menu* menu = Menu::current();
  if(menu)
    {
      menu->draw();
      menu->action();

      if(menu == main_menu)
        {
#ifndef NOSOUND
          MusicManager* music_manager;
          MusicRef menu_song;
#endif
          switch (main_menu->check())
            {
            case MNID_STARTGAME:
              // Start Game, ie. goto the slots menu
              update_load_save_game_menu(load_game_menu);
              break;
            case MNID_CONTRIB:
              // Contrib Menu
              puts("Entering contrib menu");
              generate_contrib_menu();
              break;
            case MNID_LEVELEDITOR:
              leveleditor();
              Menu::set_current(main_menu);
              break;
            case MNID_CREDITS:
#ifndef NOSOUND
              music_manager = new MusicManager();
#ifdef GP2X
              menu_song  = music_manager->load_music(datadir + "/music/credits.xm");
#else
              menu_song  = music_manager->load_music(datadir + "/music/credits.ogg");
#endif
              music_manager->halt_music();
              music_manager->play_music(menu_song,0);
#endif
              display_text_file("CREDITS", bkg_title, SCROLL_SPEED_CREDITS);
#ifndef NOSOUND
              music_manager->halt_music();
              menu_song = music_manager->load_music(datadir + "/music/theme.mod");
              music_manager->play_music(menu_song);
#endif
              Menu::set_current(main_menu);
              break;
            case MNID_QUITMAINMENU:
              Menu::set_current(0);
              break;
            }
        }
      else if(menu == options_menu)
        {
          process_options_menu();
        }
      else if(menu == load_game_menu)
        {
          if(title_last_event.type == SDL_KEYDOWN
             && title_last_event.key.keysym.sym == SDLK_DELETE)
            {
            int slot = menu->get_active_item_id();
            char str[1024];
            sprintf(str,"Are you sure you want to delete slot %d?", slot);

            draw_background();

            if(confirm_dialog(str))
              {
              sprintf(str,"%s/slot%d.stsg", st_save_dir, slot);
              printf("Removing: %s\n",str);
              remove(str);
              }

            update_load_save_game_menu(load_game_menu);
            update_time = st_get_ticks();
            Menu::set_current(main_menu);
            /* Consume so we do not re-trigger every frame. */
            title_last_event.type = SDL_NOEVENT;
            }
          else if (process_load_game_menu())
            {
              // FIXME: shouldn't be needed if GameSession doesn't relay on global variables
              // reset tux
              scroll_x = 0;
              //titletux.level_begin();
              update_time = st_get_ticks();
            }
        }
      else if(menu == contrib_menu)
        {
          check_contrib_menu();
        }
      else if (menu == contrib_subset_menu)
        {
          check_contrib_subset_menu();
        }
    }

  mouse_cursor->draw();
  touch_controls_draw();

  flipscreen();

  /* Set the time of the last update and the time of the current update */
  last_update_time = update_time;
  update_time = st_get_ticks();

  /* Pause: */
  frame++;
  SDL_Delay(25);
  return Menu::current() != 0;
}

void title(void)
{
  title_init();
  while (title_frame())
    { /* busy loop — title_frame() does one iteration */ }
  title_shutdown();
}

// EOF //

