// On-screen virtual gamepad for touch devices.
// Buttons live in window pixel space (letterbox margins); game stays logical 640×480.

#include "touch_controls.h"
#include "platform.h"
#include "globals.h"
#include "defines.h"
#include "screen.h"
#include "player.h"
#include "text.h"
#include "texture.h"
#include "setup.h"

#include <string.h>
#include <string>

enum {
  TC_LEFT = 0,
  TC_RIGHT,
  TC_UP,
  TC_DOWN,
  TC_JUMP,
  TC_ACTION,
  TC_BOTH,   /* Run+Jump together (carry item while running/jumping) */
  TC_MENU,
  TC_COUNT
};

struct TcButton {
  int x, y, w, h;
  bool held;
  bool prev_held;
#ifdef USE_SDL2
  SDL_FingerID finger;
#else
  int finger;
#endif
  bool has_finger;
};

static bool tc_enabled = false;
static TcButton tc_btn[TC_COUNT];
static bool tc_inited_layout = false;
static int tc_layout_ww = 0;
static int tc_layout_wh = 0;

/* Default content margins when the pad is enabled (fractions of window). */
static const float TC_MARGIN_L = 0.11f;
static const float TC_MARGIN_R = 0.11f;
static const float TC_MARGIN_T = 0.05f;
static const float TC_MARGIN_B = 0.20f;

static void
tc_layout(void)
{
  int ww = ST_SCREEN_W, wh = ST_SCREEN_H;
  platform_get_window_size(&ww, &wh);
  if (ww < 64) ww = ST_SCREEN_W;
  if (wh < 64) wh = ST_SCREEN_H;

  tc_layout_ww = ww;
  tc_layout_wh = wh;

  /*
   * Comfortable cross layout near the corners — not packed into a tight
   * cluster. Sizes scale with the window; spacing stays proportional so
   * the pad feels like the old on-screen layout.
   */
  int bs = wh < ww ? wh / 9 : ww / 10;   /* face-button size */
  if (bs < 48) bs = 48;
  if (bs > 88) bs = 88;
  int gap = bs / 3;                       /* open space between arms */
  int pad = bs / 4;                       /* inset from screen edge */
  if (pad < 12) pad = 12;

  /* D-pad: bottom-left corner, classic + shape with room between keys. */
  int dpad_cx = pad + bs + gap / 2 + bs / 2;
  int dpad_cy = wh - pad - bs - gap / 2 - bs / 2;
  tc_btn[TC_LEFT].x  = dpad_cx - bs - gap;  tc_btn[TC_LEFT].y  = dpad_cy - bs / 2;
  tc_btn[TC_LEFT].w  = bs;                  tc_btn[TC_LEFT].h  = bs;
  tc_btn[TC_RIGHT].x = dpad_cx + gap;       tc_btn[TC_RIGHT].y = dpad_cy - bs / 2;
  tc_btn[TC_RIGHT].w = bs;                  tc_btn[TC_RIGHT].h = bs;
  tc_btn[TC_UP].x    = dpad_cx - bs / 2;    tc_btn[TC_UP].y    = dpad_cy - bs - gap;
  tc_btn[TC_UP].w    = bs;                  tc_btn[TC_UP].h    = bs;
  tc_btn[TC_DOWN].x  = dpad_cx - bs / 2;    tc_btn[TC_DOWN].y  = dpad_cy + gap;
  tc_btn[TC_DOWN].w  = bs;                  tc_btn[TC_DOWN].h  = bs;

  /*
   * Right cluster (bottom-right corner), left → right:
   *   Action (run/shoot) | Both (run+jump) | Jump
   * "Both" is for carrying an item while running and still being able to jump
   * without needing two fingers on the face buttons.
   */
  int jump_s = bs + bs / 5;
  int act_s  = bs;
  int both_s = bs;
  tc_btn[TC_JUMP].x   = ww - pad - jump_s;
  tc_btn[TC_JUMP].y   = wh - pad - jump_s;
  tc_btn[TC_JUMP].w   = jump_s;
  tc_btn[TC_JUMP].h   = jump_s;
  tc_btn[TC_BOTH].x   = ww - pad - jump_s - gap - both_s;
  tc_btn[TC_BOTH].y   = wh - pad - both_s - gap / 2;
  tc_btn[TC_BOTH].w   = both_s;
  tc_btn[TC_BOTH].h   = both_s;
  tc_btn[TC_ACTION].x = ww - pad - jump_s - gap - both_s - gap - act_s;
  tc_btn[TC_ACTION].y = wh - pad - act_s - gap;
  tc_btn[TC_ACTION].w = act_s;
  tc_btn[TC_ACTION].h = act_s;

  /* Menu — top-left corner. */
  int menu_s = bs * 2 / 3;
  if (menu_s < 40) menu_s = 40;
  tc_btn[TC_MENU].x = pad;
  tc_btn[TC_MENU].y = pad;
  tc_btn[TC_MENU].w = menu_s;
  tc_btn[TC_MENU].h = menu_s;

  if (!tc_inited_layout)
    {
      for (int i = 0; i < TC_COUNT; ++i)
        {
          tc_btn[i].held = false;
          tc_btn[i].prev_held = false;
          tc_btn[i].has_finger = false;
          tc_btn[i].finger = 0;
        }
    }
  tc_inited_layout = true;
}

