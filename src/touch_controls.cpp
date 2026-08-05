// SPDX-FileCopyrightText: 2003-2006 SuperTux Development Team
// SPDX-FileCopyrightText: 2024-2026 SuperTux Milestone 1 port contributors
// SPDX-License-Identifier: GPL-3.0-or-later

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
#include "game_file.h"
#include "menu.h"

#include "SDL_image.h"

#include <string.h>
#include <string>
#include <vector>

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
/* Sticky Both: once a finger swipes Action→Both, stay on Both until the
   finger moves well into the lower Action zone (hysteresis / Fitts). */
static bool tc_sticky_both = false;
/* Sticky d-pad direction (TC_LEFT..TC_DOWN, or -1). Outer-edge slack only;
   the cross centre gap clears sticky so the player can stop without lift. */
static int tc_sticky_dpad = -1;

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
   * Right face cluster (bottom-right), designed for one-thumb play:
   *
   *     [BOTH]          ← swipe up from Action = run+jump
   *   [ACTION] [JUMP]   ← side by side
   *
   * Finger motion already retargets held buttons, so sliding Action→Both
   * keeps run and adds jump without a second finger.
   */
  /* Action and Jump ~50% larger than the d-pad cells. Both is a tall
     extension of Action with zero gap (swipe up = run+jump). */
  int face_s = bs * 3 / 2;
  if (face_s < 72) face_s = 72;
  if (face_s > 132) face_s = 132;
  int face_gap = gap / 2;
  if (face_gap < 6) face_gap = 6;

  tc_btn[TC_JUMP].w = face_s;
  tc_btn[TC_JUMP].h = face_s;
  tc_btn[TC_JUMP].x = ww - pad - face_s;
  tc_btn[TC_JUMP].y = wh - pad - face_s;

  tc_btn[TC_ACTION].w = face_s;
  tc_btn[TC_ACTION].h = face_s;
  tc_btn[TC_ACTION].x = ww - pad - face_s - face_gap - face_s;
  tc_btn[TC_ACTION].y = wh - pad - face_s;

  /* Contiguous with Action: bottom of Both == top of Action (no free pixels). */
  int both_h = face_s * 5 / 2;
  if (both_h < face_s) both_h = face_s;
  tc_btn[TC_BOTH].w = face_s;
  tc_btn[TC_BOTH].h = both_h;
  tc_btn[TC_BOTH].x = tc_btn[TC_ACTION].x;
  tc_btn[TC_BOTH].y = tc_btn[TC_ACTION].y - both_h;

  /* Menu — top-left, same size as the face buttons. */
  tc_btn[TC_MENU].x = pad;
  tc_btn[TC_MENU].y = pad;
  tc_btn[TC_MENU].w = face_s;
  tc_btn[TC_MENU].h = face_s;

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
  tc_sticky_both = false;
  tc_sticky_dpad = -1;
}

static int
tc_hit_raw(int x, int y)
{
  for (int i = 0; i < TC_COUNT; ++i)
    {
      if (x >= tc_btn[i].x && x < tc_btn[i].x + tc_btn[i].w
          && y >= tc_btn[i].y && y < tc_btn[i].y + tc_btn[i].h)
        return i;
    }
  return -1;
}

/* Hit-test with Action/Both hysteresis: once sticky, stay on Both until the
   finger is clearly in the lower 2/3 of Action (or leaves the column). */
