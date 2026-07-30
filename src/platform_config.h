// SDL version selection and shared includes for the platform layer.
// Prefer including this (or platform.h) over raw <SDL.h> in new code.

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

/* Keyboard state: SDL2 uses Uint8* SDL_GetKeyboardState(int*) */
#define ST_GetKeyState(numkeys) SDL_GetKeyboardState(numkeys)

#else /* SDL 1.2 */

#include <SDL.h>
#include <SDL_image.h>
#ifndef NOSOUND
#include <SDL_mixer.h>
#endif
#ifndef NOOPENGL
#include <SDL_opengl.h>
#endif

#define ST_GetKeyState(numkeys) SDL_GetKeyState(numkeys)

#endif /* USE_SDL2 */

#endif /* SUPERTUX_PLATFORM_CONFIG_H */
