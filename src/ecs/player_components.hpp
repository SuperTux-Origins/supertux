//  SuperTux
//  Copyright (C) 2026 Ingo Ruhnke <grumbel@gmail.com>
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef HEADER_SUPERTUX_ECS_PLAYER_COMPONENTS_HPP
#define HEADER_SUPERTUX_ECS_PLAYER_COMPONENTS_HPP

#include "supertux/timer.hpp"

// State of the Player, kept in the registry. The Player holds
// references to these, so they must not move when other players'
// components are removed.

/** Swimming in water tiles and the jump out of the water */
struct PlayerSwim
{
  static constexpr auto in_place_delete = true;

  bool swimming = false;
  bool boosting = false;
  /** no water tile found this frame */
  bool no_water = true;
  float angle = 0.0f;
  float accel_modifier = 100.0f;
  bool water_jump = false;
};

/** Clinging to walls and jumping off them */
struct PlayerWallJump
{
  static constexpr auto in_place_delete = true;

  bool on_left_wall = false;
  bool on_right_wall = false;
  bool in_walljump_tile = false;
  bool can_walljump = false;
};

/** Jumping, backflips and buttjumps */
struct PlayerJump
{
  static constexpr auto in_place_delete = true;

  enum FallMode { ON_GROUND, JUMPING, TRAMPOLINE_JUMP, FALLING };

  bool jumping = false;
  bool can_jump = true;
  /** started when player presses the jump button; runs until Tux jumps or JUMP_GRACE_TIME runs out */
  Timer jump_button_timer = {};
  /** started when Tux falls off a ledge; runs until Tux jumps or COYOTE_TIME runs out */
  Timer coyote_timer = {};
  bool early_apex = false;
  bool wants_buttjump = false;
  bool does_buttjump = false;
  Timer buttjump_timer = {};
  bool backflipping = false;
  int backflip_direction = 0;
  Timer backflip_timer = {};
  FallMode fall_mode = ON_GROUND;
  float last_ground_y = 0.0f;
};

/** Dying, winning and the timers that protect Tux */
struct PlayerLife
{
  static constexpr auto in_place_delete = true;

  bool dead = false;
  bool dying = false;
  Timer dying_timer = {};
  bool winning = false;
  Timer invincible_timer = {};
  Timer safe_timer = {};
  /** float around and through solid objects */
  bool ghost_mode = false;
  /** switch to ghost mode rather than dying */
  bool edit_mode = false;
};

#endif

/* EOF */
