// SDL 2 platform backend

#include <stdio.h>
#include <stdlib.h>

#include "platform.h"
#include "globals.h"
#include "defines.h"
#include <SDL_image.h>
#ifndef NOOPENGL
#include "gl_compat.h"
#ifdef USE_GLES2
#include "gles2_renderer.h"
#endif
#endif

static SDL_Window* st_window = NULL;
static SDL_GLContext st_gl_context = NULL;
/* Owned software backbuffer — window surface from SDL_GetWindowSurface is
   invalidated on resize/format changes; the engine treats `screen` as stable. */
static SDL_Surface* st_backbuffer = NULL;

#define VLOG(...) st_vlog(__VA_ARGS__)

static SDL_Surface*
create_software_backbuffer(SDL_Surface* window_surface)
{
  if (!window_surface || !window_surface->format)
    return NULL;

  SDL_PixelFormat* fmt = window_surface->format;
  SDL_Surface* bb = SDL_CreateRGBSurface(
      0, ST_SCREEN_W, ST_SCREEN_H, fmt->BitsPerPixel,
      fmt->Rmask, fmt->Gmask, fmt->Bmask, fmt->Amask);
  if (!bb)
    {
      /* Fall back to a fixed 32-bit format if the window format is awkward. */
      bb = SDL_CreateRGBSurface(0, ST_SCREEN_W, ST_SCREEN_H, 32,
                                0x00FF0000, 0x0000FF00, 0x000000FF, 0);
    }
  return bb;
}

/** Try exclusive fullscreen, then desktop fullscreen, then windowed. */
static SDL_Window*
create_game_window(const char* title, bool opengl, bool* fullscreen_inout)
{
  Uint32 base = opengl ? SDL_WINDOW_OPENGL : 0;
  SDL_Window* win = NULL;

  if (*fullscreen_inout)
    {
      win = SDL_CreateWindow(title,
                             SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                             ST_SCREEN_W, ST_SCREEN_H,
                             base | SDL_WINDOW_FULLSCREEN);
      if (!win)
        {
          VLOG("[video] FULLSCREEN failed (%s), trying FULLSCREEN_DESKTOP\n",
               SDL_GetError());
          win = SDL_CreateWindow(title,
                                 SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                 ST_SCREEN_W, ST_SCREEN_H,
                                 base | SDL_WINDOW_FULLSCREEN_DESKTOP);
        }
      if (!win)
        {
          fprintf(stderr, "Warning: fullscreen failed (%s), using windowed\n",
                  SDL_GetError());
          *fullscreen_inout = false;
        }
    }

  if (!win)
    {
      win = SDL_CreateWindow(title,
                             SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                             ST_SCREEN_W, ST_SCREEN_H,
                             base);
    }

  return win;
}

static void
software_present(void)
{
  if (!st_window || !st_backbuffer)
    return;

  SDL_Surface* window_surface = SDL_GetWindowSurface(st_window);
  if (!window_surface)
    {
      /* Surface can be lost after mode changes; try once to recover. */
      VLOG("[video] GetWindowSurface failed (%s), recreating backbuffer path\n",
           SDL_GetError());
      return;
    }

  if (window_surface->w == ST_SCREEN_W && window_surface->h == ST_SCREEN_H)
    {
      SDL_BlitSurface(st_backbuffer, NULL, window_surface, NULL);
    }
  else
    {
      /* Letterbox scale into desktop-fullscreen or HiDPI window. */
      float sx = (float)window_surface->w / (float)ST_SCREEN_W;
      float sy = (float)window_surface->h / (float)ST_SCREEN_H;
      float scale = (sx < sy) ? sx : sy;
      int dw = (int)(ST_SCREEN_W * scale + 0.5f);
      int dh = (int)(ST_SCREEN_H * scale + 0.5f);
      if (dw < 1) dw = 1;
      if (dh < 1) dh = 1;
      SDL_Rect dst;
      dst.x = (window_surface->w - dw) / 2;
      dst.y = (window_surface->h - dh) / 2;
      dst.w = dw;
      dst.h = dh;
      SDL_FillRect(window_surface, NULL, SDL_MapRGB(window_surface->format, 0, 0, 0));
      SDL_BlitScaled(st_backbuffer, NULL, window_surface, &dst);
    }

  SDL_UpdateWindowSurface(st_window);
}

