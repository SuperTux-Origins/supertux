// SDL 1.2 platform backend

#include <stdio.h>
#include <stdlib.h>

#include "platform.h"
#include "globals.h"
#include "defines.h"

bool platform_video_init(bool fullscreen, bool opengl)
{
  /* Video mode changes call this again; SDL1 SetVideoMode replaces the surface. */
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

      screen = SDL_SetVideoMode(ST_SCREEN_W, ST_SCREEN_H, 0, flags);
      if (screen == NULL && use_fullscreen)
        {
          fprintf(stderr,
                  "\nWarning: I could not set up fullscreen OpenGL video.\n"
                  "%s\n\n", SDL_GetError());
          use_fullscreen = false;
          screen = SDL_SetVideoMode(ST_SCREEN_W, ST_SCREEN_H, 0, SDL_OPENGL);
        }

      if (screen == NULL)
        {
          fprintf(stderr,
                  "\nError: I could not set up OpenGL video for %dx%d.\n"
                  "%s\n\n", ST_SCREEN_W, ST_SCREEN_H, SDL_GetError());
          return false;
        }

      glDisable(GL_DEPTH_TEST);
      glDisable(GL_CULL_FACE);
      glViewport(0, 0, screen->w, screen->h);
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glOrtho(0, screen->w, screen->h, 0, -1.0, 1.0);
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      glTranslatef(0.0f, 0.0f, 0.0f);
      return true;
    }
#endif

  /* Software / SDL surface path */
  {
    Uint32 flags = SDL_HWSURFACE | SDL_DOUBLEBUF;
    if (use_fullscreen)
      flags = SDL_FULLSCREEN;

    screen = SDL_SetVideoMode(ST_SCREEN_W, ST_SCREEN_H, 0, flags);
    if (screen == NULL && use_fullscreen)
      {
        fprintf(stderr,
                "\nWarning: I could not set up fullscreen video for "
                "%dx%d mode.\n%s\n\n",
                ST_SCREEN_W, ST_SCREEN_H, SDL_GetError());
        use_fullscreen = false;
        screen = SDL_SetVideoMode(ST_SCREEN_W, ST_SCREEN_H, 0,
                                  SDL_HWSURFACE | SDL_DOUBLEBUF);
      }

    if (screen == NULL)
      {
        fprintf(stderr,
                "\nError: I could not set up video for %dx%d mode.\n"
                "%s\n\n", ST_SCREEN_W, ST_SCREEN_H, SDL_GetError());
        return false;
      }
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
  /* SDL_QuitSubSystem handled by SDL_Quit in st_shutdown */
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
