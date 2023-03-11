//  SuperTux - WalkingBadguy
//  Copyright (C) 2006 Christoph Sommer <christoph.sommer@2006.expires.deltadevelopment.de>
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

#ifndef HEADER_SUPERTUX_BADGUY_WALKING_BADGUY_HPP
#define HEADER_SUPERTUX_BADGUY_WALKING_BADGUY_HPP

#include "badguy/badguy.hpp"

class Timer;

/** Base class for Badguys that walk on the floor. */
class WalkingBadguy : public BadGuy
{
public:
  WalkingBadguy(Vector const& pos,
                std::string const& sprite_name,
                std::string const& walk_left_action,
                std::string const& walk_right_action,
                int layer = LAYER_OBJECTS,
                std::string const& light_sprite_name = "images/objects/lightmap_light/lightmap_light-medium.sprite");
  WalkingBadguy(Vector const& pos, Direction direction,
                std::string const& sprite_name,
                std::string const& walk_left_action,
                std::string const& walk_right_action,
                int layer = LAYER_OBJECTS,
                std::string const& light_sprite_name = "images/objects/lightmap_light/lightmap_light-medium.sprite");
  WalkingBadguy(ReaderMapping const& reader,
                std::string const& sprite_name,
                std::string const& walk_left_action,
                std::string const& walk_right_action,
                int layer = LAYER_OBJECTS,
                std::string const& light_sprite_name = "images/objects/lightmap_light/lightmap_light-medium.sprite");

  void initialize() override;
  void active_update(float dt_sec) override;
  void collision_solid(CollisionHit const& hit) override;
  HitResponse collision_badguy(BadGuy& badguy, CollisionHit const& hit) override;
  void freeze() override;
  void unfreeze(bool melt = true) override;

  void active_update(float dt_sec, float target_velocity, float modifier = 1.f);

  float get_velocity_x() const { return m_physic.get_velocity_x(); }
  float get_velocity_y() const { return m_physic.get_velocity_y(); }
  void set_velocity_y(float vy);

  /** Adds velocity to the badguy (be careful when using this) */
  void add_velocity(Vector const& velocity);

  float get_walk_speed() const { return walk_speed; }
  void set_walk_speed (float);
  bool is_active() const { return BadGuy::is_active(); }

protected:
  void turn_around();

protected:
  std::string walk_left_action;
  std::string walk_right_action;
  float walk_speed;
  int max_drop_height; /**< Maximum height of drop before we will turn around, or -1 to just drop from any ledge */
  Timer turn_around_timer;
  int turn_around_counter; /**< counts number of turns since turn_around_timer was started */

private:
  WalkingBadguy(WalkingBadguy const&) = delete;
  WalkingBadguy& operator=(WalkingBadguy const&) = delete;
};

#endif

/* EOF */
