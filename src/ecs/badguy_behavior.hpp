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
#ifndef HEADER_SUPERTUX_ECS_BADGUY_BEHAVIOR_HPP
#define HEADER_SUPERTUX_ECS_BADGUY_BEHAVIOR_HPP

#include "collision/collision_hit.hpp"

class ArchetypeBadguy;
class BadGuy;
class GameObject;

/** Handlers that give a component behavior on an ArchetypeBadguy.
    Every handler is optional and fetches its component with
    ecs::get<T>(self.get_entity()).

    "All" handlers run for every behavior of the entity, in archetype
    order. For "first" handlers, the first behavior that has one
    replaces the BadGuy default (the ArchetypeBadguy::default_*
    functions call that default). Per frame: update (all), move (first,
    default: physics movement), after_move (all). */
struct BadGuyBehavior
{
  /** all: end of construction */
  void (*construct)(ArchetypeBadguy& self) = nullptr;
  /** all */
  void (*initialize)(ArchetypeBadguy& self) = nullptr;
  void (*activate)(ArchetypeBadguy& self) = nullptr;
  void (*deactivate)(ArchetypeBadguy& self) = nullptr;
  void (*update)(ArchetypeBadguy& self, float dt_sec) = nullptr;
  /** first */
  void (*move)(ArchetypeBadguy& self, float dt_sec) = nullptr;
  /** all */
  void (*after_move)(ArchetypeBadguy& self, float dt_sec) = nullptr;
  /** first */
  void (*collision_solid)(ArchetypeBadguy& self, CollisionHit const& hit) = nullptr;
  HitResponse (*collision_badguy)(ArchetypeBadguy& self, BadGuy& other, CollisionHit const& hit) = nullptr;
  bool (*collision_squished)(ArchetypeBadguy& self, GameObject& object) = nullptr;
  void (*freeze)(ArchetypeBadguy& self) = nullptr;
  void (*ignite)(ArchetypeBadguy& self) = nullptr;
  /** all */
  void (*stop_looping_sounds)(ArchetypeBadguy& self) = nullptr;
  void (*play_looping_sounds)(ArchetypeBadguy& self) = nullptr;
};

/** The behavior of a component type, specialized in badguy_behaviors.cpp */
template<typename T>
BadGuyBehavior const& behavior_of();

#endif

/* EOF */
