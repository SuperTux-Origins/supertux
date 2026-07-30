// SDL 2 platform backend
//
// Strategy A (see TODO.md): keep a software SDL_Surface framebuffer via
// SDL_GetWindowSurface (or an offline surface) and present with
// SDL_UpdateWindowSurface. OpenGL uses an SDL_GL context on the window.

#include <stdio.h>
#include <stdlib.h>

#include "platform.h"
#include "globals.h"
#include "defines.h"

static SDL_Window* st_window = 0;
static SDL_GLContext st_gl_context = 0;

bool platform_video_init(bool fullscreen, bool opengl)
{
  if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
    {
      fprintf(stderr,
              "\nError: I could not initialize video!\n"
              "The Simple DirectMedia error that occured was:\n"
              "%s\n\n", SDL_GetError());
      return false;
    }

  use_fullscreen = fullscreen;
  use_gl = opengl;

  Uint32 window_flags = 0;
  if (use_fullscreen)
    window_flags |= SDL_WINDOW_FULLSCREEN;

#ifndef NOOPENGL
  if (use_gl)
    {
      SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
      SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
      SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
      SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
      SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
      window_flags |= SDL_WINDOW_OPENGL;

      st_window = SDL_CreateWindow("SuperTux " VERSION,
                                   SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED,
                                   ST_SCREEN_W, ST_SCREEN_H,
                                   window_flags);
      if (!st_window && use_fullscreen)
        {
          fprintf(stderr, "\nWarning: fullscreen OpenGL window failed:\n%s\n",
                  SDL_GetError());
          use_fullscreen = false;
          window_flags &= ~SDL_WINDOW_FULLSCREEN;
          st_window = SDL_CreateWindow("SuperTux " VERSION,
                                       SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOWPOS_CENTERED,
                                       ST_SCREEN_W, ST_SCREEN_H,
                                       window_flags);
        }

      if (!st_window)
        {
          fprintf(stderr, "\nError: SDL_CreateWindow (OpenGL) failed:\n%s\n",
                  SDL_GetError());
          return false;
        }

      st_gl_context = SDL_GL_CreateContext(st_window);
      if (!st_gl_context)
        {
          fprintf(stderr, "\nError: SDL_GL_CreateContext failed:\n%s\n",
                  SDL_GetError());
          return false;
        }

      /* Placeholder surface for code that still reads screen->w/h */
      screen = SDL_CreateRGBSurface(0, ST_SCREEN_W, ST_SCREEN_H, 32,
                                    0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
      if (!screen)
        {
          fprintf(stderr, "\nError: could not create GL shadow surface:\n%s\n",
                  SDL_GetError());
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

  st_window = SDL_CreateWindow("SuperTux " VERSION,
                               SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED,
                               ST_SCREEN_W, ST_SCREEN_H,
                               window_flags);
  if (!st_window && use_fullscreen)
    {
      fprintf(stderr, "\nWarning: fullscreen window failed:\n%s\n",
              SDL_GetError());
      use_fullscreen = false;
      window_flags &= ~SDL_WINDOW_FULLSCREEN;
      st_window = SDL_CreateWindow("SuperTux " VERSION,
                                   SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED,
                                   ST_SCREEN_W, ST_SCREEN_H,
                                   window_flags);
    }

  if (!st_window)
    {
      fprintf(stderr, "\nError: SDL_CreateWindow failed:\n%s\n",
              SDL_GetError());
      return false;
    }

  screen = SDL_GetWindowSurface(st_window);
  if (!screen)
    {
      fprintf(stderr, "\nError: SDL_GetWindowSurface failed:\n%s\n",
              SDL_GetError());
      return false;
    }

  return true;
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
  /* Window-surface path: partial updates are not always available; present. */
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
  if (use_gl && screen)
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
