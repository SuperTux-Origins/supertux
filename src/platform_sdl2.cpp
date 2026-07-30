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
  Uint32 window_flags = 0;
  if (fullscreen)
    window_flags |= SDL_WINDOW_FULLSCREEN;

  st_window = SDL_CreateWindow("SuperTux " VERSION,
                               SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED,
                               ST_SCREEN_W, ST_SCREEN_H,
                               window_flags);
  if (!st_window && fullscreen)
    {
      fprintf(stderr, "\nWarning: fullscreen window failed:\n%s\n",
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
      fprintf(stderr, "\nError: SDL_CreateWindow failed:\n%s\n\n",
              SDL_GetError());
      return false;
    }

  screen = SDL_GetWindowSurface(st_window);
  if (!screen)
    {
      fprintf(stderr, "\nError: SDL_GetWindowSurface failed:\n%s\n\n",
              SDL_GetError());
      return false;
    }

  return true;
}

#ifndef NOOPENGL
static bool
create_opengl_window(bool fullscreen)
{
  /* Immediate-mode GL (glBegin/glOrtho) needs a compatibility context.
     8-bit channels are widely supported; the old 5/5/5 attrs often fail. */
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                      SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);

  Uint32 window_flags = SDL_WINDOW_OPENGL;
  if (fullscreen)
    window_flags |= SDL_WINDOW_FULLSCREEN;

  SDL_ClearError();
  st_window = SDL_CreateWindow("SuperTux " VERSION,
                               SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED,
                               ST_SCREEN_W, ST_SCREEN_H,
                               window_flags);
  if (!st_window && fullscreen)
    {
      fprintf(stderr, "\nWarning: fullscreen OpenGL window failed:\n%s\n",
              SDL_GetError());
      use_fullscreen = false;
      window_flags &= ~SDL_WINDOW_FULLSCREEN;
      SDL_ClearError();
      st_window = SDL_CreateWindow("SuperTux " VERSION,
                                   SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED,
                                   ST_SCREEN_W, ST_SCREEN_H,
                                   window_flags);
    }

  if (!st_window)
    {
      fprintf(stderr,
              "\nWarning: SDL_CreateWindow (OpenGL) failed: %s\n"
              "Falling back to software renderer.\n\n",
              SDL_GetError());
      return false;
    }

  st_gl_context = SDL_GL_CreateContext(st_window);
  if (!st_gl_context)
    {
      fprintf(stderr,
              "\nWarning: SDL_GL_CreateContext failed: %s\n"
              "Falling back to software renderer.\n\n",
              SDL_GetError());
      SDL_DestroyWindow(st_window);
      st_window = 0;
      return false;
    }

  SDL_GL_SetSwapInterval(1);

  /* Placeholder surface so legacy code can read screen->w / screen->h. */
  screen = SDL_CreateRGBSurface(0, ST_SCREEN_W, ST_SCREEN_H, 32,
                                0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
  if (!screen)
    {
      fprintf(stderr, "\nError: could not create GL shadow surface:\n%s\n\n",
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

bool platform_video_init(bool fullscreen, bool opengl)
{
  /* Re-init (e.g. options menu toggles fullscreen/GL): drop previous window. */
  platform_video_shutdown();

  if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
    {
      fprintf(stderr,
              "\nError: I could not initialize video!\n"
              "The Simple DirectMedia error that occured was:\n"
              "%s\n\n", SDL_GetError());
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

      /* OpenGL unavailable — clear GL request state and use software. */
      use_gl = false;
      platform_video_shutdown();
      if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
        {
          fprintf(stderr, "\nError: video re-init failed:\n%s\n\n",
                  SDL_GetError());
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
  /* Software path: window surface is owned by the window; do not free. */
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
