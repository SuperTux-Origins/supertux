// SDL version selection and compatibility helpers.
// Under USE_SDL2, SDL 1.2-shaped APIs used by this codebase are shimmed here
// so gameplay files stay free of scattered version ifdefs.

#ifndef SUPERTUX_PLATFORM_CONFIG_H
#define SUPERTUX_PLATFORM_CONFIG_H

#ifdef USE_SDL2

#include <SDL.h>
#include <SDL_image.h>
#ifndef NOSOUND
#include <SDL_mixer.h>
#endif
#ifndef NOOPENGL
#include <SDL_opengl.h>
#endif

/* --- Types renamed in SDL2 --- */
typedef SDL_Keycode SDLKey;
typedef SDL_Keymod  SDLMod;

/* --- Surface flags removed or unused in SDL2 --- */
#ifndef SDL_HWSURFACE
#define SDL_HWSURFACE   0
#endif
#ifndef SDL_SWSURFACE
#define SDL_SWSURFACE   0
#endif
#ifndef SDL_ANYFORMAT
#define SDL_ANYFORMAT   0
#endif
#ifndef SDL_HWPALETTE
#define SDL_HWPALETTE   0
#endif
#ifndef SDL_DOUBLEBUF
#define SDL_DOUBLEBUF   0
#endif

/* Alpha / colour-key flag bits (SDL1 values still tested by this tree). */
#ifndef SDL_SRCALPHA
#define SDL_SRCALPHA    0x00010000
#endif
#ifndef SDL_SRCCOLORKEY
#define SDL_SRCCOLORKEY 0x00002000
#endif
#ifndef SDL_RLEACCELOK
#define SDL_RLEACCELOK  0
#endif
#ifndef SDL_RLEACCEL
#define SDL_RLEACCEL    0
#endif

/* Keyboard state */
inline const Uint8* ST_GetKeyState(int* numkeys)
{
  return SDL_GetKeyboardState(numkeys);
}
/* Existing call sites use SDL_GetKeyState */
inline const Uint8* SDL_GetKeyState(int* numkeys)
{
  return SDL_GetKeyboardState(numkeys);
}

/* SDL2 keyboard state is indexed by scancode, not keycode. */
inline bool st_key_held(const Uint8* keystate, SDL_Keycode key)
{
  if (!keystate)
    return false;
  return keystate[SDL_GetScancodeFromKey(key)] != 0;
}

/* Key-repeat / UNICODE: approximate no-ops under SDL2. */
#ifndef SDL_DEFAULT_REPEAT_DELAY
#define SDL_DEFAULT_REPEAT_DELAY 500
#endif
#ifndef SDL_DEFAULT_REPEAT_INTERVAL
#define SDL_DEFAULT_REPEAT_INTERVAL 30
#endif
inline void SDL_EnableKeyRepeat(int, int) {}
inline int  SDL_EnableUNICODE(int) { return 1; }

/* DisplayFormat* removed in SDL2 — convert to a convenient 32-bit format. */
inline SDL_Surface* SDL_DisplayFormat(SDL_Surface* src)
{
  if (!src)
    return 0;
  SDL_Surface* conv = SDL_ConvertSurfaceFormat(src, SDL_PIXELFORMAT_RGB888, 0);
  if (!conv)
    conv = SDL_ConvertSurfaceFormat(src, SDL_PIXELFORMAT_RGBA8888, 0);
  return conv;
}

inline SDL_Surface* SDL_DisplayFormatAlpha(SDL_Surface* src)
{
  if (!src)
    return 0;
  return SDL_ConvertSurfaceFormat(src, SDL_PIXELFORMAT_RGBA8888, 0);
}

/* SDL_SetAlpha removed — map onto alpha mod + blend mode. */
inline int SDL_SetAlpha(SDL_Surface* surface, Uint32 flag, Uint8 alpha)
{
  if (!surface)
    return -1;
  if (flag & SDL_SRCALPHA)
    {
      SDL_SetSurfaceAlphaMod(surface, alpha);
      SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND);
    }
  else
    {
      SDL_SetSurfaceAlphaMod(surface, 255);
      SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);
    }
  return 0;
}

