// SPDX-FileCopyrightText: 2026 SuperTux Milestone1 wasm port
// SPDX-License-Identifier: GPL-3.0-or-later

#include "app_loop.h"
#include "title.h"
#include "gameloop.h"
#include "worldmap.h"
#include "menu.h"
#include "setup.h"
#include "defines.h"
#include "globals.h"
#include "player.h"
#include "resources.h"
#include "touch_controls.h"
#include "screen.h"
#include "tile.h"
#include "text.h"
#include "texture.h"
#include "timer.h"

#include <stdio.h>
#include <string.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#ifdef NOSOUND
/* sound.cpp is not linked — keep the shell's audio exports defined. */
extern "C" EMSCRIPTEN_KEEPALIVE void st_emscripten_audio_pause(void) {}
extern "C" EMSCRIPTEN_KEEPALIVE void st_emscripten_audio_resume(void) {}
#endif
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

/* After worldmap extro text, show CREDITS then leave map. */
static bool g_pending_credits_after_extro = false;

/* Frame-driven black fade (replaces busy-loop fade under app_loop). */
enum FadePhase { FADE_IDLE = 0, FADE_OUT, FADE_IN };
static FadePhase g_fade_phase = FADE_IDLE;
static int g_fade_from = 0;
static int g_fade_to = 0;
static unsigned int g_fade_start_ms = 0;
static int g_fade_duration_ms = 300;
/* After session ends: hold last frame under fade-out before teardown. */
static bool g_session_exit_fading = false;

bool app_loop_active(void)
{
  return g_app_active;
}

static void
app_fade_begin(int from_a, int to_a, int duration_ms)
{
  if (from_a < 0) from_a = 0;
  if (from_a > 255) from_a = 255;
  if (to_a < 0) to_a = 0;
  if (to_a > 255) to_a = 255;
  g_fade_from = from_a;
  g_fade_to = to_a;
  g_fade_duration_ms = duration_ms > 0 ? duration_ms : 300;
  g_fade_start_ms = st_get_ticks();
  g_fade_phase = (to_a > from_a) ? FADE_OUT : FADE_IN;
}

void app_fade_start_out(int duration_ms)
{
  app_fade_begin(app_fade_alpha(), 255, duration_ms);
}

void app_fade_start_in(int duration_ms)
{
  app_fade_begin(255, 0, duration_ms);
}

void app_fade_clear(void)
{
  g_fade_phase = FADE_IDLE;
  g_fade_from = g_fade_to = 0;
}

bool app_fade_active(void)
{
  return g_fade_phase != FADE_IDLE;
}

int app_fade_alpha(void)
{
  if (g_fade_phase == FADE_IDLE)
    return 0;
  unsigned int gone = st_get_ticks() - g_fade_start_ms;
  if (gone >= (unsigned int)g_fade_duration_ms)
    return g_fade_to;
  float t = (float)gone / (float)g_fade_duration_ms;
  return (int)(g_fade_from + (g_fade_to - g_fade_from) * t);
}

bool app_fade_finished(void)
{
  if (g_fade_phase == FADE_IDLE)
    return true;
  return (st_get_ticks() - g_fade_start_ms) >= (unsigned int)g_fade_duration_ms;
}

void app_fade_draw(void)
{
  int a = app_fade_alpha();
  if (a <= 0)
    return;
  fillrect(0, 0, (float)screen->w, (float)screen->h, 0, 0, 0, a);
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
  sprintf(str, "Delete Slot %d", slot);
  /* Remember where to return (usually title with load menu restored). */
  g_return_screen = APP_SCREEN_TITLE;
  confirm_dialog_begin("", str, "Cancel");
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
  display_text_file_begin(file, surface, scroll_speed, own_surface);
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
  std::string extro_file;

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
          if (!level->extro_filename.empty())
            extro_file = level->extro_filename;
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

  /* Worldmap final-level story: queue non-blocking text, then credits, then title. */
  if (!extro_file.empty())
    {
      app_destroy_worldmap();
      g_pending_credits_after_extro = true;
      Surface* bg = new Surface(datadir + "/images/background/extro.jpg", IGNORE_ALPHA);
      app_request_text_scroll(extro_file, bg, SCROLL_SPEED_MESSAGE, true);
      return;
    }

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
  if (g_pending_credits_after_extro)
    {
      g_pending_credits_after_extro = false;
      Surface* bg = new Surface(datadir + "/images/background/oiltux.jpg", IGNORE_ALPHA);
      app_request_text_scroll("CREDITS", bg, SCROLL_SPEED_CREDITS, true);
      return;
    }
  /* New-game story intro: continue into the queued worldmap. */
  if (g_pending_worldmap)
    {
      if (g_fade_phase == FADE_IDLE)
        app_fade_start_out(250);
      if (g_fade_phase == FADE_OUT && !app_fade_finished())
        {
          clearscreen(0, 0, 0);
          app_fade_draw();
          flipscreen();
          /* Stay on TEXT; next frame re-enters finish until fade completes. */
          return;
        }
      app_activate_worldmap();
      app_fade_start_in(300);
      return;
    }
  /* display_text_file_end already restores main_menu. */
  g_screen = APP_SCREEN_TITLE;
  Menu::set_current(main_menu);
}

