// SPDX-FileCopyrightText: 2026 SuperTux Milestone1 wasm port
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SUPERTUX_APP_LOOP_H
#define SUPERTUX_APP_LOOP_H

#include <string>

/**
 * Top-level screen ownership for the Emscripten frame pump.
 * Native builds keep the historic nested busy loops; on Emscripten
 * app_run() drives title / worldmap / session via frame() helpers
 * without nesting run()/display().
 */

enum AppScreen {
  APP_SCREEN_TITLE = 0,
  APP_SCREEN_WORLDMAP,
  APP_SCREEN_SESSION,
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

/** Request a level session from the active worldmap (Emscripten only). */
void app_request_session(const std::string& level_path, int mode);

/**
 * Run the whole game under a single frame callback (Emscripten).
 * Does not return until the user quits (cancels the main loop).
 */
void app_run(void);

#endif /* SUPERTUX_APP_LOOP_H */
