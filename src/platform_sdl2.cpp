// SDL 2 platform backend
//
// Strategy A (see TODO.md): keep a software SDL_Surface framebuffer via
// SDL_GetWindowSurface (or an offline surface) and present with
// SDL_UpdateWindowSurface. OpenGL uses an SDL_GL context on the window.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static void
log_video_env(void)
{
  const char* vd = SDL_GetCurrentVideoDriver();
  fprintf(stderr, "  SDL video driver: %s\n", vd ? vd : "(null)");
  fprintf(stderr, "  DISPLAY=%s\n", getenv("DISPLAY") ? getenv("DISPLAY") : "(unset)");
  fprintf(stderr, "  WAYLAND_DISPLAY=%s\n",
          getenv("WAYLAND_DISPLAY") ? getenv("WAYLAND_DISPLAY") : "(unset)");
  fprintf(stderr, "  SDL_VIDEODRIVER=%s\n",
          getenv("SDL_VIDEODRIVER") ? getenv("SDL_VIDEODRIVER") : "(unset)");
}

static bool
create_software_window(bool fullscreen)
{
  Uint32 window_flags = SDL_WINDOW_SHOWN;
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
      fprintf(stderr, "\nWarning: fullscreen window failed:\n%s\n",
              SDL_GetError());
      use_fullscreen = false;
      st_window = SDL_CreateWindow("SuperTux " VERSION,
                                   SDL_WINDOWPOS_CENTERED,
                                   SDL_WINDOWPOS_CENTERED,
                                   ST_SCREEN_W, ST_SCREEN_H,
                                   SDL_WINDOW_SHOWN);
    }

  if (!st_window)
    {
      fprintf(stderr, "\nError: SDL_CreateWindow failed:\n%s\n\n",
              SDL_GetError());
      log_video_env();
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
/* Try one combination of GL attributes + window flags. */
static bool
try_gl_window(const char* label, int major, int minor, int profile,
              int r, int g, int b, int a, int depth, bool fullscreen)
{
  SDL_GL_ResetAttributes();
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, r);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, g);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, b);
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, a);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, depth);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

  if (major > 0)
    {
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, major);
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minor);
      if (profile != 0)
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, profile);
    }

  Uint32 window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
  if (fullscreen)
    window_flags |= SDL_WINDOW_FULLSCREEN;

  fprintf(stderr,
          "  GL try [%s]: %d.%d profile=0x%x rgba=%d%d%d%d depth=%d flags=0x%x\n",
          label, major, minor, profile, r, g, b, a, depth,
          (unsigned)window_flags);

  SDL_ClearError();
  st_window = SDL_CreateWindow("SuperTux " VERSION,
                               SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED,
                               ST_SCREEN_W, ST_SCREEN_H,
                               window_flags);
  if (!st_window)
    {
      fprintf(stderr, "    CreateWindow failed: %s\n", SDL_GetError());
      return false;
    }

  SDL_ClearError();
  st_gl_context = SDL_GL_CreateContext(st_window);
  if (!st_gl_context)
    {
      fprintf(stderr, "    CreateContext failed: %s\n", SDL_GetError());
      SDL_DestroyWindow(st_window);
      st_window = 0;
      return false;
    }

  fprintf(stderr, "    OK\n");
  return true;
}

static bool
create_opengl_window(bool fullscreen)
{
  fprintf(stderr,
          "  constants: WINDOW_OPENGL=0x%x SHOWN=0x%x FULLSCREEN=0x%x size=%dx%d\n",
          (unsigned)SDL_WINDOW_OPENGL, (unsigned)SDL_WINDOW_SHOWN,
          (unsigned)SDL_WINDOW_FULLSCREEN, ST_SCREEN_W, ST_SCREEN_H);

  /* --- Attempt 0: absolute minimum (no attrs, no LoadLibrary) --- */
  {
    Uint32 flags = SDL_WINDOW_OPENGL;
    if (fullscreen)
      flags |= SDL_WINDOW_FULLSCREEN;
    fprintf(stderr, "  GL try [bare OPENGL]: flags=0x%x\n", (unsigned)flags);
    SDL_ClearError();
    st_window = SDL_CreateWindow("supertux",
                                 SDL_WINDOWPOS_UNDEFINED,
                                 SDL_WINDOWPOS_UNDEFINED,
                                 ST_SCREEN_W, ST_SCREEN_H,
                                 flags);
    fprintf(stderr, "    ptr=%p err=[%s]\n",
            (void*)st_window, SDL_GetError());
    if (st_window)
      {
        SDL_ClearError();
        st_gl_context = SDL_GL_CreateContext(st_window);
        fprintf(stderr, "    context=%p err=[%s]\n",
                (void*)st_gl_context, SDL_GetError());
        if (st_gl_context)
          goto gl_ok;
        SDL_DestroyWindow(st_window);
        st_window = 0;
      }
  }

  /* Ensure GL lib is loaded for subsequent attempts. */
  SDL_ClearError();
  if (SDL_GL_LoadLibrary(NULL) != 0)
    fprintf(stderr, "  SDL_GL_LoadLibrary: %s\n", SDL_GetError());
  else
    fprintf(stderr, "  SDL_GL_LoadLibrary: ok\n");

  /* Ordered from most appropriate for this codebase to more lenient. */
  const struct {
    const char* label;
    int major, minor, profile;
    int r, g, b, a, depth;
  } attempts[] = {
    { "compat 2.1 8888", 2, 1, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY,
      8, 8, 8, 8, 16 },
    { "compat 2.1 5550", 2, 1, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY,
      5, 5, 5, 0, 16 },
    { "default attrs",   0, 0, 0, 8, 8, 8, 8, 16 },
    { "legacy minimal",  0, 0, 0, 5, 5, 5, 0, 0 },
  };

  for (unsigned i = 0; i < sizeof(attempts) / sizeof(attempts[0]); ++i)
    {
      if (try_gl_window(attempts[i].label,
                        attempts[i].major, attempts[i].minor,
                        attempts[i].profile,
                        attempts[i].r, attempts[i].g, attempts[i].b,
                        attempts[i].a, attempts[i].depth,
                        fullscreen))
        goto gl_ok;

      if (fullscreen)
        {
          fprintf(stderr, "    retry windowed…\n");
          if (try_gl_window(attempts[i].label,
                            attempts[i].major, attempts[i].minor,
                            attempts[i].profile,
                            attempts[i].r, attempts[i].g, attempts[i].b,
                            attempts[i].a, attempts[i].depth,
                            false))
            {
              use_fullscreen = false;
              goto gl_ok;
            }
        }
    }

  fprintf(stderr, "  All OpenGL window attempts failed.\n");
  log_video_env();
  return false;

gl_ok:
  SDL_GL_SetSwapInterval(1);

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
      fprintf(stderr, "OpenGL: probing window/context configurations…\n");
      if (create_opengl_window(use_fullscreen))
        return true;

      fprintf(stderr, "Falling back to software renderer.\n\n");
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
