// SPDX-FileCopyrightText: 2026 SuperTux Milestone1 wasm port
// SPDX-License-Identifier: GPL-3.0-or-later

#include "app_loop.h"
#include "title.h"
#include "gameloop.h"
#include "worldmap.h"
#include "menu.h"
#include "setup.h"
#include "globals.h"
#include "player.h"
#include "resources.h"
#include "touch_controls.h"
#include "screen.h"
#include "tile.h"
#include "text.h"
#include "texture.h"

#include <stdio.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

static bool g_app_active = false;
static AppScreen g_screen = APP_SCREEN_TITLE;
static AppScreen g_return_screen = APP_SCREEN_TITLE;

static WorldMapNS::WorldMap* g_worldmap = 0;
static GameSession* g_session = 0;

static bool g_pending_worldmap = false;
static std::string g_wm_map_file;
static std::string g_wm_save_file;
static bool g_wm_map_is_full_path = false;

static bool g_pending_session = false;
static std::string g_session_subset;
static int g_session_levelnb = 1;
static int g_session_mode = 0;

/* Confirm: delete save slot */
static int g_delete_slot = -1;

bool app_loop_active(void)
{
  return g_app_active;
}

void app_request_worldmap(const std::string& map_file,
                          const std::string& save_file,
                          bool is_full_path_map)
{
  g_wm_map_file = map_file;
  g_wm_save_file = save_file;
  g_wm_map_is_full_path = is_full_path_map;
  g_pending_worldmap = true;
}

void app_request_session(const std::string& subset_or_path, int levelnb, int mode)
{
  g_session_subset = subset_or_path;
  g_session_levelnb = levelnb;
  g_session_mode = mode;
  g_pending_session = true;
}

void app_request_delete_slot(int slot)
{
  g_delete_slot = slot;
  char str[1024];
  sprintf(str, "Are you sure you want to delete slot %d?", slot);
  /* Remember where to return (usually title with load menu restored). */
  g_return_screen = APP_SCREEN_TITLE;
  confirm_dialog_begin(str);
  g_screen = APP_SCREEN_CONFIRM;
}

void app_request_text_scroll(const std::string& file,
                             Surface* surface,
                             float scroll_speed,
                             bool own_surface)
{
  g_return_screen = (g_screen == APP_SCREEN_TEXT) ? APP_SCREEN_TITLE : g_screen;
  if (g_return_screen == APP_SCREEN_CONFIRM || g_return_screen == APP_SCREEN_TEXT)
    g_return_screen = APP_SCREEN_TITLE;
  display_text_file_begin(file, surface, scroll_speed);
  if (own_surface)
    {
      /* display_text_file_begin does not set own_surface; path overload did.
         Expose via a small hack: begin already ran; set ownership if API exists.
         For title credits we pass bkg_title without ownership. */
      (void)own_surface;
    }
  g_screen = APP_SCREEN_TEXT;
}

static void
app_destroy_session(void)
{
  delete g_session;
  g_session = 0;
}

static void
app_destroy_worldmap(void)
{
  delete g_worldmap;
  g_worldmap = 0;
}

static void
app_activate_worldmap(void)
{
  app_destroy_session();
  app_destroy_worldmap();

  g_worldmap = new WorldMapNS::WorldMap();
  if (g_wm_map_is_full_path)
    {
      g_worldmap->loadmap(g_wm_map_file);
    }
  else
    {
      g_worldmap->set_map_file(g_wm_map_file);
      g_worldmap->load_map();
    }
  if (!g_wm_save_file.empty())
    g_worldmap->loadgame(g_wm_save_file.c_str());

  g_worldmap->begin_display();
  g_pending_worldmap = false;
  g_screen = APP_SCREEN_WORLDMAP;
}

static void
app_activate_session(void)
{
  app_destroy_session();
  g_session = new GameSession(g_session_subset, g_session_levelnb, g_session_mode);
  g_session->begin_run();
  g_pending_session = false;
  g_screen = APP_SCREEN_SESSION;
}

