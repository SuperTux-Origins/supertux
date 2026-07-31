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

#endif /* SUPERTUX_PLATFORM_H */