static void
tc_ensure_layout(void)
{
  int ww = ST_SCREEN_W, wh = ST_SCREEN_H;
  platform_get_window_size(&ww, &wh);
  if (!tc_inited_layout || ww != tc_layout_ww || wh != tc_layout_wh)
    tc_layout();
}

void touch_controls_set_enabled(bool enabled)
{
  tc_enabled = enabled;
  if (enabled)
    {
      platform_set_content_margins(TC_MARGIN_L, TC_MARGIN_R,
                                   TC_MARGIN_T, TC_MARGIN_B);
      tc_layout();
    }
  else
    {
      platform_set_content_margins(0.0f, 0.0f, 0.0f, 0.0f);
      touch_controls_reset();
    }
}

bool touch_controls_is_enabled(void)
{
  return tc_enabled;
}

void touch_controls_reset(void)
{
  if (!tc_inited_layout)
    tc_layout();
  for (int i = 0; i < TC_COUNT; ++i)
    {
      tc_btn[i].held = false;
      tc_btn[i].prev_held = false;
      tc_btn[i].has_finger = false;
    }
}

static int
tc_hit(int x, int y)
{
  for (int i = 0; i < TC_COUNT; ++i)
    {
      if (x >= tc_btn[i].x && x < tc_btn[i].x + tc_btn[i].w
          && y >= tc_btn[i].y && y < tc_btn[i].y + tc_btn[i].h)
        return i;
    }
  return -1;
}

/* Finger/window position in window pixels (not logical). */
static void
tc_event_to_window(float fx, float fy, int* wx, int* wy)
{
  int ww = ST_SCREEN_W, wh = ST_SCREEN_H;
  platform_get_window_size(&ww, &wh);
  *wx = (int)(fx * (float)ww + 0.5f);
  *wy = (int)(fy * (float)wh + 0.5f);
}

#ifdef USE_SDL2
typedef SDL_FingerID TcFingerId;
#else
typedef int TcFingerId;
#endif

static Player* tc_player = 0;

void touch_controls_set_player(Player* p)
{
  tc_player = p;
}

static void
tc_press(int id, TcFingerId finger)
{
  if (id < 0 || id >= TC_COUNT)
    return;
  tc_btn[id].held = true;
  tc_btn[id].has_finger = true;
  tc_btn[id].finger = finger;
}

static void
tc_release_button(int i)
{
  if (!tc_btn[i].held)
    return;
  tc_btn[i].held = false;
  tc_btn[i].has_finger = false;
  if (!tc_player)
    return;
  switch (i)
    {
    case TC_LEFT:   tc_player->key_event((SDLKey)keymap.left, UP); break;
    case TC_RIGHT:  tc_player->key_event((SDLKey)keymap.right, UP); break;
    case TC_UP:
      if (!tc_btn[TC_JUMP].held && !tc_btn[TC_BOTH].held)
        tc_player->key_event((SDLKey)keymap.jump, UP);
      break;
    case TC_JUMP:
      if (!tc_btn[TC_UP].held && !tc_btn[TC_BOTH].held)
        tc_player->key_event((SDLKey)keymap.jump, UP);
      break;
    case TC_BOTH:
      if (!tc_btn[TC_UP].held && !tc_btn[TC_JUMP].held)
        tc_player->key_event((SDLKey)keymap.jump, UP);
      if (!tc_btn[TC_ACTION].held)
        tc_player->key_event((SDLKey)keymap.fire, UP);
      break;
    case TC_DOWN:   tc_player->key_event((SDLKey)keymap.duck, UP); break;
    case TC_ACTION:
      if (!tc_btn[TC_BOTH].held)
        tc_player->key_event((SDLKey)keymap.fire, UP);
      break;
    default: break;
    }
}