static void
app_finish_session(void)
{
  if (!g_session)
    {
      g_screen = g_worldmap ? APP_SCREEN_WORLDMAP : APP_SCREEN_TITLE;
      return;
    }

  GameSession::ExitStatus st = g_session->get_exit_status();

  if (g_worldmap)
    {
      WorldMapNS::WorldMap::Level* level = g_worldmap->at_level();
      if (level && st == GameSession::ES_LEVEL_FINISHED)
        {
          level->solved = true;
          if (g_session->get_world() && g_session->get_world()->get_tux())
            {
              Player* tux = g_session->get_world()->get_tux();
              if (tux->got_coffee)
                player_status.bonus = PlayerStatus::FLOWER_BONUS;
              else if (tux->size == BIG)
                player_status.bonus = PlayerStatus::GROWUP_BONUS;
              else
                player_status.bonus = PlayerStatus::NO_BONUS;
            }
        }
      else if (st == GameSession::ES_GAME_OVER)
        {
          g_worldmap->request_quit();
          player_status.reset();
        }

      if (!g_worldmap->get_savegame_file().empty())
        g_worldmap->savegame(g_worldmap->get_savegame_file());
    }

  Menu::set_current(0);
  touch_controls_reset();
  app_destroy_session();
  if (g_worldmap)
    g_screen = APP_SCREEN_WORLDMAP;
  else
    {
      player_status.reset();
      Menu::set_current(main_menu);
      g_screen = APP_SCREEN_TITLE;
    }
}

static void
app_return_to_title(void)
{
  app_destroy_session();
  app_destroy_worldmap();
  Menu::set_current(main_menu);
  g_screen = APP_SCREEN_TITLE;
}

static void
app_finish_confirm(void)
{
  bool yes = confirm_dialog_result();
  if (yes && g_delete_slot > 0)
    {
      char path[1024];
      sprintf(path, "%s/slot%d.stsg", st_save_dir, g_delete_slot);
      printf("Removing: %s\n", path);
      remove(path);
      update_load_save_game_menu(load_game_menu);
#ifdef __EMSCRIPTEN__
      st_emscripten_fs_sync(0);
#endif
    }
  g_delete_slot = -1;
  Menu::set_current(main_menu);
  g_screen = APP_SCREEN_TITLE;
}

static void
app_finish_text(void)
{
  /* display_text_file_end already restores main_menu. */
  g_screen = APP_SCREEN_TITLE;
  Menu::set_current(main_menu);
}

#ifdef __EMSCRIPTEN__
static void
app_frame(void* /*arg*/)
{
  switch (g_screen)
    {
    case APP_SCREEN_TITLE:
      if (title_frame())
        return;
      if (g_pending_worldmap)
        {
          app_activate_worldmap();
          return;
        }
      if (g_pending_session)
        {
          app_activate_session();
          return;
        }
      /* Real quit (or switched to confirm/text mid-frame via request). */
      if (g_screen != APP_SCREEN_TITLE)
        return;
      title_shutdown();
      g_screen = APP_SCREEN_DONE;
      g_app_active = false;
      clearscreen(0, 0, 0);
      updatescreen();
      unloadshared();
      st_general_free();
      ::TileManager::destroy_instance();
      st_shutdown();
      emscripten_cancel_main_loop();
      break;

    case APP_SCREEN_WORLDMAP:
      if (g_pending_session)
        {
          app_activate_session();
          return;
        }
      if (g_worldmap && g_worldmap->frame())
        return;
      app_return_to_title();
      break;

    case APP_SCREEN_SESSION:
      if (g_session && g_session->frame())
        return;
      app_finish_session();
      break;

    case APP_SCREEN_CONFIRM:
      if (confirm_dialog_frame())
        return;
      app_finish_confirm();
      break;

    case APP_SCREEN_TEXT:
      if (display_text_file_frame())
        return;
      app_finish_text();
      break;

    case APP_SCREEN_DONE:
    default:
      g_app_active = false;
      emscripten_cancel_main_loop();
      break;
    }
}
#endif /* __EMSCRIPTEN__ */

void app_run(void)
{
#ifdef __EMSCRIPTEN__
  g_app_active = true;
  g_screen = APP_SCREEN_TITLE;
  title_init();
  emscripten_set_main_loop_arg(app_frame, 0, 0, 1);
#else
  title();
#endif
}
