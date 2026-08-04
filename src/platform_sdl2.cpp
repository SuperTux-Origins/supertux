// SPDX-FileCopyrightText: 2003-2006 SuperTux Development Team
// SPDX-FileCopyrightText: 2024-2026 SuperTux Milestone 1 port contributors
// SPDX-License-Identifier: GPL-3.0-or-later

// SDL 2 platform backend

#include <stdio.h>
#include <stdlib.h>

#include "platform.h"
#include "globals.h"
#include "defines.h"
#include "touch_controls.h"
#include "texture.h"
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

/* Letterbox used by software present / GL viewport — keep in sync so
   touch/mouse window coords can be mapped back to logical ST_SCREEN. */
static int st_lb_ox = 0;
static int st_lb_oy = 0;
static int st_lb_dw = ST_SCREEN_W;
static int st_lb_dh = ST_SCREEN_H;
static float st_lb_scale = 1.0f;
/* Fractions of window reserved for touch chrome (outside the game fit). */
static float st_margin_l = 0.0f;
static float st_margin_r = 0.0f;
static float st_margin_t = 0.0f;
static float st_margin_b = 0.0f;
static int st_overlay_active = 0;

#define VLOG(...) st_vlog(__VA_ARGS__)

static void
st_update_letterbox(int window_w, int window_h)
{
  if (window_w <= 0 || window_h <= 0)
    {
      st_lb_ox = 0;
      st_lb_oy = 0;
      st_lb_dw = ST_SCREEN_W;
      st_lb_dh = ST_SCREEN_H;
      st_lb_scale = 1.0f;
      return;
    }

  int ml = (int)(st_margin_l * (float)window_w + 0.5f);
  int mr = (int)(st_margin_r * (float)window_w + 0.5f);
  int mt = (int)(st_margin_t * (float)window_h + 0.5f);
  int mb = (int)(st_margin_b * (float)window_h + 0.5f);
  if (ml < 0) ml = 0;
  if (mr < 0) mr = 0;
  if (mt < 0) mt = 0;
  if (mb < 0) mb = 0;

  int content_w = window_w - ml - mr;
  int content_h = window_h - mt - mb;
  if (content_w < 16) content_w = 16;
  if (content_h < 16) content_h = 16;

  float sx = (float)content_w / (float)ST_SCREEN_W;
  float sy = (float)content_h / (float)ST_SCREEN_H;
  float scale = (sx < sy) ? sx : sy;
  int dw = (int)(ST_SCREEN_W * scale + 0.5f);
  int dh = (int)(ST_SCREEN_H * scale + 0.5f);
  if (dw < 1) dw = 1;
  if (dh < 1) dh = 1;

  st_lb_ox = ml + (content_w - dw) / 2;
  st_lb_oy = mt + (content_h - dh) / 2;
  st_lb_dw = dw;
  st_lb_dh = dh;
  st_lb_scale = scale;
}

void platform_window_to_logical(int* x, int* y)
{
  if (!x || !y)
    return;
  if (st_lb_scale <= 0.0f)
    return;
  *x = (int)(((float)(*x - st_lb_ox) / st_lb_scale) + 0.5f);
  *y = (int)(((float)(*y - st_lb_oy) / st_lb_scale) + 0.5f);
}

Uint32 platform_get_mouse_state(int* x, int* y)
{
  Uint32 buttons = SDL_GetMouseState(x, y);
  platform_window_to_logical(x, y);
  return buttons;
}

void platform_get_window_size(int* w, int* h)
{
  int ww = ST_SCREEN_W, wh = ST_SCREEN_H;
  if (st_window)
    {
#ifndef NOOPENGL
      if (use_gl)
        SDL_GL_GetDrawableSize(st_window, &ww, &wh);
      else
#endif
        SDL_GetWindowSize(st_window, &ww, &wh);
    }
  if (w) *w = ww;
  if (h) *h = wh;
}