static void
tc_release_finger(TcFingerId finger)
{
  for (int i = 0; i < TC_COUNT; ++i)
    {
      if (tc_btn[i].has_finger && tc_btn[i].finger == finger)
        tc_release_button(i);
    }
}

static void
tc_move_finger(TcFingerId finger, int x, int y)
{
  int hit = tc_hit(x, y);
  for (int i = 0; i < TC_COUNT; ++i)
    {
      if (tc_btn[i].has_finger && tc_btn[i].finger == finger && i != hit)
        tc_release_button(i);
    }
  if (hit >= 0)
    tc_press(hit, finger);
}

bool touch_controls_event(const SDL_Event& event)
{
  if (!tc_enabled)
    return false;

  tc_ensure_layout();

#ifdef USE_SDL2
  switch (event.type)
    {
    case SDL_FINGERDOWN:
      {
        int wx, wy;
        tc_event_to_window(event.tfinger.x, event.tfinger.y, &wx, &wy);
        int hit = tc_hit(wx, wy);
        if (hit >= 0)
          {
            tc_press(hit, event.tfinger.fingerId);
            return true;
          }
      }
      break;
    case SDL_FINGERUP:
      {
        bool owned = false;
        for (int i = 0; i < TC_COUNT; ++i)
          if (tc_btn[i].has_finger && tc_btn[i].finger == event.tfinger.fingerId)
            owned = true;
        tc_release_finger(event.tfinger.fingerId);
        if (owned)
          return true;
      }
      break;
    case SDL_FINGERMOTION:
      {
        int wx, wy;
        tc_event_to_window(event.tfinger.x, event.tfinger.y, &wx, &wy);
        bool owned = false;
        for (int i = 0; i < TC_COUNT; ++i)
          if (tc_btn[i].has_finger && tc_btn[i].finger == event.tfinger.fingerId)
            owned = true;
        if (owned || tc_hit(wx, wy) >= 0)
          {
            tc_move_finger(event.tfinger.fingerId, wx, wy);
            return true;
          }
      }
      break;
    case SDL_MOUSEBUTTONDOWN:
      if (event.button.button == SDL_BUTTON_LEFT)
        {
          /* Raw window coords — do not map to logical. */
          int x = event.button.x, y = event.button.y;
          int hit = tc_hit(x, y);
          if (hit >= 0)
            {
              tc_press(hit, (TcFingerId)-1);
              return true;
            }
        }
      break;
    case SDL_MOUSEBUTTONUP:
      if (event.button.button == SDL_BUTTON_LEFT)
        tc_release_finger((TcFingerId)-1);
      break;
    case SDL_MOUSEMOTION:
      if (event.motion.state & SDL_BUTTON_LMASK)
        {
          int x = event.motion.x, y = event.motion.y;
          bool owned = false;
          for (int i = 0; i < TC_COUNT; ++i)
            if (tc_btn[i].has_finger && tc_btn[i].finger == (TcFingerId)-1)
              owned = true;
          if (owned)
            {
              tc_move_finger((TcFingerId)-1, x, y);
              return true;
            }
        }
      break;
    default:
      break;
    }
#else
  (void)event;
#endif
  return false;
}

/* tv-bezel.png (1920×1080): transparent 4:3 hole at this rect. */
static const int BEZEL_IW = 1920, BEZEL_IH = 1080;
static const int BEZEL_HX = 320, BEZEL_HY = 60, BEZEL_HW = 1280, BEZEL_HH = 960;

static Surface* tc_bezel = 0;
static bool tc_bezel_tried = false;

