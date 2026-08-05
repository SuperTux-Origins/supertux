// SPDX-FileCopyrightText: 2026 SuperTux Milestone1 wasm port
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SUPERTUX_APP_LOOP_H
#define SUPERTUX_APP_LOOP_H

#include <string>

class Surface;

/**
 * Top-level screen ownership for the non-blocking frame pump.
 * app_run() drives title / worldmap / session / overlays via frame()
 * helpers without nesting run()/display()/confirm/credits. Used on
 * Emscripten (emscripten_set_main_loop) and desktop (busy while +
 * st_frame_delay). Nested busy loops remain for leveleditor / CLI.
 */

enum AppScreen {
  APP_SCREEN_TITLE = 0,
  APP_SCREEN_WORLDMAP,
  APP_SCREEN_SESSION,
  APP_SCREEN_CONFIRM,
  APP_SCREEN_TEXT,
  APP_SCREEN_DONE
};

/** True while app_run() owns the top-level frame pump. */
bool app_loop_active(void);

/**
 * Frame-driven full-screen black fade (works under app_loop / WASM).
 * Alpha is 0 = transparent, 255 = solid black. Draw with app_fade_draw()
 * after the current scene, before flipscreen().
 */
int  app_fade_alpha(void);
void app_fade_draw(void);
bool app_fade_active(void);
/** Start fade to black (out) or from black (in). duration_ms ~250–400. */
void app_fade_start_out(int duration_ms = 300);
void app_fade_start_in(int duration_ms = 300);
/** True once the current fade has reached its end alpha. */
bool app_fade_finished(void);
void app_fade_clear(void);

/**
 * Request a worldmap after the current title frame ends.
 * map_file: e.g. "world1.stwm" or a path accepted by loadmap/set_map_file.
 * save_file: full path to .stsg (may not exist yet).
 */
void app_request_worldmap(const std::string& map_file,
                          const std::string& save_file,
                          bool is_full_path_map = false);

/**
 * Request a level session (non-blocking hand-off under app_loop).
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
 * Run the whole game under the unified frame pump.
 * Emscripten: emscripten_set_main_loop (does not return).
 * Desktop: while (app_frame) until quit; returns to main for teardown.
 */
void app_run(void);

#endif /* SUPERTUX_APP_LOOP_H */
