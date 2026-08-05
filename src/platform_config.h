// SPDX-FileCopyrightText: 2003-2006 SuperTux Development Team
// SPDX-FileCopyrightText: 2024-2026 SuperTux Milestone 1 port contributors
// SPDX-License-Identifier: GPL-3.0-or-later

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

/**
 * Escape or Android Back keycode.
 *
 * Do NOT gate on #ifdef SDLK_AC_BACK: in SDL2 that name is an anonymous
 * enum member, not a #define, so the preprocessor never sees it and the
 * AC_BACK branch was compiled out — Back reached Menu::event (sym=
 * 1073742094) but matched no case and did nothing.
 */
/**
 * Escape / menu keys. Esc and Android Back are always active.
 * Optional Menu binding is PlayerKeymap::menu (default Tab); implemented
 * in player.cpp so it can read the live keymap.
 */
bool st_is_escape_key(SDL_Keycode key);
bool st_is_escape_event(const SDL_Event& event);

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

bool st_is_escape_key(SDLKey key);
bool st_is_escape_event(const SDL_Event& event);

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
    /* Prefer SDL_TEXTINPUT in menus; this is a fallback for keycode mapping. */
    SDL_Keycode k = event.key.keysym.sym;
    SDL_Keymod mod = SDL_GetModState();
    bool shift = (mod & (KMOD_LSHIFT | KMOD_RSHIFT)) != 0;
    bool caps  = (mod & KMOD_CAPS) != 0;

    if (k >= SDLK_a && k <= SDLK_z)
      {
        char c = (char)k;
        if (shift ^ caps)
          c = (char)(c - 'a' + 'A');
        return c;
      }
    if (k >= SDLK_0 && k <= SDLK_9)
      {
        if (!shift)
          return (char)k;
        /* US layout shifted digits — good enough for slot names. */
        static const char shifted[] = { ')', '!', '@', '#', '$', '%', '^', '&', '*', '(' };
        return shifted[k - SDLK_0];
      }
    if (k == SDLK_SPACE)
      return ' ';
    if (k == SDLK_MINUS)
      return shift ? '_' : '-';
    if (k == SDLK_EQUALS)
      return shift ? '+' : '=';
    if (k == SDLK_PERIOD)
      return shift ? '>' : '.';
    if (k == SDLK_COMMA)
      return shift ? '<' : ',';
    if (k == SDLK_SLASH)
      return shift ? '?' : '/';
    if (k == SDLK_SEMICOLON)
      return shift ? ':' : ';';
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

/** True if event is SDL2 text input with a printable first byte. */
inline bool st_event_text_input(const SDL_Event& event, char* out_char)
{
#ifdef USE_SDL2
  if (event.type != SDL_TEXTINPUT)
    return false;
  if (!event.text.text[0])
    return false;
  /* Take first UTF-8 byte only if it is ASCII printable. */
  unsigned char c = (unsigned char)event.text.text[0];
  if (c < 32 || c >= 127)
    return false;
  if (out_char)
    *out_char = (char)c;
  return true;
#else
  (void)event;
  (void)out_char;
  return false;
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

/* Implemented in platform_sdl1.cpp / platform_sdl2.cpp (letterbox-aware). */
void platform_window_to_logical(int* x, int* y);
Uint32 platform_get_mouse_state(int* x, int* y);

inline void st_event_mouse_xy(const SDL_Event& event, int* x, int* y)
{
#ifdef USE_SDL2
  if (event.type == SDL_MOUSEWHEEL)
    {
      platform_get_mouse_state(x, y);
      return;
    }
#endif
  if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP)
    {
      *x = event.button.x;
      *y = event.button.y;
      platform_window_to_logical(x, y);
      return;
    }
  if (event.type == SDL_MOUSEMOTION)
    {
      *x = event.motion.x;
      *y = event.motion.y;
      platform_window_to_logical(x, y);
      return;
    }
  platform_get_mouse_state(x, y);
}


#endif /* SUPERTUX_PLATFORM_CONFIG_H */
