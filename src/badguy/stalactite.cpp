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

#include "badguy/stalactite.hpp"

#include "audio/sound_manager.hpp"
#include "math/random.hpp"
#include "object/bullet.hpp"
#include "object/player.hpp"
#include "sprite/sprite.hpp"
#include "supertux/sector.hpp"
#include "util/file_system.hpp"
#include "util/reader_mapping.hpp"

static const int SHAKE_RANGE_X = 40;
static const float SHAKE_TIME = .8f;
static const float SHAKE_RANGE_Y = 400;

const std::vector<std::string> Stalactite::s_sprites = { "stalactite.sprite", "rock_stalactite.sprite" };

Stalactite::StalactiteType
Stalactite::StalactiteType_from_string(std::string const& type_string)
{
  if (type_string == "ice")
    return StalactiteType::ICE;
  else if (type_string == "rock")
    return StalactiteType::ROCK;
  else
    throw std::exception();
}

Stalactite::Stalactite(ReaderMapping const& mapping) :
  BadGuy(mapping, "images/creatures/stalactite/stalactite.sprite", LAYER_TILES - 1),
  m_type(),
  timer(),
  state(STALACTITE_HANGING),
  shake_delta(0.0f, 0.0f)
{
  std::string type;
  type = mapping.get("type", std::string());
  try
  {
    m_type = StalactiteType_from_string(type);
  }
  catch (std::exception&)
  {
    if (type.empty())
    {
      log_warning("No stalactite type set, setting to ice.");
    }
    else
    {
      log_warning("Unknown type of stalactite:{}, setting to ice.", type);
    }

    m_type = StalactiteType::ICE;
  }

  m_countMe = false;
  set_colgroup_active(COLGROUP_TOUCHABLE);
  SoundManager::current()->preload("sounds/cracking.wav");
  SoundManager::current()->preload("sounds/sizzle.ogg");
  SoundManager::current()->preload("sounds/icecrash.ogg");
}

void
Stalactite::active_update(float dt_sec)
{
  if (state == STALACTITE_HANGING) {
    auto player = get_nearest_player();
    if (player && !player->get_ghost_mode()) {
      if (player->get_bbox().get_right() > m_col.m_bbox.get_left() - SHAKE_RANGE_X
         && player->get_bbox().get_left() < m_col.m_bbox.get_right() + SHAKE_RANGE_X
         && player->get_bbox().get_bottom() > m_col.m_bbox.get_top()
         && player->get_bbox().get_top() < m_col.m_bbox.get_bottom() + SHAKE_RANGE_Y
         && Sector::get().can_see_player(m_col.m_bbox.get_middle())) {
        timer.start(SHAKE_TIME);
        state = STALACTITE_SHAKING;
        SoundManager::current()->play("sounds/cracking.wav", get_pos());
      }
    }
  } else if (state == STALACTITE_SHAKING) {
    shake_delta = Vector(static_cast<float>(graphicsRandom.rand(-3, 3)), 0.0f);
    if (timer.check()) {
      state = STALACTITE_FALLING;
      m_physic.enable_gravity(true);
      set_colgroup_active(COLGROUP_MOVING);
    }
  } else if (state == STALACTITE_FALLING) {
    m_col.set_movement(m_physic.get_movement(dt_sec));
  }
}

void
Stalactite::squish()
{
  state = STALACTITE_SQUISHED;
  m_physic.enable_gravity(true);
  m_physic.set_velocity_x(0);
  m_physic.set_velocity_y(0);
  set_state(STATE_SQUISHED);
  m_sprite->set_action("squished");
  SoundManager::current()->play("sounds/icecrash.ogg", get_pos());
  set_group(COLGROUP_MOVING_ONLY_STATIC);
  run_dead_script();
}

void
Stalactite::collision_solid(CollisionHit const& hit)
{
  if (state == STALACTITE_FALLING) {
    if (hit.bottom) squish();
  }
  if (state == STALACTITE_SQUISHED) {
    m_physic.set_velocity_y(0);
  }
}

HitResponse
Stalactite::collision_player(Player& player, CollisionHit const& )
{
  if (state != STALACTITE_SQUISHED) {
    player.kill(false);
  }

  return FORCE_MOVE;
}

HitResponse
Stalactite::collision_badguy(BadGuy& other, CollisionHit const& hit)
{
  if (state == STALACTITE_SQUISHED) return FORCE_MOVE;

  // ignore other Stalactites
  if (dynamic_cast<Stalactite*>(&other)) return FORCE_MOVE;

  if (state != STALACTITE_FALLING) return BadGuy::collision_badguy(other, hit);

  if (other.is_freezable() && m_type != StalactiteType::ROCK) {
    other.freeze();
  } else {
    other.kill_fall();
  }

  return FORCE_MOVE;
}

HitResponse
Stalactite::collision_bullet(Bullet& bullet, CollisionHit const& hit)
{
  if (m_type == StalactiteType::ROCK)
  {
    bullet.ricochet(*this, hit);
  }
  else if (state == STALACTITE_HANGING)
  {
    timer.start(SHAKE_TIME);
    state = STALACTITE_SHAKING;
    bullet.remove_me();
    if (bullet.get_type() == FIRE_BONUS)
      SoundManager::current()->play("sounds/sizzle.ogg", get_pos());
    SoundManager::current()->play("sounds/cracking.wav", get_pos());
  }

  return FORCE_MOVE;
}

void
Stalactite::kill_fall()
{
}

void
Stalactite::draw(DrawingContext& context)
{
  if (get_state() == STATE_INIT || get_state() == STATE_INACTIVE)
    return;

  if (state == STALACTITE_SQUISHED) {
    m_sprite->draw(context.color(), get_pos(), LAYER_OBJECTS);
  } else if (state == STALACTITE_SHAKING) {
    m_sprite->draw(context.color(), get_pos() + shake_delta, m_layer, m_flip);
  } else {
    m_sprite->draw(context.color(), get_pos(), m_layer, m_flip);
  }
}

void
Stalactite::deactivate()
{
  if (state != STALACTITE_HANGING)
    remove_me();
}

/* EOF */
