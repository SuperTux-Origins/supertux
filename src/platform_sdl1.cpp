// SDL 1.2 platform backend

#include <stdio.h>
#include <stdlib.h>

#include "platform.h"
#include "globals.h"
#include "defines.h"

#define VLOG(...) do { if (verbose_mode) fprintf(stderr, __VA_ARGS__); } while (0)

bool platform_video_init(bool fullscreen, bool opengl)
{
  VLOG("[video] platform_video_init(fullscreen=%d, opengl=%d) SDL1\n",
       (int)fullscreen, (int)opengl);

  if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
    {
      fprintf(stderr, "Error: SDL_InitSubSystem(VIDEO): %s\n", SDL_GetError());
      return false;
    }

  use_fullscreen = fullscreen;
  use_gl = opengl;

#ifndef NOOPENGL
  if (use_gl)
    {
      SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
      SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
      SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
      SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
      SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

      Uint32 flags = SDL_OPENGL;
      if (use_fullscreen)
        flags |= SDL_FULLSCREEN;

      VLOG("[video] SetVideoMode OPENGL flags=0x%x\n", (unsigned)flags);
      screen = SDL_SetVideoMode(ST_SCREEN_W, ST_SCREEN_H, 0, flags);
      if (!screen && use_fullscreen)
        {
          use_fullscreen = false;
          screen = SDL_SetVideoMode(ST_SCREEN_W, ST_SCREEN_H, 0, SDL_OPENGL);
        }

      if (screen)
        {
          glDisable(GL_DEPTH_TEST);
          glDisable(GL_CULL_FACE);
          glViewport(0, 0, screen->w, screen->h);
          glMatrixMode(GL_PROJECTION);
          glLoadIdentity();
          glOrtho(0, screen->w, screen->h, 0, -1.0, 1.0);
          glMatrixMode(GL_MODELVIEW);
          glLoadIdentity();
          VLOG("[video] OpenGL ready %dx%d\n", screen->w, screen->h);
          return true;
        }

      fprintf(stderr, "Warning: OpenGL video failed (%s), using software\n",
              SDL_GetError());
      use_gl = false;
    }
#endif

  {
    Uint32 flags = SDL_SWSURFACE;
    if (use_fullscreen)
      flags |= SDL_FULLSCREEN;

    VLOG("[video] SetVideoMode software flags=0x%x\n", (unsigned)flags);
    screen = SDL_SetVideoMode(ST_SCREEN_W, ST_SCREEN_H, 0, flags);
    if (!screen && use_fullscreen)
      {
        use_fullscreen = false;
        screen = SDL_SetVideoMode(ST_SCREEN_W, ST_SCREEN_H, 0, SDL_SWSURFACE);
      }
    if (!screen)
      {
        fprintf(stderr, "Error: SDL_SetVideoMode failed: %s\n", SDL_GetError());
        return false;
      }
    VLOG("[video] software ready %dx%d\n", screen->w, screen->h);
  }

  return true;
}

void platform_set_caption(const char* title, const char* icon)
{
  SDL_WM_SetCaption(title, icon);
}

void platform_present(bool full_update)
{
#ifndef NOOPENGL
  if (use_gl)
    {
      SDL_GL_SwapBuffers();
      return;
    }
#endif
  if (full_update)
    SDL_Flip(screen);
  else
    SDL_UpdateRect(screen, 0, 0, screen->w, screen->h);
}

void platform_update_rect(int x, int y, int w, int h)
{
#ifndef NOOPENGL
  if (use_gl)
    return;
#endif
  SDL_UpdateRect(screen, x, y, w, h);
}

void platform_video_shutdown(void)
{
}

const char* platform_name(void)
{
  return "SDL1";
}

void platform_set_icon(SDL_Surface* icon)
{
  if (icon)
    SDL_WM_SetIcon(icon, NULL);
}
