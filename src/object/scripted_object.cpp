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

#include "object/scripted_object.hpp"

#include "math/random.hpp"
#include "object/player.hpp"
#include "supertux/sector.hpp"
#include "util/reader.hpp"
#include "util/reader_mapping.hpp"

ScriptedObject::ScriptedObject(ReaderMapping const& mapping) :
  MovingSprite(mapping, "images/objects/bonus_block/brick.sprite", LAYER_OBJECTS, COLGROUP_MOVING_STATIC),
  ExposedObject<ScriptedObject, scripting::ScriptedObject>(this),
  physic(),
  solid(),
  physic_enabled(),
  visible(),
  hit_script(),
  new_vel_set(false),
  new_vel(0.0f, 0.0f),
  new_size(0.0f, 0.0f)
{
  m_default_sprite_name = {};

  if (m_name.empty()) {
    m_name = "unnamed" + std::to_string(graphicsRandom.rand());
    log_warning("Scripted object must have a name specified, setting to: {}", m_name);
  }

  solid = mapping.get("solid", true);
  physic_enabled = mapping.get("physic-enabled", true);
  visible = mapping.get("visible", true);
  hit_script = mapping.get("hit-script", std::string());
  m_layer = reader_get_layer(mapping, LAYER_OBJECTS);
  if ( solid ){
    set_group( COLGROUP_MOVING_STATIC );
  } else {
    set_group( COLGROUP_DISABLED );
  }
}

void
ScriptedObject::move(float x, float y)
{
  m_col.m_bbox.move(Vector(x, y));
}

float
ScriptedObject::get_pos_x() const
{
  return get_pos().x;
}

float
ScriptedObject::get_pos_y() const
{
  return get_pos().y;
}

void
ScriptedObject::set_velocity(float x, float y)
{
  new_vel = Vector(x, y);
  new_vel_set = true;
}

float
ScriptedObject::get_velocity_x() const
{
  return physic.get_velocity_x();
}

float
ScriptedObject::get_velocity_y() const
{
  return physic.get_velocity_y();
}

void
ScriptedObject::set_visible(bool visible_)
{
  visible = visible_;
}

bool
ScriptedObject::is_visible() const
{
  return visible;
}

void
ScriptedObject::set_solid(bool solid_)
{
  solid = solid_;
  if ( solid ){
    set_group( COLGROUP_MOVING_STATIC );
  } else {
    set_group( COLGROUP_DISABLED );
  }
}

bool
ScriptedObject::is_solid() const
{
  return solid;
}

bool
ScriptedObject::gravity_enabled() const
{
	return physic.gravity_enabled();
}

void
ScriptedObject::enable_gravity(bool f)
{
	physic.enable_gravity(f);
}

void
ScriptedObject::set_action(std::string const& animation)
{
  m_sprite->set_action(animation);
}

std::string
ScriptedObject::get_action() const
{
  return m_sprite->get_action();
}

void
ScriptedObject::update(float dt_sec)
{
  if (!physic_enabled)
    return;

  if (new_vel_set) {
    physic.set_velocity(new_vel.x, new_vel.y);
    new_vel_set = false;
  }
  m_col.set_movement(physic.get_movement(dt_sec));
}

void
ScriptedObject::draw(DrawingContext& context)
{
  if (!visible) return;

  MovingSprite::draw(context);
}

void
ScriptedObject::collision_solid(CollisionHit const& hit)
{
  if (!physic_enabled)
    return;

  if (hit.bottom) {
    if (physic.get_velocity_y() > 0)
      physic.set_velocity_y(0);
  } else if (hit.top) {
    physic.set_velocity_y(.1f);
  }

  if (hit.left || hit.right) {
    physic.set_velocity_x(0);
  }
}

HitResponse
ScriptedObject::collision(GameObject& other, CollisionHit const& )
{
  auto player = dynamic_cast<Player*> (&other);
  if (player && !hit_script.empty()) {
    Sector::get().run_script(hit_script, "hit-script");
  }

  return FORCE_MOVE;
}

/* EOF */