void platform_set_content_margins(float left, float right, float top, float bottom)
{
  st_margin_l = left;
  st_margin_r = right;
  st_margin_t = top;
  st_margin_b = bottom;
  if (st_margin_l < 0.0f) st_margin_l = 0.0f;
  if (st_margin_r < 0.0f) st_margin_r = 0.0f;
  if (st_margin_t < 0.0f) st_margin_t = 0.0f;
  if (st_margin_b < 0.0f) st_margin_b = 0.0f;
  /* Recompute on next present / coordinate map. */
  int ww = ST_SCREEN_W, wh = ST_SCREEN_H;
  if (st_window)
    {
#ifdef USE_GLES2
      if (use_gl)
        SDL_GL_GetDrawableSize(st_window, &ww, &wh);
      else
#endif
        SDL_GetWindowSize(st_window, &ww, &wh);
    }
  st_update_letterbox(ww, wh);
}

void platform_get_letterbox(int* ox, int* oy, int* dw, int* dh)
{
  if (ox) *ox = st_lb_ox;
  if (oy) *oy = st_lb_oy;
  if (dw) *dw = st_lb_dw;
  if (dh) *dh = st_lb_dh;
}

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

#ifdef __EMSCRIPTEN__
  /* Browser canvas: never use SDL exclusive fullscreen — it leaves the
     canvas at the wrong size after exit. Page UI uses the Fullscreen API
     and CSS fit modes; SDL stays at a resizable windowed canvas. */
  (void)fullscreen_inout;
  win = SDL_CreateWindow(title,
                         SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                         ST_SCREEN_W, ST_SCREEN_H,
                         base | SDL_WINDOW_RESIZABLE);
  return win;
#else
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
#endif
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

  st_update_letterbox(window_surface->w, window_surface->h);

  if (window_surface->w == ST_SCREEN_W && window_surface->h == ST_SCREEN_H
      && st_margin_l == 0.0f && st_margin_r == 0.0f
      && st_margin_t == 0.0f && st_margin_b == 0.0f)
    {
      SDL_BlitSurface(st_backbuffer, NULL, window_surface, NULL);
    }
  else
    {
      /* Letterbox scale into desktop-fullscreen or HiDPI window. */
      SDL_Rect dst;
      dst.x = st_lb_ox;
      dst.y = st_lb_oy;
      dst.w = st_lb_dw;
      dst.h = st_lb_dh;
      if (dst.w < 1) dst.w = 1;
      if (dst.h < 1) dst.h = 1;
      SDL_FillRect(window_surface, NULL, SDL_MapRGB(window_surface->format, 0, 0, 0));
      SDL_BlitScaled(st_backbuffer, NULL, window_surface, &dst);
    }

  /* Touch overlay must be painted after the scaled blit (FillRect wipes it). */
  if (touch_controls_is_enabled())
    touch_controls_draw();

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

  st_update_letterbox(ww, wh);

  /* st_lb_oy is top-down (SDL); glViewport Y is bottom-up. With asymmetric
     touch margins the two differ — using top-down Y parks the game too low. */
  int gl_oy = wh - st_lb_oy - st_lb_dh;
  if (gl_oy < 0)
    gl_oy = 0;

#ifdef USE_GLES2
  gles2_renderer_set_viewport_rect(st_lb_ox, gl_oy, st_lb_dw, st_lb_dh);
#else
  glViewport(st_lb_ox, gl_oy, st_lb_dw, st_lb_dh);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, ST_SCREEN_W, ST_SCREEN_H, 0, -1.0, 1.0);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
#endif
}
#endif

void platform_overlay_begin(void)
{
  int ww = ST_SCREEN_W, wh = ST_SCREEN_H;
#ifndef NOOPENGL
  if (use_gl && st_window)
    {
      SDL_GL_GetDrawableSize(st_window, &ww, &wh);
#ifdef USE_GLES2
      gles2_renderer_set_overlay(ww, wh);
#else
      glViewport(0, 0, ww, wh);
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glOrtho(0, ww, wh, 0, -1.0, 1.0);
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
#endif
      st_overlay_active = 1;
      return;
    }
#endif
  if (st_window)
    SDL_GetWindowSize(st_window, &ww, &wh);
  st_overlay_active = 1;
  (void)ww;
  (void)wh;
}

