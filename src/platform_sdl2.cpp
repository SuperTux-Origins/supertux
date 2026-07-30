// SDL 2 platform backend

#include <stdio.h>
#include <stdlib.h>

#include "platform.h"
#include "globals.h"
#include "defines.h"
#include <SDL_image.h>

static SDL_Window* st_window = 0;
static SDL_GLContext st_gl_context = 0;
static bool video_was_inited = false;

static void
log_windows(const char* where)
{
  fprintf(stderr, "[video] %s: st_window=%p st_gl_context=%p screen=%p "
          "use_gl=%d use_fullscreen=%d video_was_inited=%d\n",
          where, (void*)st_window, (void*)st_gl_context, (void*)screen,
          (int)use_gl, (int)use_fullscreen, (int)video_was_inited);
  if (st_window)
    {
      int x, y, w, h;
      Uint32 flags = SDL_GetWindowFlags(st_window);
      SDL_GetWindowPosition(st_window, &x, &y);
      SDL_GetWindowSize(st_window, &w, &h);
      fprintf(stderr, "[video]   window id=%u flags=0x%x pos=%d,%d size=%dx%d title=\"%s\"\n",
              (unsigned)SDL_GetWindowID(st_window), (unsigned)flags,
              x, y, w, h, SDL_GetWindowTitle(st_window));
    }
  fprintf(stderr, "[video]   SDL_GetError=[%s] driver=%s\n",
          SDL_GetError(),
          SDL_GetCurrentVideoDriver() ? SDL_GetCurrentVideoDriver() : "(none)");
}

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

  fprintf(stderr, "[video] CreateWindow software flags=0x%x size=%dx%d\n",
          (unsigned)flags, ST_SCREEN_W, ST_SCREEN_H);
  SDL_ClearError();
  st_window = SDL_CreateWindow("SuperTux " VERSION,
                               SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED,
                               ST_SCREEN_W, ST_SCREEN_H,
                               flags);
  log_windows("after software CreateWindow");

  if (!st_window && fullscreen)
    {
      fprintf(stderr, "[video] fullscreen software failed, retry windowed\n");
      use_fullscreen = false;
      SDL_ClearError();
      st_window = SDL_CreateWindow("SuperTux " VERSION,
                                   SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED,
                                   ST_SCREEN_W, ST_SCREEN_H,
                                   0);
      log_windows("after software CreateWindow windowed");
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
  fprintf(stderr, "[video] software surface %dx%d format=%u\n",
          screen->w, screen->h, (unsigned)screen->format->format);
  return true;
}

#ifndef NOOPENGL
static bool
create_opengl_window(bool fullscreen)
{
  /* Same call pattern as the known-working /tmp/glwin test. */
  Uint32 flags = SDL_WINDOW_OPENGL;
  if (fullscreen)
    flags |= SDL_WINDOW_FULLSCREEN;

  fprintf(stderr, "[video] CreateWindow OPENGL flags=0x%x size=%dx%d "
          "(SDL_WINDOW_OPENGL=0x%x)\n",
          (unsigned)flags, ST_SCREEN_W, ST_SCREEN_H,
          (unsigned)SDL_WINDOW_OPENGL);
  fprintf(stderr, "[video]   existing windows before create: ");
  log_windows("pre-GL-CreateWindow");

  SDL_ClearError();
  st_window = SDL_CreateWindow("SuperTux " VERSION,
                               SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED,
                               ST_SCREEN_W, ST_SCREEN_H,
                               flags);
  fprintf(stderr, "[video] CreateWindow returned %p, err=[%s]\n",
          (void*)st_window, SDL_GetError());

  if (!st_window && fullscreen)
    {
      fprintf(stderr, "[video] fullscreen GL failed, retry windowed\n");
      use_fullscreen = false;
      SDL_ClearError();
      st_window = SDL_CreateWindow("SuperTux " VERSION,
                                   SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED,
                                   ST_SCREEN_W, ST_SCREEN_H,
                                   SDL_WINDOW_OPENGL);
      fprintf(stderr, "[video] windowed CreateWindow returned %p, err=[%s]\n",
              (void*)st_window, SDL_GetError());
    }

  if (!st_window)
    {
      /* Last resort: identical to the working minimal test. */
      fprintf(stderr, "[video] retry exact minimal test pattern…\n");
      SDL_ClearError();
      st_window = SDL_CreateWindow("t", 0, 0, 640, 480, SDL_WINDOW_OPENGL);
      fprintf(stderr, "[video] minimal CreateWindow returned %p, err=[%s]\n",
              (void*)st_window, SDL_GetError());
    }

  if (!st_window)
    {
      fprintf(stderr, "Error: SDL_CreateWindow(OPENGL) failed: %s\n",
              SDL_GetError());
      return false;
    }

  log_windows("after GL CreateWindow");

  SDL_ClearError();
  st_gl_context = SDL_GL_CreateContext(st_window);
  fprintf(stderr, "[video] CreateContext returned %p, err=[%s]\n",
          (void*)st_gl_context, SDL_GetError());
  if (!st_gl_context)
    {
      fprintf(stderr, "Error: SDL_GL_CreateContext failed: %s\n",
              SDL_GetError());
      SDL_DestroyWindow(st_window);
      st_window = 0;
      return false;
    }

  SDL_GL_SetSwapInterval(1);

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

  log_windows("GL ready");
  return true;
}
#endif

bool platform_video_init(bool fullscreen, bool opengl)
{
  fprintf(stderr, "[video] platform_video_init(fullscreen=%d, opengl=%d)\n",
          (int)fullscreen, (int)opengl);
  log_windows("enter init");

  platform_video_shutdown();
  log_windows("after shutdown");

  /* Use SDL_Init(VIDEO) like the working test, not only InitSubSystem. */
  SDL_ClearError();
  if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
      fprintf(stderr, "Error: SDL_Init(VIDEO): %s\n", SDL_GetError());
      return false;
    }
  video_was_inited = true;
  fprintf(stderr, "[video] SDL_Init(VIDEO) ok, driver=%s\n",
          SDL_GetCurrentVideoDriver() ? SDL_GetCurrentVideoDriver() : "?");

  use_fullscreen = fullscreen;
  use_gl = opengl;

#ifndef NOOPENGL
  if (use_gl)
    {
      if (create_opengl_window(use_fullscreen))
        {
          init_image_subsystem(); /* after window, like a normal app */
          return true;
        }

      fprintf(stderr, "OpenGL unavailable — falling back to software.\n");
      use_gl = false;
      if (st_window)
        {
          fprintf(stderr, "[video] destroying failed GL window %p\n", (void*)st_window);
          SDL_DestroyWindow(st_window);
          st_window = 0;
        }
      st_gl_context = 0;
      screen = 0;
    }
#endif

  if (!create_software_window(use_fullscreen))
    return false;
  init_image_subsystem();
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
  fprintf(stderr, "[video] platform_video_shutdown\n");
  log_windows("shutdown enter");

#ifndef NOOPENGL
  if (st_gl_context)
    {
      fprintf(stderr, "[video] DeleteContext %p\n", (void*)st_gl_context);
      SDL_GL_DeleteContext(st_gl_context);
      st_gl_context = 0;
    }
  if (screen && use_gl)
    {
      fprintf(stderr, "[video] FreeSurface shadow %p\n", (void*)screen);
      SDL_FreeSurface(screen);
      screen = 0;
    }
#endif
  if (st_window)
    {
      fprintf(stderr, "[video] DestroyWindow %p\n", (void*)st_window);
      SDL_DestroyWindow(st_window);
      st_window = 0;
    }
  screen = 0;
  log_windows("shutdown leave");
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
