//  SuperTux
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

#ifndef HEADER_SUPERTUX_OBJECT_SCRIPTED_OBJECT_HPP
#define HEADER_SUPERTUX_OBJECT_SCRIPTED_OBJECT_HPP

#include "object/moving_sprite.hpp"
#include "scripting/scripted_object.hpp"
#include "squirrel/exposed_object.hpp"
#include "supertux/physic.hpp"

class ScriptedObject final :
  public MovingSprite,
  public ExposedObject<ScriptedObject, scripting::ScriptedObject>
{
public:
  ScriptedObject(ReaderMapping const& mapping);

  void update(float dt_sec) override;
  void draw(DrawingContext& context) override;

  void collision_solid(CollisionHit const& hit) override;
  HitResponse collision(GameObject& other, CollisionHit const& hit) override;

  // --- scripting Interface stuff ---
  void set_action(std::string const& animation);
  std::string get_action() const;

  void move(float x, float y);
  float get_pos_x() const;
  float get_pos_y() const;
  void set_velocity(float x, float y);
  float get_velocity_x() const;
  float get_velocity_y() const;
  void set_visible(bool visible);
  bool is_visible() const;
  void set_solid(bool solid);
  bool is_solid() const;
  void enable_gravity(bool f);
  bool gravity_enabled() const;

private:
  Physic physic;
  bool solid;
  bool physic_enabled;
  bool visible;
  std::string hit_script;
  bool new_vel_set;
  Vector new_vel;
  Vector new_size;

private:
  ScriptedObject(ScriptedObject const&) = delete;
  ScriptedObject& operator=(ScriptedObject const&) = delete;
};

#endif

/* EOF */
