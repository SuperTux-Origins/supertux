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
#ifndef HEADER_SUPERTUX_ECS_BADGUY_COMPONENTS_HPP
#define HEADER_SUPERTUX_ECS_BADGUY_COMPONENTS_HPP

#include <optional>
#include <string>

#include "supertux/timer.hpp"
#include "util/reader_mapping.hpp"

/** Walks along the floor, turning at walls and optionally at ledges. */
struct Walker
{
  /** WalkingBadguy holds a reference to its Walker */
  static constexpr auto in_place_delete = true;

  float speed = 80.0f;
  /** maximum drop before turning around at a ledge, -1 to walk off any ledge */
  int max_drop_height = -1;
  std::string left_action = "left";
  std::string right_action = "right";

  /** too many turns within a second make the badguy dizzy, it falls off */
  Timer turn_around_timer = {};
  int turn_around_counter = 0;

  /** Set by other behaviors (Patrol) for the current frame only:
      velocity to walk towards instead of +/-speed, and the acceleration
      multiplier. */
  std::optional<float> target_velocity = {};
  float acceleration = 1.0f;
};

inline void read_component(ReaderMapping const& mapping, Walker& walker)
{
  mapping.read("speed", walker.speed);
  mapping.read("max-drop-height", walker.max_drop_height);
  mapping.read("left-action", walker.left_action);
  mapping.read("right-action", walker.right_action);
}

/** Floats down slowly when there is no ground directly below
    (walkingleaf, viciousivy). */
struct Floater
{
  float max_fall_speed = 35.0f;
};

inline void read_component(ReaderMapping const& mapping, Floater& floater)
{
  mapping.read("max-fall-speed", floater.max_fall_speed);
}

/** Walks back and forth around the start position, slowing down before
    turning (crystallo). Requires a Walker. */
struct Patrol
{
  float radius = 100.0f;
  /** walk acceleration multiplier */
  float acceleration = 1.0f;
  /** action prefix used while slower than walk speed, empty for none */
  std::string slowdown_action;
};

inline void read_component(ReaderMapping const& mapping, Patrol& patrol)
{
  mapping.read("radius", patrol.radius);
  mapping.read("acceleration", patrol.acceleration);
  mapping.read("slowdown-action", patrol.slowdown_action);
}

/** How the badguy reacts to being stomped. Without this component the
    BadGuy default applies (not squishable). */
struct SquishReaction
{
  /** sprite action, suffixed with -left/-right */
  std::string action = "squished";
  /** keep the sprite's bottom edge in place when switching action */
  bool anchor_bottom = false;
  /** stop moving and fall with normal gravity */
  bool stop = false;
  std::string particles;
  int particle_count = 0;
};

inline void read_component(ReaderMapping const& mapping, SquishReaction& squish)
{
  mapping.read("action", squish.action);
  mapping.read("anchor-bottom", squish.anchor_bottom);
  mapping.read("stop", squish.stop);
  mapping.read("particles", squish.particles);
  mapping.read("particle-count", squish.particle_count);
}

#endif

/* EOF */
