//  MoleRock - Rock thrown by "Mole" Badguy
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

#include "badguy/mole_rock.hpp"


#include "audio/sound_manager.hpp"
#include "math/random.hpp"

MoleRock::MoleRock(ReaderMapping const& reader) :
  BadGuy(reader, "images/creatures/mole/mole_rock.sprite", LAYER_TILES - 2),
  parent(nullptr),
  initial_velocity(Vector(0, -400))
{
  m_physic.enable_gravity(true);
  m_countMe = false;
  SoundManager::current()->preload("sounds/darthit.wav");
  SoundManager::current()->preload("sounds/stomp.wav");
}

MoleRock::MoleRock(Vector const& pos, Vector const& velocity, BadGuy const* parent_ = nullptr) :
  BadGuy(pos, Direction::LEFT, "images/creatures/mole/mole_rock.sprite", LAYER_TILES - 2),
  parent(parent_),
  initial_velocity(velocity)
{
  m_physic.enable_gravity(true);
  m_countMe = false;
  SoundManager::current()->preload("sounds/darthit.wav");
  SoundManager::current()->preload("sounds/stomp.wav");
}

bool
MoleRock::updatePointers(GameObject const* from_object, GameObject* to_object)
{
  if (from_object == parent) {
    parent = dynamic_cast<MoleRock*>(to_object);
    return true;
  }
  return false;
}

void
MoleRock::initialize()
{
  m_physic.set_velocity(initial_velocity);

  // Randomly select a rock size to display.
  m_sprite->set_action(graphicsRandom.rand(2) == 0 ? "small" : "medium");
}

void
MoleRock::deactivate()
{
  remove_me();
}

void
MoleRock::active_update(float dt_sec)
{
  BadGuy::active_update(dt_sec);
}

void
MoleRock::collision_solid(CollisionHit const& )
{
  SoundManager::current()->play("sounds/darthit.wav", get_pos());
  remove_me();
}

HitResponse
MoleRock::collision_badguy(BadGuy& badguy, CollisionHit const& )
{
  // ignore collisions with parent
  if (&badguy == parent) {
    return FORCE_MOVE;
  }
  SoundManager::current()->play("sounds/stomp.wav", get_pos());
  remove_me();
  badguy.kill_fall();
  return ABORT_MOVE;
}

HitResponse
MoleRock::collision_player(Player& player, CollisionHit const& hit)
{
  SoundManager::current()->play("sounds/stomp.wav", get_pos());
  remove_me();
  return BadGuy::collision_player(player, hit);
}

bool
MoleRock::is_flammable() const
{
  return false;
}

/* EOF */
