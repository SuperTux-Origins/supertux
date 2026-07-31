// On-screen virtual gamepad for touch devices.

#include "touch_controls.h"
#include "platform.h"
#include "globals.h"
#include "defines.h"
#include "screen.h"
#include "player.h"
#include "text.h"

#include <string.h>

enum {
  TC_LEFT = 0,
  TC_RIGHT,
  TC_UP,
  TC_DOWN,
  TC_JUMP,
  TC_ACTION,
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

static void
tc_layout(void)
{
  /* Logical 640×480 bottom corners. */
  /* D-pad cluster left */
  tc_btn[TC_LEFT].x = 12;   tc_btn[TC_LEFT].y = 368;  tc_btn[TC_LEFT].w = 64; tc_btn[TC_LEFT].h = 64;
  tc_btn[TC_RIGHT].x = 140; tc_btn[TC_RIGHT].y = 368; tc_btn[TC_RIGHT].w = 64; tc_btn[TC_RIGHT].h = 64;
  tc_btn[TC_UP].x = 76;     tc_btn[TC_UP].y = 304;    tc_btn[TC_UP].w = 64; tc_btn[TC_UP].h = 64;
  tc_btn[TC_DOWN].x = 76;   tc_btn[TC_DOWN].y = 432;  tc_btn[TC_DOWN].w = 64; tc_btn[TC_DOWN].h = 64;
  /* Action buttons right */
  tc_btn[TC_JUMP].x = 540;   tc_btn[TC_JUMP].y = 360;  tc_btn[TC_JUMP].w = 80; tc_btn[TC_JUMP].h = 80;
  tc_btn[TC_ACTION].x = 450; tc_btn[TC_ACTION].y = 400; tc_btn[TC_ACTION].w = 72; tc_btn[TC_ACTION].h = 72;
  /* Menu / Escape — top-left, always available */
  tc_btn[TC_MENU].x = 8;     tc_btn[TC_MENU].y = 8;    tc_btn[TC_MENU].w = 48; tc_btn[TC_MENU].h = 48;

  for (int i = 0; i < TC_COUNT; ++i)
    {
      tc_btn[i].held = false;
      tc_btn[i].prev_held = false;
      tc_btn[i].has_finger = false;
      tc_btn[i].finger = 0;
    }
  tc_inited_layout = true;
}

void touch_controls_set_enabled(bool enabled)
{
  tc_enabled = enabled;
  if (!tc_inited_layout)
    tc_layout();
  if (!enabled)
    touch_controls_reset();
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

static void
tc_finger_to_logical(float fx, float fy, int* lx, int* ly)
{
  int ww = ST_SCREEN_W;
  int wh = ST_SCREEN_H;
  platform_get_window_size(&ww, &wh);
  int x = (int)(fx * (float)ww + 0.5f);
  int y = (int)(fy * (float)wh + 0.5f);
  platform_window_to_logical(&x, &y);
  *lx = x;
  *ly = y;
}

#ifdef USE_SDL2
typedef SDL_FingerID TcFingerId;
#else
typedef int TcFingerId;
#endif

static void
tc_press(int id, TcFingerId finger)
{
  if (id < 0 || id >= TC_COUNT)
    return;
  tc_btn[id].held = true;
  tc_btn[id].has_finger = true;
  tc_btn[id].finger = finger;
}

/* Optional callback target for releases (set by gameloop). */
static Player* tc_player = 0;

void touch_controls_set_player(Player* p)
{
  tc_player = p;
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
      if (!tc_btn[TC_JUMP].held)
        tc_player->key_event((SDLKey)keymap.jump, UP);
      break;
    case TC_JUMP:
      if (!tc_btn[TC_UP].held)
        tc_player->key_event((SDLKey)keymap.jump, UP);
      break;
    case TC_DOWN:   tc_player->key_event((SDLKey)keymap.duck, UP); break;
    case TC_ACTION: tc_player->key_event((SDLKey)keymap.fire, UP); break;
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

  if (!tc_inited_layout)
    tc_layout();

#ifdef USE_SDL2
  switch (event.type)
    {
    case SDL_FINGERDOWN:
      {
        int lx, ly;
        tc_finger_to_logical(event.tfinger.x, event.tfinger.y, &lx, &ly);
        int hit = tc_hit(lx, ly);
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
        int lx, ly;
        tc_finger_to_logical(event.tfinger.x, event.tfinger.y, &lx, &ly);
        bool owned = false;
        for (int i = 0; i < TC_COUNT; ++i)
          if (tc_btn[i].has_finger && tc_btn[i].finger == event.tfinger.fingerId)
            owned = true;
        if (owned || tc_hit(lx, ly) >= 0)
          {
            tc_move_finger(event.tfinger.fingerId, lx, ly);
            return true;
          }
      }
      break;
    /* Mouse fallback (single touch synthesis) */
    case SDL_MOUSEBUTTONDOWN:
      if (event.button.button == SDL_BUTTON_LEFT)
        {
          int x = event.button.x, y = event.button.y;
          platform_window_to_logical(&x, &y);
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
        {
          tc_release_finger((TcFingerId)-1);
        }
      break;
    case SDL_MOUSEMOTION:
      if (event.motion.state & SDL_BUTTON_LMASK)
        {
          int x = event.motion.x, y = event.motion.y;
          platform_window_to_logical(&x, &y);
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

void touch_controls_draw(void)
{
  if (!tc_enabled)
    return;
  if (!tc_inited_layout)
    tc_layout();

  for (int i = 0; i < TC_COUNT; ++i)
    {
      int alpha = tc_btn[i].held ? 140 : 70;
      int r = 40, g = 40, b = 40;
      if (i == TC_JUMP) { r = 40; g = 120; b = 40; }
      if (i == TC_ACTION) { r = 120; g = 40; b = 40; }
      if (i == TC_MENU) { r = 40; g = 40; b = 100; }
      fillrect(tc_btn[i].x, tc_btn[i].y, tc_btn[i].w, tc_btn[i].h,
               r, g, b, alpha);
    }

  if (white_small_text)
    {
      white_small_text->draw("L", tc_btn[TC_LEFT].x + 24, tc_btn[TC_LEFT].y + 24, 0);
      white_small_text->draw("R", tc_btn[TC_RIGHT].x + 24, tc_btn[TC_RIGHT].y + 24, 0);
      white_small_text->draw("U", tc_btn[TC_UP].x + 24, tc_btn[TC_UP].y + 24, 0);
      white_small_text->draw("D", tc_btn[TC_DOWN].x + 24, tc_btn[TC_DOWN].y + 24, 0);
      white_small_text->draw("J", tc_btn[TC_JUMP].x + 32, tc_btn[TC_JUMP].y + 32, 0);
      white_small_text->draw("A", tc_btn[TC_ACTION].x + 28, tc_btn[TC_ACTION].y + 28, 0);
      white_small_text->draw("M", tc_btn[TC_MENU].x + 16, tc_btn[TC_MENU].y + 16, 0);
    }
}

bool touch_controls_escape_pressed(void)
{
  if (!tc_enabled)
    return false;
  if (!tc_inited_layout)
    tc_layout();
  bool pressed = tc_btn[TC_MENU].held && !tc_btn[TC_MENU].prev_held;
  tc_btn[TC_MENU].prev_held = tc_btn[TC_MENU].held;
  return pressed;
}

void touch_controls_apply_player(Player& tux)
{
  if (!tc_enabled)
    return;

  tc_player = &tux;

  /* Drive held directions every frame; releases are applied in event path. */
  if (tc_btn[TC_LEFT].held)
    tux.key_event((SDLKey)keymap.left, DOWN);
  if (tc_btn[TC_RIGHT].held)
    tux.key_event((SDLKey)keymap.right, DOWN);
  if (tc_btn[TC_UP].held || tc_btn[TC_JUMP].held)
    tux.key_event((SDLKey)keymap.jump, DOWN);
  if (tc_btn[TC_DOWN].held)
    tux.key_event((SDLKey)keymap.duck, DOWN);
  if (tc_btn[TC_ACTION].held)
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

  /* Edge-trigger: press when held && !prev_held */
  static const int order[] = { TC_UP, TC_DOWN, TC_LEFT, TC_RIGHT, TC_JUMP, TC_ACTION };
  static const int mapact[] = { 0, 1, 2, 3, 4, 4 }; /* jump/action both = hit */

  bool found = false;
  for (int i = 0; i < 6; ++i)
    {
      int b = order[i];
      if (tc_btn[b].held && !tc_btn[b].prev_held)
        {
          *action = mapact[i];
          found = true;
          break;
        }
    }

  /* Only advance prev for nav buttons — TC_MENU uses escape_pressed(). */
  for (int i = 0; i < 6; ++i)
    tc_btn[order[i]].prev_held = tc_btn[order[i]].held;

  return found;
}