#ifndef NOOPENGL
static void
gl_setup_viewport(void)
{
  int ww = ST_SCREEN_W;
  int wh = ST_SCREEN_H;
  if (st_window)
    SDL_GL_GetDrawableSize(st_window, &ww, &wh);

#ifdef USE_GLES2
  gles2_renderer_set_viewport(ww, wh);
#else
  float sx = (float)ww / (float)ST_SCREEN_W;
  float sy = (float)wh / (float)ST_SCREEN_H;
  float scale = (sx < sy) ? sx : sy;
  int dw = (int)(ST_SCREEN_W * scale + 0.5f);
  int dh = (int)(ST_SCREEN_H * scale + 0.5f);
  if (dw < 1) dw = 1;
  if (dh < 1) dh = 1;

  glViewport((ww - dw) / 2, (wh - dh) / 2, dw, dh);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, ST_SCREEN_W, ST_SCREEN_H, 0, -1.0, 1.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
#endif
}
#endif

static void
log_window(const char* where)
{
  if (!verbose_mode)
    return;

  const char* path = "software";
  if (use_gl)
    {
#ifdef USE_GLES2
      path = "OpenGL ES 2.0";
#else
      path = "OpenGL (desktop)";
#endif
    }

  st_vlog("[video] %s — render path: %s\n", where, path);
  st_vlog("[video]   window=%p glctx=%p screen=%p fullscreen=%s\n",
          (void*)st_window, (void*)st_gl_context, (void*)screen,
          use_fullscreen ? "yes" : "no");
  if (st_window)
    {
      int x = 0, y = 0, w = 0, h = 0;
      int dw = 0, dh = 0;
      Uint32 flags = SDL_GetWindowFlags(st_window);
      SDL_GetWindowPosition(st_window, &x, &y);
      SDL_GetWindowSize(st_window, &w, &h);
      SDL_GL_GetDrawableSize(st_window, &dw, &dh);
      st_vlog("[video]   id=%u flags=0x%x pos=%d,%d size=%dx%d",
              (unsigned)SDL_GetWindowID(st_window), (unsigned)flags,
              x, y, w, h);
      if (dw > 0 && dh > 0 && (dw != w || dh != h))
        st_vlog(" drawable=%dx%d", dw, dh);
      st_vlog(" title=\"%s\"\n", SDL_GetWindowTitle(st_window));
    }
  st_vlog("[video]   driver=%s\n",
          SDL_GetCurrentVideoDriver() ? SDL_GetCurrentVideoDriver() : "(none)");
#ifndef NOOPENGL
  if (use_gl && st_gl_context)
    {
      const char* gl_ver = (const char*)glGetString(GL_VERSION);
      const char* gl_ren = (const char*)glGetString(GL_RENDERER);
      st_vlog("[video]   GL_VERSION=%s\n", gl_ver ? gl_ver : "(null)");
      st_vlog("[video]   GL_RENDERER=%s\n", gl_ren ? gl_ren : "(null)");
    }
#endif
}