static int
tc_hit(int x, int y)
{
  const TcButton& act = tc_btn[TC_ACTION];
  const TcButton& both = tc_btn[TC_BOTH];
  bool in_col = (x >= act.x && x < act.x + act.w
                 && y >= both.y && y < act.y + act.h);

  if (in_col)
    {
      /* Shared edge is act.y (Both bottom == Action top). Hysteresis:
         enter Both as soon as y crosses above the edge; leave Both only
         when deep in the lower part of Action — never a dead band. */
      int enter_both_y = act.y;
      int leave_both_y = act.y + (act.h * 3) / 4;

      if (tc_sticky_both)
        {
          if (y >= leave_both_y)
            tc_sticky_both = false;
        }
      else if (y < enter_both_y)
        {
          tc_sticky_both = true;
        }

      if (tc_sticky_both || y < enter_both_y)
        return TC_BOTH;
      return TC_ACTION;
    }

  /* Outside the column: clear sticky so a new press starts fresh. */
  if (!(x >= act.x && x < act.x + act.w))
    tc_sticky_both = false;

  int hit = tc_hit_raw(x, y);

  /* D-pad: once a direction is pressed, expand its hit zone *outward*
     only so sliding further past the outer edge keeps the direction.
     Do not claim the cross centre gap — that must remain a release
     zone (finger back to centre = stop). */
  if (hit >= TC_LEFT && hit <= TC_DOWN)
    {
      tc_sticky_dpad = hit;
      return hit;
    }

  if (tc_sticky_dpad >= TC_LEFT && tc_sticky_dpad <= TC_DOWN)
    {
      const TcButton& b = tc_btn[tc_sticky_dpad];
      int expand = b.w / 2; /* modest outward slack */
      if (expand < 24) expand = 24;
      if (expand > 64) expand = 64;
      int cross = expand / 3; /* slight cross-axis fat-finger room */
      int L = b.x, R = b.x + b.w, T = b.y, B = b.y + b.h;
      if (tc_sticky_dpad == TC_LEFT)
        {
          L -= expand;
          T -= cross;
          B += cross;
          /* R stays at original inner edge — centre gap is free */
        }
      else if (tc_sticky_dpad == TC_RIGHT)
        {
          R += expand;
          T -= cross;
          B += cross;
        }
      else if (tc_sticky_dpad == TC_UP)
        {
          T -= expand;
          L -= cross;
          R += cross;
        }
      else /* TC_DOWN */
        {
          B += expand;
          L -= cross;
          R += cross;
        }

      if (x >= L && x < R && y >= T && y < B)
        return tc_sticky_dpad;

      tc_sticky_dpad = -1;
    }

  return hit;
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
  /* Synthetic mouse uses finger id -1. Never let it steal a button that
     a real finger already owns — that leaves held stuck when FINGERUP
     arrives for the original id and mouse-up never matches. */
  if (finger == (TcFingerId)-1
      && tc_btn[id].has_finger
      && tc_btn[id].finger != (TcFingerId)-1)
    return;
  tc_btn[id].held = true;
  tc_btn[id].has_finger = true;
  tc_btn[id].finger = finger;
}

/*
 * Virtual pad drives Player::input directly with a fixed layout. It must
 * not go through keymap / key_event — those are for the physical keyboard
 * configurator, and routing the pad through them made the pad follow
 * remaps and pollute MN_CONTROLFIELD captures.
 *
 * apply: only assert DOWN for held pad buttons (never force UP here, so a
 * physical keyboard can still hold the same action).
 * release: clear an axis only when no pad button still provides it.
 */
static void
tc_apply_player_downs(void)
{
  if (!tc_player)
    return;
  if (tc_btn[TC_LEFT].held)
    tc_player->input.left = DOWN;
  if (tc_btn[TC_RIGHT].held)
    tc_player->input.right = DOWN;
  if (tc_btn[TC_DOWN].held)
    tc_player->input.down = DOWN;
  if (tc_btn[TC_UP].held || tc_btn[TC_JUMP].held || tc_btn[TC_BOTH].held)
    tc_player->input.up = DOWN;
  if (tc_btn[TC_ACTION].held || tc_btn[TC_BOTH].held)
    tc_player->input.fire = DOWN;
}

