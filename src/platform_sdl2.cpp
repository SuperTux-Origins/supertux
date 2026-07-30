// SDL 2 platform backend
//
// Software: SDL_GetWindowSurface + SDL_UpdateWindowSurface.
// OpenGL: same pattern as a minimal SDL2 GL app — CreateWindow(OPENGL),
// then CreateContext. Avoid setting profile attributes before CreateWindow;
// they caused "Invalid window" on some NixOS/Mesa setups.

#include <stdio.h>
#include <stdlib.h>

#include "platform.h"
#include "globals.h"
#include "defines.h"
#include <SDL_image.h>

static SDL_Window* st_window = 0;
static SDL_GLContext st_gl_context = 0;

static void
init_image_subsystem(void)
{
  int img_flags = IMG_INIT_PNG | IMG_INIT_JPG;
  if ((IMG_Init(img_flags) & img_flags) != img_flags)
    fprintf(stderr, "Warning: IMG_Init incomplete: %s\n", IMG_GetError());
}

static bool
create_software_window(bool fullscreen)
{
  Uint32 flags = 0;
  if (fullscreen)
    flags |= SDL_WINDOW_FULLSCREEN;

  st_window = SDL_CreateWindow("SuperTux " VERSION,
                               SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED,
                               ST_SCREEN_W, ST_SCREEN_H,
                               flags);
  if (!st_window && fullscreen)
    {
      fprintf(stderr, "Warning: fullscreen failed (%s), trying windowed\n",
              SDL_GetError());
      use_fullscreen = false;
      st_window = SDL_CreateWindow("SuperTux " VERSION,
                                   SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED,
                                   ST_SCREEN_W, ST_SCREEN_H,
                                   0);
    }

  if (!st_window)
    {
      fprintf(stderr, "Error: SDL_CreateWindow failed: %s\n", SDL_GetError());
      return false;
    }

  screen = SDL_GetWindowSurface(st_window);
  if (!screen)
    {
      fprintf(stderr, "Error: SDL_GetWindowSurface failed: %s\n", SDL_GetError());
      return false;
    }
  return true;
}

#ifndef NOOPENGL
static bool
create_opengl_window(bool fullscreen)
{
  /* Match the known-working minimal test: no GL attributes before
     CreateWindow. Attributes before window creation have returned
     "Invalid window" on some systems where bare OPENGL works. */
  Uint32 flags = SDL_WINDOW_OPENGL;
  if (fullscreen)
    flags |= SDL_WINDOW_FULLSCREEN;

  st_window = SDL_CreateWindow("SuperTux " VERSION,
                               SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED,
                               ST_SCREEN_W, ST_SCREEN_H,
                               flags);
  if (!st_window && fullscreen)
    {
      fprintf(stderr, "Warning: fullscreen GL failed (%s), trying windowed\n",
              SDL_GetError());
      use_fullscreen = false;
      st_window = SDL_CreateWindow("SuperTux " VERSION,
                                   SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED,
                                   ST_SCREEN_W, ST_SCREEN_H,
                                   SDL_WINDOW_OPENGL);
    }

  if (!st_window)
    {
      fprintf(stderr, "Error: SDL_CreateWindow(OPENGL) failed: %s\n",
              SDL_GetError());
      return false;
    }

  st_gl_context = SDL_GL_CreateContext(st_window);
  if (!st_gl_context)
    {
      fprintf(stderr, "Error: SDL_GL_CreateContext failed: %s\n",
              SDL_GetError());
      SDL_DestroyWindow(st_window);
      st_window = 0;
      return false;
    }

  SDL_GL_SetSwapInterval(1);

  /* Shadow surface so legacy code can read screen->w / screen->h. */
  screen = SDL_CreateRGBSurface(0, ST_SCREEN_W, ST_SCREEN_H, 32,
                                0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
  if (!screen)
    {
      fprintf(stderr, "Error: GL shadow surface: %s\n", SDL_GetError());
      return false;
    }

  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  glViewport(0, 0, ST_SCREEN_W, ST_SCREEN_H);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, ST_SCREEN_W, ST_SCREEN_H, 0, -1.0, 1.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  return true;
}
#endif

bool platform_video_init(bool fullscreen, bool opengl)
{
  platform_video_shutdown();

  if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
    {
      fprintf(stderr, "Error: SDL_InitSubSystem(VIDEO): %s\n", SDL_GetError());
      return false;
    }

  init_image_subsystem();

  use_fullscreen = fullscreen;
  use_gl = opengl;

#ifndef NOOPENGL
  if (use_gl)
    {
      if (create_opengl_window(use_fullscreen))
        return true;

      fprintf(stderr, "OpenGL unavailable — falling back to software.\n");
      use_gl = false;
      platform_video_shutdown();
      if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
        {
          fprintf(stderr, "Error: video re-init failed: %s\n", SDL_GetError());
          return false;
        }
      init_image_subsystem();
    }
#endif

  return create_software_window(use_fullscreen);
}

void platform_set_caption(const char* title, const char* /*icon*/)
{
  if (st_window)
    SDL_SetWindowTitle(st_window, title);
}

void platform_present(bool /*full_update*/)
{
#ifndef NOOPENGL
  if (use_gl && st_window)
    {
      SDL_GL_SwapWindow(st_window);
      return;
    }
#endif
  if (st_window)
    SDL_UpdateWindowSurface(st_window);
}

void platform_update_rect(int /*x*/, int /*y*/, int /*w*/, int /*h*/)
{
  platform_present(true);
}

void platform_video_shutdown(void)
{
#ifndef NOOPENGL
  if (st_gl_context)
    {
      SDL_GL_DeleteContext(st_gl_context);
      st_gl_context = 0;
    }
  if (screen && use_gl)
    {
      SDL_FreeSurface(screen);
      screen = 0;
    }
#endif
  if (st_window)
    {
      SDL_DestroyWindow(st_window);
      st_window = 0;
    }
  screen = 0;
}

const char* platform_name(void)
{
  return "SDL2";
}

void platform_set_icon(SDL_Surface* icon)
{
  if (st_window && icon)
    SDL_SetWindowIcon(st_window, icon);
}