static void
tc_draw_bezel(void)
{
  if (!tc_bezel_tried)
    {
      tc_bezel_tried = true;
      std::string path = datadir + "/images/status/tv-bezel.png";
      FILE* fp = fopen(path.c_str(), "rb");
      if (fp)
        {
          fclose(fp);
          tc_bezel = new Surface(path, USE_ALPHA);
          st_vlog("[video] loaded arctic TV bezel (%dx%d)\n",
                  tc_bezel ? tc_bezel->w : 0, tc_bezel ? tc_bezel->h : 0);
        }
      else
        st_vlog("[video] no tv-bezel.png — plain letterbox margins\n");
    }
  if (!tc_bezel)
    return;

  int lx = 0, ly = 0, lw = ST_SCREEN_W, lh = ST_SCREEN_H;
  platform_get_letterbox(&lx, &ly, &lw, &lh);
  /* Map image hole → letterbox; frame scales with it (may stretch slightly). */
  float sx = (float)lw / (float)BEZEL_HW;
  float sy = (float)lh / (float)BEZEL_HH;
  int dw = (int)(BEZEL_IW * sx + 0.5f);
  int dh = (int)(BEZEL_IH * sy + 0.5f);
  int dx = lx - (int)(BEZEL_HX * sx + 0.5f);
  int dy = ly - (int)(BEZEL_HY * sy + 0.5f);
  platform_overlay_surface(tc_bezel, dx, dy, dw, dh);
}

void touch_controls_draw(void)
{
  if (!tc_enabled)
    return;

  tc_ensure_layout();

  int ww = ST_SCREEN_W, wh = ST_SCREEN_H;
  platform_get_window_size(&ww, &wh);

  platform_overlay_begin();
  /* Arctic fill under the bezel so gaps outside the frame aren't pure black. */
  platform_overlay_fillrect(0, 0, ww, wh, 25, 45, 70, 255);
  tc_draw_bezel();

  for (int i = 0; i < TC_COUNT; ++i)
    {
      int alpha = tc_btn[i].held ? 160 : 90;
      int r = 50, g = 50, b = 50;
      if (i == TC_JUMP)   { r = 40;  g = 140; b = 40; }
      if (i == TC_ACTION) { r = 140; g = 40;  b = 40; }
      if (i == TC_BOTH)   { r = 140; g = 120; b = 40; } /* run+jump */
      if (i == TC_MENU)   { r = 40;  g = 40;  b = 120; }
      platform_overlay_fillrect(tc_btn[i].x, tc_btn[i].y,
                                tc_btn[i].w, tc_btn[i].h,
                                r, g, b, alpha);
    }
  platform_overlay_end();
}

void touch_controls_apply_player(Player& tux)
{
  if (!tc_enabled)
    return;

  tc_player = &tux;

  if (tc_btn[TC_LEFT].held)
    tux.key_event((SDLKey)keymap.left, DOWN);
  if (tc_btn[TC_RIGHT].held)
    tux.key_event((SDLKey)keymap.right, DOWN);
  if (tc_btn[TC_UP].held || tc_btn[TC_JUMP].held || tc_btn[TC_BOTH].held)
    tux.key_event((SDLKey)keymap.jump, DOWN);
  if (tc_btn[TC_DOWN].held)
    tux.key_event((SDLKey)keymap.duck, DOWN);
  if (tc_btn[TC_ACTION].held || tc_btn[TC_BOTH].held)
    tux.key_event((SDLKey)keymap.fire, DOWN);
}

bool touch_controls_held(int dir)
{
  if (!tc_enabled || dir < 0 || dir >= TC_COUNT)
    return false;
  return tc_btn[dir].held;
}

bool touch_controls_menu_nav(int* action)
{
  if (!tc_enabled || !action)
    return false;

  static const int order[] = { TC_UP, TC_DOWN, TC_LEFT, TC_RIGHT, TC_JUMP, TC_ACTION, TC_BOTH };
  static const int mapact[] = { 0, 1, 2, 3, 4, 4, 4 };

  bool found = false;
  for (int i = 0; i < 7; ++i)
    {
      int b = order[i];
      if (tc_btn[b].held && !tc_btn[b].prev_held)
        {
          *action = mapact[i];
          found = true;
          break;
        }
    }

  for (int i = 0; i < 7; ++i)
    tc_btn[order[i]].prev_held = tc_btn[order[i]].held;

  return found;
}

bool touch_controls_escape_pressed(void)
{
  if (!tc_enabled)
    return false;
  tc_ensure_layout();
  bool pressed = tc_btn[TC_MENU].held && !tc_btn[TC_MENU].prev_held;
  tc_btn[TC_MENU].prev_held = tc_btn[TC_MENU].held;
  return pressed;
}
