// SDL 1.2 platform backend

#include <stdio.h>
#include <stdlib.h>

#include "platform.h"
#include "globals.h"
#include "defines.h"

static SDL_Surface*
try_set_video_mode(int bpp, Uint32 flags)
{
  SDL_Surface* s = SDL_SetVideoMode(ST_SCREEN_W, ST_SCREEN_H, bpp, flags);
  if (!s)
    fprintf(stderr,
            "  SetVideoMode(%dx%d, bpp=%d, flags=0x%x) failed: %s\n",
            ST_SCREEN_W, ST_SCREEN_H, bpp, (unsigned)flags, SDL_GetError());
  return s;
}

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

      screen = try_set_video_mode(0, flags);
      if (screen == NULL && use_fullscreen)
        {
          fprintf(stderr,
                  "\nWarning: fullscreen OpenGL failed, trying windowed.\n");
          use_fullscreen = false;
          screen = try_set_video_mode(0, SDL_OPENGL);
        }

      if (screen == NULL)
        {
          fprintf(stderr,
                  "\nError: I could not set up OpenGL video for %dx%d.\n"
                  "Falling back to software renderer.\n\n",
                  ST_SCREEN_W, ST_SCREEN_H);
          use_gl = false;
          /* fall through to software path */
        }
      else
        {
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
    }
#endif

  /*
   * Software path. Prefer SWSURFACE: HWSURFACE|DOUBLEBUF often fails on
   * modern X11 with a misleading "Couldn't find matching GLX visual"
   * even when SDL_OPENGL was never requested.
   */
  {
    const Uint32 fs = use_fullscreen ? SDL_FULLSCREEN : 0;
    const int depths[] = { 32, 24, 16, 0 };
    const Uint32 mode_flags[] = {
      (Uint32)(SDL_SWSURFACE | fs),
      (Uint32)(SDL_HWSURFACE | SDL_DOUBLEBUF | fs),
      (Uint32)(SDL_HWSURFACE | fs),
      (Uint32)(SDL_ANYFORMAT | fs),
      (Uint32)fs
    };

    screen = NULL;
    for (unsigned d = 0; d < sizeof(depths) / sizeof(depths[0]) && !screen; ++d)
      for (unsigned m = 0; m < sizeof(mode_flags) / sizeof(mode_flags[0]) && !screen; ++m)
        screen = try_set_video_mode(depths[d], mode_flags[m]);

    if (screen == NULL && use_fullscreen)
      {
        fprintf(stderr,
                "\nWarning: fullscreen software mode failed, trying windowed.\n");
        use_fullscreen = false;
        for (unsigned d = 0; d < sizeof(depths) / sizeof(depths[0]) && !screen; ++d)
          {
            screen = try_set_video_mode(depths[d], SDL_SWSURFACE);
            if (!screen)
              screen = try_set_video_mode(depths[d], SDL_HWSURFACE | SDL_DOUBLEBUF);
          }
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
