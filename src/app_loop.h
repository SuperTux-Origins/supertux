// SPDX-FileCopyrightText: 2026 SuperTux Milestone1 wasm port
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SUPERTUX_APP_LOOP_H
#define SUPERTUX_APP_LOOP_H

#include <string>

class Surface;

/**
 * Top-level screen ownership for the Emscripten frame pump.
 * Native builds keep the historic nested busy loops; on Emscripten
 * app_run() drives title / worldmap / session / overlays via frame()
 * helpers without nesting run()/display()/confirm/credits.
 */

enum AppScreen {
  APP_SCREEN_TITLE = 0,
  APP_SCREEN_WORLDMAP,
  APP_SCREEN_SESSION,
  APP_SCREEN_CONFIRM,
  APP_SCREEN_TEXT,
  APP_SCREEN_DONE
};

/** True when the Emscripten non-blocking app loop is active. */
bool app_loop_active(void);

/**
 * Request a worldmap after the current title frame ends.
 * map_file: e.g. "world1.stwm" or a path accepted by loadmap/set_map_file.
 * save_file: full path to .stsg (may not exist yet).
 */
void app_request_worldmap(const std::string& map_file,
                          const std::string& save_file,
                          bool is_full_path_map = false);

/**
 * Request a level session (Emscripten only).
 * Matches GameSession(subset_or_path, levelnb, mode):
 *   ST_GL_LOAD_LEVEL_FILE — subset_or_path is a full level path; levelnb ignored
 *   ST_GL_PLAY            — subset name + 1-based level number
 */
void app_request_session(const std::string& subset_or_path, int levelnb, int mode);

/** Yes/No confirm to delete a save slot (title load menu). */
void app_request_delete_slot(int slot);

/**
 * Scrollable text overlay (credits / intro).
 * surface is not owned unless own_surface is true.
 */
void app_request_text_scroll(const std::string& file,
                             Surface* surface,
                             float scroll_speed,
                             bool own_surface = false);

/**
 * Run the whole game under a single frame callback (Emscripten).
 * Does not return until the user quits (cancels the main loop).
 */
void app_run(void);

/**
 * Request pause (open in-game / worldmap pause menu). Used by the web
 * shell Pause button and by Page Visibility when the tab is hidden.
 * Harmless on native; no-op if already in a menu or on the title screen.
 */
void app_request_pause(void);

/** True once after app_request_pause until consumed by session/worldmap. */
bool app_consume_pause_request(void);

#endif /* SUPERTUX_APP_LOOP_H */
