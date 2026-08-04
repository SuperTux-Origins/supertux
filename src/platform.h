// SPDX-FileCopyrightText: 2003-2006 SuperTux Development Team
// SPDX-FileCopyrightText: 2024-2026 SuperTux Milestone 1 port contributors
// SPDX-License-Identifier: GPL-3.0-or-later

// Platform abstraction for SuperTux Milestone 1.
// Isolates SDL 1.2 vs SDL 2 video/window differences so gameplay code
// does not grow a forest of version ifdefs.
//
// Selected at CMake time via ENABLE_SDL2 → compile definition USE_SDL2.

#ifndef SUPERTUX_PLATFORM_H
#define SUPERTUX_PLATFORM_H

#include "platform_config.h"

#ifndef RES320X240
#define ST_SCREEN_W 640
#define ST_SCREEN_H 480
#else
#define ST_SCREEN_W 320
#define ST_SCREEN_H 240
#endif

/** Initialize the video subsystem and create the display.
 *  Sets the global `screen` surface on success.
 *  @param fullscreen request fullscreen
 *  @param opengl     request OpenGL context (may fall back)
 *  @return true on success
 */
bool platform_video_init(bool fullscreen, bool opengl);

/** Apply window title (and optional icon name on SDL1). */
void platform_set_caption(const char* title, const char* icon);

/** Present the backbuffer (Flip / SwapBuffers / UpdateWindowSurface). */
void platform_present(bool full_update);
/** Re-apply MIN/MAG on the offscreen frame texture (smooth-graphics option). */
void platform_apply_frame_filter(void);

/** Partial update (software path); no-op or full present on some backends. */
void platform_update_rect(int x, int y, int w, int h);

/** Shut down video (window/context). Does not call full SDL_Quit. */
void platform_video_shutdown(void);

/** Backend name for diagnostics ("SDL1" or "SDL2"). */
const char* platform_name(void);

/** Window icon (SDL1: WM icon; SDL2: SDL_SetWindowIcon). */
void platform_set_icon(SDL_Surface* icon);

/** Map window/client coordinates (touch, mouse events) into logical
 *  ST_SCREEN_W×ST_SCREEN_H space used by gameplay and the software
 *  backbuffer. No-op when the window matches logical size (SDL1, or
 *  SDL2 without letterbox). Safe to call with NULL. */
void platform_window_to_logical(int* x, int* y);

/** SDL_GetMouseState then map into logical coordinates. */
Uint32 platform_get_mouse_state(int* x, int* y);

/** Current window size in pixels (for finger→logical mapping). */
void platform_get_window_size(int* w, int* h);

/**
 * Reserve fractions of the window (0–1) as margins around the game
 * letterbox so touch controls can sit outside the playfield.
 * L/R/T/B are left, right, top, bottom. Zero = full-window fit (old behaviour).
 */
void platform_set_content_margins(float left, float right, float top, float bottom);

/** Current letterbox rect in window/drawable pixels (game content). */
void platform_get_letterbox(int* ox, int* oy, int* dw, int* dh);

/**
 * Overlay pass in window pixel coordinates (full drawable).
 * Use between game drawing and present to paint touch controls in margins.
 */
void platform_overlay_begin(void);
void platform_overlay_fillrect(int x, int y, int w, int h,
                               int r, int g, int b, int a);
/** Stretched blit of a Surface in window coordinates (active overlay pass). */
void platform_overlay_surface(class Surface* surf, int x, int y, int w, int h);
void platform_overlay_end(void);

#endif /* SUPERTUX_PLATFORM_H */