static void
tc_release_button(int i)
{
  if (!tc_btn[i].held)
    return;
  tc_btn[i].held = false;
  tc_btn[i].has_finger = false;
  if (i >= TC_LEFT && i <= TC_DOWN && tc_sticky_dpad == i)
    tc_sticky_dpad = -1;
  if (!tc_player)
    return;
  switch (i)
    {
    case TC_LEFT:
      if (!tc_btn[TC_LEFT].held)
        tc_player->input.left = UP;
      break;
    case TC_RIGHT:
      if (!tc_btn[TC_RIGHT].held)
        tc_player->input.right = UP;
      break;
    case TC_DOWN:
      if (!tc_btn[TC_DOWN].held)
        tc_player->input.down = UP;
      break;
    case TC_UP:
    case TC_JUMP:
    case TC_BOTH:
      if (!tc_btn[TC_UP].held && !tc_btn[TC_JUMP].held && !tc_btn[TC_BOTH].held)
        tc_player->input.up = UP;
      if (i == TC_BOTH || i == TC_ACTION)
        {
          if (!tc_btn[TC_ACTION].held && !tc_btn[TC_BOTH].held)
            tc_player->input.fire = UP;
        }
      break;
    case TC_ACTION:
      if (!tc_btn[TC_ACTION].held && !tc_btn[TC_BOTH].held)
        tc_player->input.fire = UP;
      break;
    default:
      break;
    }
}

static void
tc_release_finger(TcFingerId finger)
{
  bool owned_face = false;
  bool owned_dpad = false;
  for (int i = 0; i < TC_COUNT; ++i)
    {
      if (tc_btn[i].has_finger && tc_btn[i].finger == finger)
        {
          if (i == TC_ACTION || i == TC_BOTH)
            owned_face = true;
          if (i >= TC_LEFT && i <= TC_DOWN)
            owned_dpad = true;
          tc_release_button(i);
        }
    }
  if (owned_face)
    tc_sticky_both = false;
  if (owned_dpad)
    tc_sticky_dpad = -1;
}

static void
tc_move_finger(TcFingerId finger, int x, int y)
{
  int hit = tc_hit(x, y);
  /* Press the new target first so Action→Both keeps fire held (release
     of Action checks TC_BOTH.held and skips the fire UP). */
  if (hit >= 0)
    tc_press(hit, finger);
  else if (tc_sticky_dpad >= 0)
    {
      /* Off every pad cell: drop sticky so a later press starts clean. */
      tc_sticky_dpad = -1;
    }
  for (int i = 0; i < TC_COUNT; ++i)
    {
      if (!(tc_btn[i].has_finger && tc_btn[i].finger == finger && i != hit))
        continue;
      /* While sticky Both, never release Action from this finger — fire
         stays down for the whole contiguous strip. */
      if (hit == TC_BOTH && i == TC_ACTION)
        {
          tc_btn[i].held = true; /* keep action asserted under both */
          continue;
        }
      if (hit == TC_ACTION && i == TC_BOTH)
        {
          /* Moving back down into Action: drop Both (and jump) only. */
          tc_release_button(i);
          continue;
        }
      tc_release_button(i);
    }
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
#ifdef __ANDROID__
      /* Android synthesizes mouse events from each touch. Handling both
         FINGER* and MOUSE* double-owns buttons under different ids and
         leaves the d-pad stuck when FINGERUP and mouse-up disagree. */
      break;
#else
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
#endif
    case SDL_MOUSEBUTTONUP:
#ifdef __ANDROID__
      break;
#else
      if (event.button.button == SDL_BUTTON_LEFT)
        tc_release_finger((TcFingerId)-1);
      break;
#endif
    case SDL_MOUSEMOTION:
#ifdef __ANDROID__
      break;
#else
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
#endif
    default:
      break;
    }
#else
  (void)event;
#endif
  return false;
}

/*
 * tv-bezel.png defaults (image pixel space). Overridden by
 * images/status/tv-bezel.hole when present:
 *   hx hy hw hh [iw ih]
 * e.g. "524 51 876 606 1920 1125"
 */
static int bezel_hx = 524, bezel_hy = 51, bezel_hw = 876, bezel_hh = 606;
static int bezel_iw = 1920, bezel_ih = 1125;

static Surface* tc_bezel = 0;
static bool tc_bezel_tried = false;

