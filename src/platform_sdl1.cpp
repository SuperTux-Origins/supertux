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

/* After a failed OpenGL attempt, GL attribute state can make even
 * non-OPENGL SetVideoMode look for a GLX visual. Drop and re-init video. */
static bool
reinit_video_subsystem(void)
{
  SDL_QuitSubSystem(SDL_INIT_VIDEO);
  if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
    {
      fprintf(stderr,
              "\nError: video re-init failed:\n%s\n\n", SDL_GetError());
      return false;
    }
  return true;
}

static SDL_Surface*
try_software_modes(bool fullscreen)
{
  const Uint32 fs = fullscreen ? SDL_FULLSCREEN : (Uint32)0;
  const int depths[] = { 32, 24, 16, 0 };
  /* SWSURFACE first — avoids HWSURFACE/DOUBLEBUF paths that mis-report GLX errors. */
  const Uint32 base_flags[] = {
    (Uint32)(SDL_SWSURFACE | fs),
    (Uint32)(SDL_SWSURFACE | SDL_ANYFORMAT | fs),
    (Uint32)(SDL_HWSURFACE | fs),
    (Uint32)(SDL_HWSURFACE | SDL_DOUBLEBUF | fs),
    (Uint32)(SDL_ANYFORMAT | fs),
    fs
  };

  for (unsigned d = 0; d < sizeof(depths) / sizeof(depths[0]); ++d)
    for (unsigned m = 0; m < sizeof(base_flags) / sizeof(base_flags[0]); ++m)
      {
        SDL_Surface* s = try_set_video_mode(depths[d], base_flags[m]);
        if (s)
          return s;
      }
  return NULL;
}

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

      if (screen != NULL)
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

      fprintf(stderr,
              "\nWarning: OpenGL video failed (%s).\n"
              "Falling back to software renderer.\n\n",
              SDL_GetError());
      use_gl = false;

      /* Critical: clear GL attribute state left by the failed attempt. */
      if (!reinit_video_subsystem())
        return false;
    }
#endif

  /* Software / SDL surface path (also the OpenGL fallback). */
  screen = try_software_modes(use_fullscreen);
  if (screen == NULL && use_fullscreen)
    {
      fprintf(stderr,
              "\nWarning: fullscreen software mode failed, trying windowed.\n");
      use_fullscreen = false;
      screen = try_software_modes(false);
    }

  if (screen == NULL)
    {
      /* One more hard reset in case GL attrs were set elsewhere. */
      if (reinit_video_subsystem())
        screen = try_software_modes(false);
    }

  if (screen == NULL)
    {
      char driver[64] = "?";
      SDL_VideoDriverName(driver, sizeof(driver));
      const char* disp = getenv("DISPLAY");
      fprintf(stderr,
              "\nError: I could not set up video for %dx%d mode.\n"
              "%s\n"
              "Hint: SDL 1.2 needs working X11 (or XWayland).\n"
              "      Try --sdl and ensure DISPLAY is set.\n"
              "      SDL video driver: %s\n"
              "      DISPLAY=%s\n\n",
              ST_SCREEN_W, ST_SCREEN_H, SDL_GetError(),
              driver, disp ? disp : "(unset)");
      return false;
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