bool platform_video_init(bool fullscreen, bool opengl)
{
  VLOG("[video] platform_video_init(fullscreen=%d, opengl=%d)\n",
       (int)fullscreen, (int)opengl);

  platform_video_shutdown();

#ifdef USE_GLES2
  /* X11's default GLX path often cannot create a real ES context; force EGL
     so PROFILE_ES is honored (Mesa). Harmless on Wayland/Android. */
#ifdef SDL_HINT_VIDEO_X11_FORCE_EGL
  SDL_SetHint(SDL_HINT_VIDEO_X11_FORCE_EGL, "1");
#endif
#ifdef SDL_HINT_OPENGL_ES_DRIVER
  /* Prefer a native GLES library when SDL has a choice. */
  SDL_SetHint(SDL_HINT_OPENGL_ES_DRIVER, "1");
#endif
#endif

  if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
    {
      fprintf(stderr, "Error: SDL_InitSubSystem(VIDEO): %s\n", SDL_GetError());
      SDL_Log("platform_video_init failed: %s", SDL_GetError());
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
      SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
      SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
      SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
      SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
#ifdef USE_GLES2
      /* OpenGL ES 2.0 — required on Android; optional on desktop Linux. */
#ifdef SDL_GL_CONTEXT_PROFILE_MASK
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#endif
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
      /* Desktop compatibility context for immediate-mode GL (glBegin, etc.). */
#ifdef SDL_GL_CONTEXT_PROFILE_MASK
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                          SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
#endif
#endif

      VLOG("[video] CreateWindow OPENGL%s fullscreen=%d %dx%d\n",
#ifdef USE_GLES2
           " ES2",
#else
           "",
#endif
           (int)use_fullscreen, ST_SCREEN_W, ST_SCREEN_H);
      SDL_ClearError();
      st_window = create_game_window("SuperTux " VERSION, true, &use_fullscreen);
      VLOG("[video] CreateWindow -> %p err=[%s]\n",
           (void*)st_window, SDL_GetError());

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
                  SDL_Log("platform_video_init failed: %s", SDL_GetError());
                  return false;
                }
              glDisable(GL_DEPTH_TEST);
              glDisable(GL_CULL_FACE);
#ifdef USE_GLES2
              if (!gles2_renderer_init())
                {
                  fprintf(stderr, "Warning: GLES2 renderer init failed\n");
                  SDL_GL_DeleteContext(st_gl_context);
                  st_gl_context = NULL;
                  SDL_FreeSurface(screen);
                  screen = NULL;
                  SDL_DestroyWindow(st_window);
                  st_window = NULL;
                }
              else
#endif
                {
                  gl_setup_viewport();
                  /* Black frame immediately so the window is not garbage. */
                  glClearColor(0.f, 0.f, 0.f, 1.f);
                  glClear(GL_COLOR_BUFFER_BIT);
                  SDL_GL_SwapWindow(st_window);
#ifdef USE_GLES2
                  log_window("GLES2 ready");
#else
                  log_window("GL ready");
#endif
                  return true;
                }
            }
          if (st_window)
            {
              fprintf(stderr, "Warning: SDL_GL_CreateContext failed: %s\n",
                      SDL_GetError());
              SDL_DestroyWindow(st_window);
              st_window = NULL;
            }
        }
      else
        {
          fprintf(stderr, "Warning: SDL_CreateWindow(OPENGL) failed: %s\n",
                  SDL_GetError());
        }

#ifdef USE_GLES2
      fprintf(stderr, "OpenGL ES 2 unavailable — falling back to software.\n");
#else
      fprintf(stderr, "OpenGL unavailable — falling back to software.\n");
#endif
      use_gl = false;
    }
#endif

  {
    VLOG("[video] CreateWindow software fullscreen=%d %dx%d\n",
         (int)use_fullscreen, ST_SCREEN_W, ST_SCREEN_H);
    st_window = create_game_window("SuperTux " VERSION, false, &use_fullscreen);
    VLOG("[video] CreateWindow -> %p err=[%s]\n",
         (void*)st_window, SDL_GetError());

    if (!st_window)
      {
        fprintf(stderr, "Error: SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Log("platform_video_init failed: %s", SDL_GetError());
        return false;
      }

    {
      SDL_Surface* window_surface = SDL_GetWindowSurface(st_window);
      if (!window_surface)
        {
          fprintf(stderr, "Error: SDL_GetWindowSurface failed: %s\n", SDL_GetError());
          SDL_Log("platform_video_init failed: %s", SDL_GetError());
          return false;
        }

      if (st_backbuffer)
        {
          SDL_FreeSurface(st_backbuffer);
          st_backbuffer = NULL;
        }
      st_backbuffer = create_software_backbuffer(window_surface);
      if (!st_backbuffer)
        {
          fprintf(stderr, "Error: software backbuffer: %s\n", SDL_GetError());
          SDL_Log("platform_video_init failed: %s", SDL_GetError());
          return false;
        }
      screen = st_backbuffer;
      SDL_FillRect(st_backbuffer, NULL,
                   SDL_MapRGB(st_backbuffer->format, 0, 0, 0));
      software_present();
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
      /* Re-apply viewport in case of desktop-fullscreen / drawable size change. */
      gl_setup_viewport();
      SDL_GL_SwapWindow(st_window);
      return;
    }
#endif
  software_present();
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
#ifdef USE_GLES2
  gles2_renderer_shutdown();
#endif
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
  if (st_backbuffer)
    {
      /* screen aliases st_backbuffer in software mode */
      if (screen == st_backbuffer)
        screen = NULL;
      SDL_FreeSurface(st_backbuffer);
      st_backbuffer = NULL;
    }
  if (st_window)
    {
      SDL_DestroyWindow(st_window);
      st_window = NULL;
    }
  screen = NULL;
  /* Do not IMG_Quit here — video may re-init while surfaces still exist. */
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