static void
tc_load_bezel_hole(void)
{
  std::string hole_path = datadir + "/images/status/tv-bezel.hole";
  std::vector<char> buf;
  if (!game_file_read(hole_path, buf) || buf.empty())
    return;
  buf.push_back('\0');
  int hx = 0, hy = 0, hw = 0, hh = 0, iw = 0, ih = 0;
  int n = sscanf(&buf[0], "%d %d %d %d %d %d", &hx, &hy, &hw, &hh, &iw, &ih);
  if (n >= 4 && hw > 0 && hh > 0)
    {
      bezel_hx = hx;
      bezel_hy = hy;
      bezel_hw = hw;
      bezel_hh = hh;
      if (n >= 6 && iw > 0 && ih > 0)
        {
          bezel_iw = iw;
          bezel_ih = ih;
        }
      st_vlog("[video] TV bezel hole from .hole: %d %d %dx%d (img %dx%d)\n",
              bezel_hx, bezel_hy, bezel_hw, bezel_hh, bezel_iw, bezel_ih);
    }
}

static void
tc_draw_bezel(void)
{
  if (!tc_bezel_tried)
    {
      tc_bezel_tried = true;
      tc_load_bezel_hole();
      std::string path = datadir + "/images/status/tv-bezel.png";
      /* Optional chrome — load failure must not abort the game. */
      SDL_Surface* raw = 0;
      {
        SDL_RWops* rw = open_game_file(path);
        if (rw)
          raw = IMG_Load_RW(rw, 1);
      }
      if (!raw)
        {
          st_vlog("[video] TV bezel load(%s) failed: %s\n",
                  path.c_str(), IMG_GetError());
        }
      else
        {
          tc_bezel = new Surface(raw, USE_ALPHA);
          SDL_FreeSurface(raw);
          /* Prefer real decoded size over design defaults / .hole iw/ih. */
          if (tc_bezel && tc_bezel->w > 0 && tc_bezel->h > 0)
            {
              bezel_iw = tc_bezel->w;
              bezel_ih = tc_bezel->h;
            }
          st_vlog("[video] loaded arctic TV bezel (%dx%d) hole=(%d,%d %dx%d)\n",
                  bezel_iw, bezel_ih, bezel_hx, bezel_hy, bezel_hw, bezel_hh);
        }
    }
  if (!tc_bezel)
    return;

  int lx = 0, ly = 0, lw = ST_SCREEN_W, lh = ST_SCREEN_H;
  platform_get_letterbox(&lx, &ly, &lw, &lh);

  /*
   * Map the transparent hole rectangle exactly onto the letterboxed
   * playfield. Independent sx/sy: the hole need not be 4:3 (current
   * asset is ~876×606). Frame may stretch slightly; the game fills the
   * cutout with no black bars inside the TV.
   */
  float hx = (float)bezel_hx;
  float hy = (float)bezel_hy;
  float hw = (float)bezel_hw;
  float hh = (float)bezel_hh;
  if (hw < 1.0f) hw = 1.0f;
  if (hh < 1.0f) hh = 1.0f;

  float sx = (float)lw / hw;
  float sy = (float)lh / hh;
  int dw = (int)((float)bezel_iw * sx + 0.5f);
  int dh = (int)((float)bezel_ih * sy + 0.5f);
  int dx = lx - (int)(hx * sx + 0.5f);
  int dy = ly - (int)(hy * sy + 0.5f);

  static bool logged_rect = false;
  if (!logged_rect)
    {
      logged_rect = true;
      st_vlog("[video] TV bezel draw: dst=(%d,%d %dx%d) hole→lb=(%d,%d %dx%d) scale=(%.3f,%.3f)\n",
              dx, dy, dw, dh, lx, ly, lw, lh, sx, sy);
    }

  platform_overlay_surface(tc_bezel, dx, dy, dw, dh);
}