void platform_overlay_fillrect(int x, int y, int w, int h,
                               int r, int g, int b, int a)
{
  if (!st_overlay_active)
    return;
#ifndef NOOPENGL
  if (use_gl)
    {
#ifdef USE_GLES2
      gles2_draw_solid_quad((float)x, (float)y, (float)w, (float)h,
                            (unsigned char)r, (unsigned char)g,
                            (unsigned char)b, (unsigned char)a);
#else
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glColor4ub((GLubyte)r, (GLubyte)g, (GLubyte)b, (GLubyte)a);
      glBegin(GL_QUADS);
      glVertex2i(x, y);
      glVertex2i(x + w, y);
      glVertex2i(x + w, y + h);
      glVertex2i(x, y + h);
      glEnd();
      glDisable(GL_BLEND);
#endif
      return;
    }
#endif
  /* Software: paint onto the window surface directly. */
  if (!st_window)
    return;
  SDL_Surface* ws = SDL_GetWindowSurface(st_window);
  if (!ws)
    return;
  SDL_Rect dst;
  dst.x = x; dst.y = y; dst.w = w; dst.h = h;
  /* Approximate alpha by solid fill (software path rarely used on Android). */
  SDL_FillRect(ws, &dst, SDL_MapRGB(ws->format, (Uint8)r, (Uint8)g, (Uint8)b));
  (void)a;
}

void platform_overlay_surface(Surface* surf, int x, int y, int w, int h)
{
  if (!st_overlay_active || !surf || w < 1 || h < 1)
    return;
#ifndef NOOPENGL
  if (use_gl)
    {
      /* GL path: draw_stretched uses the overlay ortho set in begin(). */
      surf->draw_stretched((float)x, (float)y, w, h, 255, NO_UPDATE);
      return;
    }
#endif
  /*
   * Software: SurfaceSDL::draw_stretched blits to the logical `screen`
   * backbuffer (640×480). Overlay coords are window pixels, so that would
   * either clip away or stamp garbage into the playfield. Blit onto the
   * window surface instead (same target as overlay_fillrect).
   */
  if (!st_window || !surf->impl)
    return;
  SDL_Surface* src = surf->impl->get_sdl_surface();
  SDL_Surface* ws = SDL_GetWindowSurface(st_window);
  if (!src || !ws)
    return;
  SDL_Rect dst;
  dst.x = x;
  dst.y = y;
  dst.w = w;
  dst.h = h;
  SDL_BlitScaled(src, NULL, ws, &dst);
}

void platform_overlay_end(void)
{
  if (!st_overlay_active)
    return;
  st_overlay_active = 0;
#ifndef NOOPENGL
  if (use_gl)
    gl_setup_viewport();
#endif
}

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
#ifdef __EMSCRIPTEN__
  /* Shell HTML owns fullscreen / fit; SDL stays windowed on the canvas. */
  use_fullscreen = false;
#endif
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
      /* Overlay (bezel, touch pad) is in window pixel space and must be
         drawn after the letterboxed game viewport, before the swap.
         Callers may also draw it; a second pass is harmless. */
      if (touch_controls_is_enabled())
        touch_controls_draw();
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

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

/* Called from the HTML shell when the canvas CSS/fullscreen size changes
   so SDL's window and the letterbox match the drawable. */
extern "C" EMSCRIPTEN_KEEPALIVE void
st_emscripten_canvas_resize(int w, int h)
{
  if (w < 16)
    w = ST_SCREEN_W;
  if (h < 16)
    h = ST_SCREEN_H;
  if (!st_window)
    return;
  SDL_SetWindowSize(st_window, w, h);
  st_update_letterbox(w, h);
#ifndef NOOPENGL
  if (use_gl)
    gl_setup_viewport();
#endif
  VLOG("[video] emscripten canvas resize %dx%d\n", w, h);
}

/* Restore native logical size (e.g. after leaving browser fullscreen). */
extern "C" EMSCRIPTEN_KEEPALIVE void
st_emscripten_canvas_native(void)
{
  st_emscripten_canvas_resize(ST_SCREEN_W, ST_SCREEN_H);
}
#endif /* __EMSCRIPTEN__ */
