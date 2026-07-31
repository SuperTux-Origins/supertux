// On-screen virtual gamepad for touch devices (Android).
// Provides d-pad + Jump + Action and maps the system Back key to Escape.

#ifndef SUPERTUX_TOUCH_CONTROLS_H
#define SUPERTUX_TOUCH_CONTROLS_H

#include "platform_config.h"

class Player;

/** Enable/disable the overlay (default: on under __ANDROID__). */
void touch_controls_set_enabled(bool enabled);
bool touch_controls_is_enabled(void);

/** Clear all held buttons (focus loss, menu open, level restart). */
void touch_controls_reset(void);

/**
 * Handle an SDL event. Returns true if the event was consumed by the pad
 * (caller may skip further game handling for that event).
 * Also treats Android Back (SDLK_AC_BACK) as Escape when applicable —
 * Back is not consumed here; callers should use st_is_escape_key().
 */
bool touch_controls_event(const SDL_Event& event);

/** Draw semi-transparent pad (logical ST_SCREEN coordinates). */
void touch_controls_draw(void);

/** Apply current held state onto the player (call each frame while playing). */
void touch_controls_apply_player(Player& tux);

/** Optional: bind player for immediate key-up on finger release. */
void touch_controls_set_player(Player* p);

/** Held state for worldmap / other consumers. */
bool touch_controls_held(int dir); /* 0=left 1=right 2=up 3=down 4=jump 5=action */

/**
 * One-shot menu navigation from the pad (edge-triggered).
 * Returns true and fills *action if a press occurred this poll.
 * action: 0=up 1=down 2=left 3=right 4=hit
 */
bool touch_controls_menu_nav(int* action);

#endif /* SUPERTUX_TOUCH_CONTROLS_H */