void touch_controls_draw(void)
{
  if (!tc_enabled)
    return;

  tc_ensure_layout();

  int ww = ST_SCREEN_W, wh = ST_SCREEN_H;
  platform_get_window_size(&ww, &wh);

  int lx = 0, ly = 0, lw = ST_SCREEN_W, lh = ST_SCREEN_H;
  platform_get_letterbox(&lx, &ly, &lw, &lh);

  platform_overlay_begin();
  /*
   * Fill only the margin bands (never the letterboxed playfield).
   * Use a near-black underlay so an arctic-coloured bezel frame is
   * visible against it; a matching (25,45,70) fill made the frame
   * disappear into the chrome.
   */
  const int ar = 8, ag = 12, ab = 20;
  if (ly > 0)
    platform_overlay_fillrect(0, 0, ww, ly, ar, ag, ab, 255);
  if (ly + lh < wh)
    platform_overlay_fillrect(0, ly + lh, ww, wh - (ly + lh), ar, ag, ab, 255);
  if (lx > 0)
    platform_overlay_fillrect(0, ly, lx, lh, ar, ag, ab, 255);
  if (lx + lw < ww)
    platform_overlay_fillrect(lx + lw, ly, ww - (lx + lw), lh, ar, ag, ab, 255);

  tc_draw_bezel();

  for (int i = 0; i < TC_COUNT; ++i)
    {
      /* Idle ~70% opaque; held almost solid — was 90/160 and too faint. */
      int alpha = tc_btn[i].held ? 230 : 180;
      int r = 70, g = 70, b = 80;
      if (i == TC_JUMP)   { r = 50;  g = 180; b = 60; }
      if (i == TC_ACTION) { r = 200; g = 50;  b = 50; }
      if (i == TC_BOTH)   { r = 200; g = 160; b = 40; } /* run+jump */
      if (i == TC_MENU)   { r = 60;  g = 60;  b = 180; }
      if (i == TC_LEFT || i == TC_RIGHT || i == TC_UP || i == TC_DOWN)
        { r = 90; g = 100; b = 120; }
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
  /* Fixed layout — independent of keyboard configurator / keymap. */
  tc_apply_player_downs();
}

bool touch_controls_held(int dir)
{
  if (!tc_enabled || dir < 0 || dir >= TC_COUNT)
    return false;
  return tc_btn[dir].held;
}

bool touch_controls_just_pressed(int dir)
{
  if (!tc_enabled || dir < 0 || dir >= TC_COUNT)
    return false;
  bool pressed = tc_btn[dir].held && !tc_btn[dir].prev_held;
  tc_btn[dir].prev_held = tc_btn[dir].held;
  return pressed;
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

static void
tc_inject_menu_key(Menu* menu, SDLKey key)
{
  if (!menu)
    return;
  SDL_Event syn;
  memset(&syn, 0, sizeof(syn));
  syn.type = SDL_KEYDOWN;
  syn.key.keysym.sym = key;
  menu->event(syn);
}

bool touch_controls_process_event(SDL_Event& event, bool* want_escape)
{
  if (want_escape)
    *want_escape = false;

  /* Window close / SIGINT→SDL_QUIT must reach the caller's quit path.
     When a menu is open we would otherwise return true for every event and
     swallow SDL_QUIT (title always has main_menu; pause menus too). */
  if (event.type == SDL_QUIT)
    return false;

  bool ate = touch_controls_event(event);
  bool esc = st_is_escape_event(event) || touch_controls_escape_pressed();
  Menu* menu = Menu::current();

  if (menu)
    {
      /* Do not treat Menu/Start as Escape while rebinding a control field —
         that would steal the press meant to assign the Menu button. */
      bool binding = false;
      if (menu->active_item >= 0
          && menu->active_item < (int)menu->item.size()
          && menu->item[menu->active_item].kind == MN_CONTROLFIELD)
        binding = true;

      if (esc && !binding)
        tc_inject_menu_key(menu, SDLK_ESCAPE);
      else if (!ate)
        menu->event(event);

      /* Do not inject pad nav as KEYDOWNs while binding a key — that would
         capture SDLK_UP/RETURN etc. into the configurator. */
      int tact = 0;
      if (!binding && touch_controls_menu_nav(&tact))
        {
          SDLKey key = SDLK_RETURN;
          if (tact == 0) key = SDLK_UP;
          else if (tact == 1) key = SDLK_DOWN;
          else if (tact == 2) key = SDLK_LEFT;
          else if (tact == 3) key = SDLK_RIGHT;
          tc_inject_menu_key(menu, key);
        }
      return true;
    }

  if (esc && want_escape)
    *want_escape = true;

  return ate;
}
