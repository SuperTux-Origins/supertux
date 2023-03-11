//  SuperTux -  A Jump'n Run
//  Copyright (C) 2006 Matthias Braun <matze@braunis.de>
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

#ifndef HEADER_SUPERTUX_SUPERTUX_SECTOR_HPP
#define HEADER_SUPERTUX_SUPERTUX_SECTOR_HPP

#include <vector>
#include <stdint.h>

#include "math/anchor_point.hpp"
#include "math/easing.hpp"
#include "math/fwd.hpp"
#include "squirrel/squirrel_environment.hpp"
#include "supertux/d_scope.hpp"
#include "supertux/game_object_manager.hpp"
#include "supertux/tile.hpp"
#include "video/color.hpp"

namespace collision {
class Constraints;
}

class Camera;
class CollisionSystem;
class CollisionGroundMovementManager;
class DisplayEffect;
class DrawingContext;
class Level;
class MovingObject;
class Player;
class Rectf;
class Size;
class TileMap;
class Writer;

/** Represents one of (potentially) multiple, separate parts of a Level.
    Sectors contain GameObjects, e.g. Badguys and Players. */
class Sector final : public GameObjectManager
{
public:
  friend class CollisionSystem;
  friend class EditorSectorMenu;

private:
  static Sector* s_current;

public:
  /** get currently activated sector. */
  static Sector& get() { assert(s_current != nullptr); return *s_current; }
  static Sector* current() { return s_current; }

public:
  Sector(Level& parent);
  ~Sector() override;

  /** Needs to be called after parsing to finish the construction of
      the Sector before using it. */
  void finish_construction();

  Level& get_level() const;

  /** activates this sector (change music, initialize player class, ...) */
  void activate(std::string const& spawnpoint);
  void activate(Vector const& player_pos);
  void deactivate();

  void update(float dt_sec);

  void draw(DrawingContext& context);

  /** stops all looping sounds in whole sector. */
  void stop_looping_sounds();

  /** continues the looping sounds in whole sector. */
  void play_looping_sounds();

  void set_name(std::string const& name_) { m_name = name_; }
  std::string const& get_name() const { return m_name; }

  /** tests if a given rectangle is inside the sector
      (a rectangle that is on top of the sector is considered inside) */
  bool inside(Rectf const& rectangle) const;

  /** Checks if the specified rectangle is free of (solid) tiles.
      Note that this does not include static objects, e.g. bonus blocks. */
  bool is_free_of_tiles(Rectf const& rect, const bool ignoreUnisolid = false, uint32_t tiletype = Tile::SOLID) const;

  /** Checks if the specified rectangle is free of both
      1.) solid tiles and
      2.) MovingObjects in COLGROUP_STATIC.
      Note that this does not include badguys or players. */
  bool is_free_of_statics(Rectf const& rect, MovingObject const* ignore_object = nullptr, const bool ignoreUnisolid = false) const;

  /** Checks if the specified rectangle is free of both
      1.) solid tiles and
      2.) MovingObjects in COLGROUP_STATIC, COLGROUP_MOVINGSTATIC or COLGROUP_MOVING.
      This includes badguys and players. */
  bool is_free_of_movingstatics(Rectf const& rect, MovingObject const* ignore_object = nullptr) const;

  bool free_line_of_sight(Vector const& line_start, Vector const& line_end, bool ignore_objects = false, MovingObject const* ignore_object = nullptr) const;
  bool can_see_player(Vector const& eye) const;

  Player* get_nearest_player (Vector const& pos) const;
  Player* get_nearest_player (Rectf const& pos) const {
    return (get_nearest_player (get_anchor_pos (pos, ANCHOR_MIDDLE)));
  }

  std::vector<MovingObject*> get_nearby_objects (Vector const& center, float max_distance) const;

  Rectf get_active_region() const;

  int get_foremost_layer() const;

  /** returns the editor size (in tiles) of a sector */
  Size get_editor_size() const;

  /** resize all tilemaps with given size */
  void resize_sector(Size const& old_size, Size const& new_size, Size const& resize_offset);

  /** globally changes solid tilemaps' tile ids */
  void change_solid_tiles(uint32_t old_tile_id, uint32_t new_tile_id);

  /** set gravity throughout sector */
  void set_gravity(float gravity);
  float get_gravity() const;

  void set_init_script(std::string const& init_script) {
    m_init_script = init_script;
  }

  void run_script(std::string const& script, std::string const& sourcename);

  Camera& get_camera() const;
  std::vector<Player*> get_players() const;
  DisplayEffect& get_effect() const;

private:
  uint32_t collision_tile_attributes(Rectf const& dest, Vector const& mov) const;

  bool before_object_add(GameObject& object) override;
  void before_object_remove(GameObject& object) override;

  int calculate_foremost_layer() const;

  /** Convert tiles into their corresponding GameObjects (e.g.
      bonusblocks, add light to lava tiles) */
  void convert_tiles2gameobject();

private:
  /** Parent level containing this sector */
  Level& m_level;

  std::string m_name;

  bool m_fully_constructed;

  std::string m_init_script;

  int m_foremost_layer;

  std::unique_ptr<SquirrelEnvironment> m_squirrel_environment;
  std::unique_ptr<CollisionSystem> m_collision_system;

  float m_gravity;

private:
  Sector(Sector const&) = delete;
  Sector& operator=(Sector const&) = delete;
};

#endif

/* EOF */
