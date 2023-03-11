//  SuperTux
//  Copyright (C) 2006 Matthias Braun <matze@braunis.de>
//                2018 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_SUPERTUX_COLLISION_COLLISION_SYSTEM_HPP
#define HEADER_SUPERTUX_COLLISION_COLLISION_SYSTEM_HPP

#include <memory>
#include <stdint.h>
#include <vector>

#include "collision/collision.hpp"
#include "math/fwd.hpp"
#include "supertux/tile.hpp"

class CollisionObject;
class CollisionGroundMovementManager;
class DrawingContext;
class Rectf;
class Sector;

class CollisionSystem final
{
public:
  CollisionSystem(Sector& sector);

  void add(CollisionObject* object);
  void remove(CollisionObject* object);

  /** Draw collision shapes for debugging */
  void draw(DrawingContext& context);

  /** Checks for all possible collisions. And calls the
      collision_handlers, which the collision_objects provide for this
      case (or not). */
  void update();

  std::shared_ptr<CollisionGroundMovementManager> const& get_ground_movement_manager()
  {
    return m_ground_movement_manager;
  }

  bool is_free_of_tiles(Rectf const& rect, const bool ignoreUnisolid = false, uint32_t tiletype = Tile::SOLID) const;
  bool is_free_of_statics(Rectf const& rect, CollisionObject const* ignore_object, const bool ignoreUnisolid) const;
  bool is_free_of_movingstatics(Rectf const& rect, CollisionObject const* ignore_object) const;
  bool free_line_of_sight(Vector const& line_start, Vector const& line_end, bool ignore_objects, CollisionObject const* ignore_object) const;

  std::vector<CollisionObject*> get_nearby_objects(Vector const& center, float max_distance) const;

private:
  /** Does collision detection of an object against all other static
      objects (and the tilemap) in the level. Collision response is
      done for the first hit in time. (other hits get ignored, the
      function should be called repeatedly to resolve those)

      returns true if the collision detection should be aborted for
      this object (because of ABORT_MOVE in the collision response or
      no collisions) */
  void collision_static(collision::Constraints* constraints,
                        Vector const& movement, Rectf const& dest,
                        CollisionObject& object);

  void collision_tilemap(collision::Constraints* constraints,
                         Vector const& movement, Rectf const& dest,
                         CollisionObject& object) const;

  uint32_t collision_tile_attributes(Rectf const& dest, Vector const& mov) const;

  void collision_object(CollisionObject* object1, CollisionObject* object2) const;

  void collision_static_constrains(CollisionObject& object);

private:
  Sector& m_sector;

  std::vector<CollisionObject*>  m_objects;

  std::shared_ptr<CollisionGroundMovementManager> m_ground_movement_manager;

private:
  CollisionSystem(CollisionSystem const&) = delete;
  CollisionSystem& operator=(CollisionSystem const&) = delete;
};

#endif

/* EOF */
