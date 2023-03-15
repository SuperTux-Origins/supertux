//  SuperTux
//  Copyright (C) 2008 Wolfgang Becker <uafr@gmx.de>
//  Copyright (C) 2010 Florian Forster <supertux at octo.it>
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

#include "badguy/owl.hpp"

#include "audio/sound_manager.hpp"
#include "object/player.hpp"
#include "supertux/game_object_factory.hpp"
#include "supertux/sector.hpp"
#include "util/reader_mapping.hpp"

namespace {

const float FLYING_SPEED = 120.0f;
const float ACTIVATION_DISTANCE = 128.0f;

} // namespace

std::vector<std::string> Owl::s_portable_objects;

Owl::Owl(ReaderMapping const& reader) :
  BadGuy(reader, "images/creatures/owl/owl.sprite", LAYER_OBJECTS + 1),
  carried_obj_name(),
  carried_object(nullptr)
{
  carried_obj_name = reader.get("carry", std::string("skydive"));
  set_action (m_dir == Direction::LEFT ? "left" : "right", /* loops = */ -1);
}

void
Owl::initialize()
{
  m_physic.set_velocity_x(m_dir == Direction::LEFT ? -FLYING_SPEED : FLYING_SPEED);
  m_physic.enable_gravity(false);
  m_sprite->set_action(m_dir);

  auto game_object = GameObjectFactory::instance().create(carried_obj_name, get_pos(), m_dir);
  if (game_object == nullptr)
  {
    log_fatal("Creating \"{}\" object failed.", carried_obj_name);
  }
  else
  {
    carried_object = dynamic_cast<Portable*>(game_object.get());
    if (carried_object == nullptr)
    {
      log_warning("Object is not portable: {}", carried_obj_name);
    }
    else
    {
      Sector::get().add_object(std::move(game_object));
    }
  }
}

bool
Owl::is_above_player() const
{
  auto player = Sector::get().get_nearest_player(m_col.m_bbox);
  if (!player)
    return false;

  // Let go of carried objects a short while *before* Tux is below us. This
  // makes it more likely that we'll hit him.
  float x_offset = (m_dir == Direction::LEFT) ? ACTIVATION_DISTANCE : -ACTIVATION_DISTANCE;

  Rectf const& player_bbox = player->get_bbox();

  return ((player_bbox.get_top() >= m_col.m_bbox.get_bottom()) /* player is below us */
          && ((player_bbox.get_right() + x_offset) > m_col.m_bbox.get_left())
          && ((player_bbox.get_left() + x_offset) < m_col.m_bbox.get_right()));
}

void
Owl::active_update (float dt_sec)
{
  BadGuy::active_update (dt_sec);

  if (m_frozen)
    return;

  if (carried_object != nullptr) {
    if (!is_above_player ()) {
      Vector obj_pos = get_anchor_pos(m_col.m_bbox, ANCHOR_BOTTOM);
      obj_pos.x -= 16.f; /* FIXME: Actually do use the half width of the carried object here. */
      obj_pos.y += 3.f; /* Move a little away from the hitbox (the body). Looks nicer. */

      //To drop enemie before leave the screen
      if (obj_pos.x<=16 || obj_pos.x+16>=Sector::get().get_width()){
        carried_object->ungrab (*this, m_dir);
        carried_object = nullptr;
      }

     else
        carried_object->grab (*this, obj_pos, m_dir);
    }
    else { /* if (is_above_player) */
      carried_object->ungrab (*this, m_dir);
      carried_object = nullptr;
    }
  }
}

bool
Owl::collision_squished(GameObject& object)
{
  if (m_frozen)
    return BadGuy::collision_squished(object);

  auto player = Sector::get().get_nearest_player(m_col.m_bbox);
  if (player)
    player->bounce (*this);

  if (carried_object != nullptr) {
    carried_object->ungrab (*this, m_dir);
    carried_object = nullptr;
  }

  kill_fall ();
  return true;
}

void
Owl::kill_fall()
{
  if (!m_frozen)
  {
    SoundManager::current()->play("sounds/fall.wav", get_pos());
    m_physic.set_velocity_y(0);
    m_physic.set_acceleration_y(0);
    m_physic.enable_gravity(true);
    set_state(STATE_FALLING);
  }
  else
    BadGuy::kill_fall();

  if (carried_object != nullptr) {
    carried_object->ungrab (*this, m_dir);
    carried_object = nullptr;
  }

  // start dead-script
  run_dead_script();
}

void
Owl::freeze()
{
  if (carried_object != nullptr) {
    carried_object->ungrab (*this, m_dir);
    carried_object = nullptr;
  }
  m_physic.enable_gravity(true);
  BadGuy::freeze();
}

void
Owl::unfreeze(bool melt)
{
  BadGuy::unfreeze(melt);
  m_physic.set_velocity_x(m_dir == Direction::LEFT ? -FLYING_SPEED : FLYING_SPEED);
  m_physic.enable_gravity(false);
  m_sprite->set_action(m_dir);
}

bool
Owl::is_freezable() const
{
  return true;
}

void
Owl::collision_solid(CollisionHit const& hit)
{
  if (m_frozen)
  {
    BadGuy::collision_solid(hit);
    return;
  }
  if (hit.top || hit.bottom) {
    m_physic.set_velocity_y(0);
  } else if (hit.left || hit.right) {
    if (m_dir == Direction::LEFT) {
      set_action ("right", /* loops = */ -1);
      m_dir = Direction::RIGHT;
      m_physic.set_velocity_x (FLYING_SPEED);
    }
    else {
      set_action ("left", /* loops = */ -1);
      m_dir = Direction::LEFT;
      m_physic.set_velocity_x (-FLYING_SPEED);
    }
  }
}

void
Owl::ignite()
{
  if (carried_object != nullptr) {
    carried_object->ungrab (*this, m_dir);
    carried_object = nullptr;
  }
  BadGuy::ignite();
}

/* EOF */