static void
app_frame(void* /*arg*/)
{
  switch (g_screen)
    {
    case APP_SCREEN_TITLE:
      if (title_frame())
        {
          /* Pending map/session: fade to black, then switch. */
          if ((g_pending_worldmap || g_pending_session)
              && g_fade_phase == FADE_IDLE)
            app_fade_start_out(300);
          if (g_fade_phase == FADE_OUT)
            {
              app_fade_draw();
              flipscreen();
              if (app_fade_finished())
                {
                  if (g_pending_worldmap)
                    {
                      app_activate_worldmap();
                      app_fade_start_in(300);
                    }
                  else if (g_pending_session)
                    {
                      app_activate_session();
                      app_fade_start_in(300);
                    }
                }
              return;
            }
          if (g_fade_phase == FADE_IN)
            {
              app_fade_draw();
              flipscreen();
              if (app_fade_finished())
                app_fade_clear();
              return;
            }
          return;
        }
      /* title_frame may have switched to TEXT/CONFIRM (e.g. new-game
         intro, credits, delete-slot). Do not steal into a pending
         worldmap/session until that screen finishes. */
      if (g_screen != APP_SCREEN_TITLE)
        return;
      if (g_pending_worldmap || g_pending_session)
        {
          if (g_fade_phase == FADE_IDLE)
            app_fade_start_out(300);
          if (g_fade_phase == FADE_OUT && !app_fade_finished())
            {
              /* Title already stopped drawing; hold black. */
              clearscreen(0, 0, 0);
              app_fade_draw();
              flipscreen();
              return;
            }
          if (g_pending_worldmap)
            app_activate_worldmap();
          else
            app_activate_session();
          app_fade_start_in(300);
          return;
        }
      /* Real quit. */
      title_shutdown();
      g_screen = APP_SCREEN_DONE;
      g_app_active = false;
#ifdef __EMSCRIPTEN__
      /* Full teardown only on wasm — desktop main() cleans up after app_run. */
      clearscreen(0, 0, 0);
      updatescreen();
      unloadshared();
      st_general_free();
      ::TileManager::destroy_instance();
      st_shutdown();
      emscripten_cancel_main_loop();
#endif
      break;

    case APP_SCREEN_WORLDMAP:
      if (g_pending_session)
        {
          if (g_fade_phase == FADE_IDLE)
            app_fade_start_out(300);
          if (g_fade_phase == FADE_OUT && !app_fade_finished())
            {
              /* Keep last map frame under rising black (map draws + fade). */
              if (g_worldmap)
                g_worldmap->frame();
              else
                {
                  clearscreen(0, 0, 0);
                  app_fade_draw();
                  flipscreen();
                }
              return;
            }
          app_activate_session();
          app_fade_start_in(350);
          return;
        }
      if (g_worldmap && g_worldmap->frame())
        {
          if (g_fade_phase == FADE_IN)
            {
              if (app_fade_finished())
                app_fade_clear();
            }
          return;
        }
      /* Leaving map → title: fade out first. */
      if (g_fade_phase == FADE_IDLE)
        app_fade_start_out(300);
      if (g_fade_phase == FADE_OUT && !app_fade_finished())
        {
          if (g_worldmap)
            g_worldmap->frame();
          return;
        }
      app_return_to_title();
      app_fade_start_in(300);
      break;

    case APP_SCREEN_SESSION:
      if (g_session && g_session->frame())
        {
          if (g_fade_phase == FADE_IN && app_fade_finished())
            app_fade_clear();
          return;
        }
      /* Session requested exit — fade to black before teardown. */
      if (g_session && !g_session_exit_fading)
        {
          g_session_exit_fading = true;
          app_fade_start_out(300);
        }
      if (g_session_exit_fading && g_fade_phase == FADE_OUT
          && !app_fade_finished())
        {
          if (g_session)
            {
              g_session->draw();
              app_fade_draw();
              flipscreen();
            }
          return;
        }
      g_session_exit_fading = false;
      app_finish_session();
      app_fade_start_in(300);
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
#ifdef __EMSCRIPTEN__
      emscripten_cancel_main_loop();
#endif
      break;
    }
}

void app_run(void)
{
  g_app_active = true;
  g_screen = APP_SCREEN_TITLE;
  title_init();
#ifdef __EMSCRIPTEN__
  /* rAF-paced; does not return. */
  emscripten_set_main_loop_arg(app_frame, 0, 0, 1);
#else
  /* Same state machine as wasm; frame helpers pace via st_frame_delay. */
  while (g_app_active && g_screen != APP_SCREEN_DONE)
    app_frame(0);
  g_app_active = false;
#endif
}
