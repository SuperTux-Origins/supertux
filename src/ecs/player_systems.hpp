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

#ifndef HEADER_SUPERTUX_ECS_PLAYER_SYSTEMS_HPP
#define HEADER_SUPERTUX_ECS_PLAYER_SYSTEMS_HPP

class Player;

/** The per-frame steps of Player::update() that work on the player
    components (ecs/player_components.hpp). Player::update() calls them
    in its original order. A friend of Player, so the steps can use the
    remaining Player helpers while the decomposition goes on. */
struct PlayerSystems
{
  /** Fade out the multiplayer name tag */
  static void update_tag(Player& self, float dt_sec);
  /** Enter and leave water, and jump out of it */
  static void update_swimming(Player& self);
  /** Cling to walls in walljump tiles */
  static void update_wall_cling(Player& self);
  /** Roll the sprite if Tux is rolling (stone) */
  static void update_rolling(Player& self, float dt_sec);
  /** Hitbox width for running, and walking down slopes */
  static void update_ground_movement(Player& self);
  /** Keep the direction and spin the sprite while backflipping */
  static void update_backflip(Player& self, float dt_sec);
  /** Coyote time, fall mode and landing from jumps and backflips */
  static void update_landing(Player& self);
  /** Let the horizontal boost wear off */
  static void update_boost(Player& self, float dt_sec);
  /** Sparkle while invincible */
  static void spawn_invincible_sparkles(Player& self);
  /** When climbing, animate only while moving */
  static void update_climb_animation(Player& self);
};

#endif

/* EOF */
