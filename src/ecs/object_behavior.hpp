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
#ifndef HEADER_SUPERTUX_ECS_OBJECT_BEHAVIOR_HPP
#define HEADER_SUPERTUX_ECS_OBJECT_BEHAVIOR_HPP

#include "collision/collision_hit.hpp"
#include "util/reader_mapping.hpp"

class ArchetypeObject;
class Bullet;
class Player;
class DrawingContext;
class GameObject;

/** Handlers that give a component behavior on an ArchetypeObject, the
    generic shell for non-badguy objects (see BadGuyBehavior for the
    dispatch rules). "all" handlers run for every behavior in archetype
    order, for "first" handlers the first behavior that has one replaces
    the default. */
struct ObjectBehavior
{
  /** all: read instance data from the level object, before construct */
  void (*read)(ArchetypeObject& self, ReaderMapping const& mapping) = nullptr;
  /** all: end of construction */
  void (*construct)(ArchetypeObject& self) = nullptr;
  /** all */
  void (*update)(ArchetypeObject& self, float dt_sec) = nullptr;
  /** first, default: draw the sprite */
  void (*draw)(ArchetypeObject& self, DrawingContext& context) = nullptr;
  /** first, default: FORCE_MOVE */
  HitResponse (*collision)(ArchetypeObject& self, GameObject& other, CollisionHit const& hit) = nullptr;
  /** first, default: nothing */
  void (*collision_solid)(ArchetypeObject& self, CollisionHit const& hit) = nullptr;
  /** first, default: true */
  bool (*collides)(ArchetypeObject const& self, GameObject& other, CollisionHit const& hit) = nullptr;
  /** first, default: nothing; e.g. a block hit by the player */
  void (*hit)(ArchetypeObject& self, Player& player) = nullptr;
};

/** The object behavior of a component type, specialized in object_behaviors.cpp */
template<typename T>
ObjectBehavior const& object_behavior_of();

#endif

/* EOF */
