// SDL 2 platform backend

#include <stdio.h>
#include <stdlib.h>

#include "platform.h"
#include "globals.h"
#include "defines.h"
#include <SDL_image.h>

static SDL_Window* st_window = NULL;
static SDL_GLContext st_gl_context = NULL;

#define VLOG(...) do { if (verbose_mode) fprintf(stderr, __VA_ARGS__); } while (0)

static void
log_window(const char* where)
{
  if (!verbose_mode)
    return;
  fprintf(stderr, "[video] %s: window=%p glctx=%p screen=%p use_gl=%d fullscreen=%d\n",
          where, (void*)st_window, (void*)st_gl_context, (void*)screen,
          (int)use_gl, (int)use_fullscreen);
  if (st_window)
    {
      int x = 0, y = 0, w = 0, h = 0;
      Uint32 flags = SDL_GetWindowFlags(st_window);
      SDL_GetWindowPosition(st_window, &x, &y);
      SDL_GetWindowSize(st_window, &w, &h);
      fprintf(stderr, "[video]   id=%u flags=0x%x pos=%d,%d size=%dx%d title=\"%s\"\n",
              (unsigned)SDL_GetWindowID(st_window), (unsigned)flags,
              x, y, w, h, SDL_GetWindowTitle(st_window));
    }
  fprintf(stderr, "[video]   driver=%s\n",
          SDL_GetCurrentVideoDriver() ? SDL_GetCurrentVideoDriver() : "(none)");
}

bool platform_video_init(bool fullscreen, bool opengl)
{
  VLOG("[video] platform_video_init(fullscreen=%d, opengl=%d)\n",
       (int)fullscreen, (int)opengl);

  platform_video_shutdown();

  if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
    {
      fprintf(stderr, "Error: SDL_InitSubSystem(VIDEO): %s\n", SDL_GetError());
      return false;
    }

  {
    int img_flags = IMG_INIT_PNG | IMG_INIT_JPG;
    if ((IMG_Init(img_flags) & img_flags) != img_flags)
      fprintf(stderr, "Warning: IMG_Init: %s\n", IMG_GetError());
  }

  use_fullscreen = fullscreen;
  use_gl = opengl;

  VLOG("[video] driver=%s DISPLAY=%s\n",
       SDL_GetCurrentVideoDriver() ? SDL_GetCurrentVideoDriver() : "(none)",
       getenv("DISPLAY") ? getenv("DISPLAY") : "(unset)");

#ifndef NOOPENGL
  if (use_gl)
    {
      Uint32 flags = SDL_WINDOW_OPENGL;
      if (use_fullscreen)
        flags |= SDL_WINDOW_FULLSCREEN;

      VLOG("[video] CreateWindow OPENGL flags=0x%x %dx%d\n",
           (unsigned)flags, ST_SCREEN_W, ST_SCREEN_H);
      SDL_ClearError();
      st_window = SDL_CreateWindow("SuperTux " VERSION,
                                   SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED,
                                   ST_SCREEN_W, ST_SCREEN_H,
                                   flags);
      VLOG("[video] CreateWindow -> %p err=[%s]\n",
           (void*)st_window, SDL_GetError());

      if (!st_window && use_fullscreen)
        {
          fprintf(stderr, "Warning: fullscreen OpenGL failed (%s), trying windowed\n",
                  SDL_GetError());
          use_fullscreen = false;
          st_window = SDL_CreateWindow("SuperTux " VERSION,
                                       SDL_WINDOWPOS_CENTERED,
                                       SDL_WINDOWPOS_CENTERED,
                                       ST_SCREEN_W, ST_SCREEN_H,
                                       SDL_WINDOW_OPENGL);
          VLOG("[video] windowed CreateWindow -> %p err=[%s]\n",
               (void*)st_window, SDL_GetError());
        }

      if (st_window)
        {
          SDL_ClearError();
          st_gl_context = SDL_GL_CreateContext(st_window);
          VLOG("[video] CreateContext -> %p err=[%s]\n",
               (void*)st_gl_context, SDL_GetError());
          if (st_gl_context)
            {
              SDL_GL_SetSwapInterval(1);
              screen = SDL_CreateRGBSurface(0, ST_SCREEN_W, ST_SCREEN_H, 32,
                                            0x00FF0000, 0x0000FF00,
                                            0x000000FF, 0xFF000000);
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
              log_window("GL ready");
              return true;
            }
          fprintf(stderr, "Warning: SDL_GL_CreateContext failed: %s\n",
                  SDL_GetError());
          SDL_DestroyWindow(st_window);
          st_window = NULL;
        }
      else
        {
          fprintf(stderr, "Warning: SDL_CreateWindow(OPENGL) failed: %s\n",
                  SDL_GetError());
        }

      fprintf(stderr, "OpenGL unavailable — falling back to software.\n");
      use_gl = false;
    }
#endif

  {
    Uint32 flags = 0;
    if (use_fullscreen)
      flags |= SDL_WINDOW_FULLSCREEN;

    VLOG("[video] CreateWindow software flags=0x%x %dx%d\n",
         (unsigned)flags, ST_SCREEN_W, ST_SCREEN_H);
    st_window = SDL_CreateWindow("SuperTux " VERSION,
                                 SDL_WINDOWPOS_CENTERED,
                                 SDL_WINDOWPOS_CENTERED,
                                 ST_SCREEN_W, ST_SCREEN_H,
                                 flags);
    VLOG("[video] CreateWindow -> %p err=[%s]\n",
         (void*)st_window, SDL_GetError());

    if (!st_window && use_fullscreen)
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
    log_window("software ready");
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
  platform_present(true);
}

void platform_video_shutdown(void)
{
  VLOG("[video] shutdown window=%p glctx=%p\n",
       (void*)st_window, (void*)st_gl_context);
#ifndef NOOPENGL
  if (st_gl_context)
    {
      SDL_GL_DeleteContext(st_gl_context);
      st_gl_context = NULL;
    }
  if (screen && use_gl)
    {
      SDL_FreeSurface(screen);
      screen = NULL;
    }
#endif
  if (st_window)
    {
      SDL_DestroyWindow(st_window);
      st_window = NULL;
    }
  screen = NULL;
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