/* Colour key: call sites pass SDL_SRCCOLORKEY; SDL2 wants SDL_TRUE. */
inline int st_set_color_key(SDL_Surface* surface, Uint32 flag, Uint32 key)
{
  if (!surface)
    return -1;
  if (flag & SDL_SRCCOLORKEY || flag == SDL_TRUE)
    return SDL_SetColorKey(surface, SDL_TRUE, key);
  return SDL_SetColorKey(surface, SDL_FALSE, 0);
}

/* Icon — implemented in platform backend */
void platform_set_icon(SDL_Surface* icon);
#define SDL_WM_SetIcon(icon, mask) platform_set_icon(icon)

/* Partial screen update — implemented in platform backend */
void platform_update_rect(int x, int y, int w, int h);
inline void SDL_UpdateRect(SDL_Surface* /*surf*/, Sint32 x, Sint32 y, Sint32 w, Sint32 h)
{
  platform_update_rect((int)x, (int)y, (int)w, (int)h);
}

inline void SDL_Flip(SDL_Surface* /*screen*/)
{
  extern void platform_present(bool full_update);
  platform_present(true);
}

inline void SDL_GL_SwapBuffers(void)
{
  extern void platform_present(bool full_update);
  platform_present(true);
}

#else /* ---------------- SDL 1.2 ---------------- */

#include <SDL.h>
#include <SDL_image.h>
#ifndef NOSOUND
#include <SDL_mixer.h>
#endif
#ifndef NOOPENGL
#include <SDL_opengl.h>
#endif

inline Uint8* ST_GetKeyState(int* numkeys)
{
  return SDL_GetKeyState(numkeys);
}

inline bool st_key_held(const Uint8* keystate, SDLKey key)
{
  if (!keystate)
    return false;
  return keystate[(unsigned)key] != 0;
}

inline int st_set_color_key(SDL_Surface* surface, Uint32 flag, Uint32 key)
{
  return SDL_SetColorKey(surface, flag, key);
}

#endif /* USE_SDL2 */



inline Uint8 st_surface_alpha(SDL_Surface* surface)
{
  if (!surface)
    return 255;
#ifdef USE_SDL2
  {
    Uint8 a = 255;
    SDL_GetSurfaceAlphaMod(surface, &a);
    return a;
  }
#else
  return surface->format->alpha;
#endif
}

/* --- Event helpers (same call sites for SDL1 and SDL2) --- */

inline char st_key_ascii(const SDL_Event& event)
{
  if (event.type != SDL_KEYDOWN && event.type != SDL_KEYUP)
    return 0;
#ifdef USE_SDL2
  {
    SDL_Keycode k = event.key.keysym.sym;
    /* Prefer shifted symbol when available via mod state for letters is complex;
       SDL2 text input is better long-term; approximate with keycode. */
    if (k >= SDLK_a && k <= SDLK_z)
      return (char)k;
    if (k >= SDLK_0 && k <= SDLK_9)
      return (char)k;
    if (k == SDLK_SPACE)
      return ' ';
    if (k >= SDLK_SPACE && k <= SDLK_AT)
      return (char)k;
    return 0;
  }
#else
  if ((event.key.keysym.unicode & 0xFF80) == 0)
    return (char)(event.key.keysym.unicode & 0x7F);
  return 0;
#endif
}

inline bool st_event_wheel_up(const SDL_Event& event)
{
#ifdef USE_SDL2
  return event.type == SDL_MOUSEWHEEL && event.wheel.y > 0;
#else
  return event.type == SDL_MOUSEBUTTONUP && event.button.button == 4;
#endif
}

inline bool st_event_wheel_down(const SDL_Event& event)
{
#ifdef USE_SDL2
  return event.type == SDL_MOUSEWHEEL && event.wheel.y < 0;
#else
  return event.type == SDL_MOUSEBUTTONUP && event.button.button == 5;
#endif
}

inline void st_event_mouse_xy(const SDL_Event& event, int* x, int* y)
{
#ifdef USE_SDL2
  if (event.type == SDL_MOUSEWHEEL)
    {
      SDL_GetMouseState(x, y);
      return;
    }
#endif
  if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP)
    {
      *x = event.button.x;
      *y = event.button.y;
      return;
    }
  if (event.type == SDL_MOUSEMOTION)
    {
      *x = event.motion.x;
      *y = event.motion.y;
      return;
    }
  SDL_GetMouseState(x, y);
}


#endif /* SUPERTUX_PLATFORM_CONFIG_H */
