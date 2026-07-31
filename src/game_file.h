// SPDX-FileCopyrightText: 2003-2006 SuperTux Development Team
// SPDX-FileCopyrightText: 2024-2026 SuperTux Milestone 1 port contributors
// SPDX-License-Identifier: GPL-3.0-or-later

/* Thin game-data I/O: desktop filesystem or Android APK assets via SDL_RWops. */
#ifndef SUPERTUX_GAME_FILE_H
#define SUPERTUX_GAME_FILE_H

#include <string>
#include <vector>

#include "SDL.h"

/** Open a game data file for reading.
 *  Tries the path as-is (mods, absolute installs), then a path relative to
 *  datadir (APK assets on Android via SDL_RWFromFile). Caller must
 *  SDL_RWclose() unless handing ownership to IMG_Load_RW / Mix_*_RW with
 *  freesrc=1. Returns NULL on failure. */
SDL_RWops* open_game_file(const std::string& path);

/** True if open_game_file would succeed. */
bool game_file_exists(const std::string& path);

/** Read entire file into memory. Returns false on failure. */
bool game_file_read(const std::string& path, std::vector<char>& out);

/** Strip datadir (and leading ./) to an assets-relative path. */
std::string game_file_relative(const std::string& path);

#endif
