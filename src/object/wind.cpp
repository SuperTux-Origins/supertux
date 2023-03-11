//  SuperTux - Wind
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

#include "object/wind.hpp"

#include "badguy/badguy.hpp"
#include "control/controller.hpp"
#include "math/random.hpp"
#include "object/particles.hpp"
#include "object/player.hpp"
#include "object/rock.hpp"
#include "object/sprite_particle.hpp"
#include "sprite/sprite.hpp"
#include "sprite/sprite_manager.hpp"
#include "supertux/sector.hpp"
#include "util/reader_mapping.hpp"
#include "video/drawing_context.hpp"

Wind::Wind(ReaderMapping const& reader) :
  MovingObject(reader),
  ExposedObject<Wind, scripting::Wind>(this),
  blowing(),
  speed(0.0f, 0.0f),
  acceleration(),
  new_size(0.0f, 0.0f),
  dt_sec(0),
  affects_badguys(),
  affects_objects(),
  affects_player(),
  fancy_wind()
{
  float w,h;
  m_col.m_bbox.get_left() = reader.get("x", 0.0f);
  m_col.m_bbox.get_top() = reader.get("y", 0.0f);
  w = reader.get("width", 32.0f);
  h = reader.get("height", 32.0f);
  m_col.m_bbox.set_size(w, h);

  blowing = reader.get("blowing", true);

  speed.x = reader.get("speed-x", 0.0f);
  speed.y = reader.get("speed-y", 0.0f);

  acceleration = reader.get("acceleration", 100.0f);

  affects_badguys = reader.get("affects-badguys", false);
  affects_objects = reader.get("affects-objects", false);
  affects_player = reader.get("affects-player", true);

  fancy_wind = reader.get("fancy-wind", false);

  set_group(COLGROUP_TOUCHABLE);
}

void
Wind::update(float dt_sec_)
{
  dt_sec = dt_sec_;

  if (!blowing) return;
  if (m_col.m_bbox.get_width() <= 16 || m_col.m_bbox.get_height() <= 16) return;

  Vector ppos = Vector(graphicsRandom.randf(m_col.m_bbox.get_left()+8, m_col.m_bbox.get_right()-8), graphicsRandom.randf(m_col.m_bbox.get_top()+8, m_col.m_bbox.get_bottom()-8));
  Vector pspeed = Vector(graphicsRandom.randf(speed.x-20, speed.x+20), graphicsRandom.randf(speed.y-20, speed.y+20));

  // TODO: Rotate sprite rather than just use 2 different actions
  // Approx. 1 particle per tile
  if (graphicsRandom.randf(0.f, 100.f) < (m_col.m_bbox.get_width() / 32.f) * (m_col.m_bbox.get_height() / 32.f))
  {
    // emit a particle
	  if (fancy_wind)
    {
	    Sector::get().add<SpriteParticle>("images/particles/wind.sprite", (std::abs(speed.x) > std::abs(speed.y)) ? "default" : "flip", ppos, ANCHOR_MIDDLE, pspeed, Vector(0, 0), LAYER_BACKGROUNDTILES + 1); 
	  }
	  else
    {
	    Sector::get().add<Particles>(ppos, 44, 46, pspeed, Vector(0, 0), 1, Color(.4f, .4f, .4f), 3, .1f, LAYER_BACKGROUNDTILES + 1);
	  }
  }
}

void
Wind::draw(DrawingContext& context)
{
}

HitResponse
Wind::collision(GameObject& other, CollisionHit const& )
{
  if (!blowing) return ABORT_MOVE;

  auto player = dynamic_cast<Player*> (&other);
  if (player && affects_player)
  {
    player->override_velocity();
    if (!player->on_ground())
	  {
      player->add_velocity(speed * acceleration * dt_sec, speed);
    }
    else
    {
      if (player->get_controller().hold(Control::RIGHT) || player->get_controller().hold(Control::LEFT))
	    {
	      player->add_velocity(Vector(speed.x, 0) * acceleration * dt_sec, speed);
	    }
	    else
      {
	      //When on ground, get blown slightly differently, but the max speed is less than it would be otherwise seen as we take "friction" into account
	      player->add_velocity((Vector(speed.x, 0) * 0.1f) * (acceleration+1), (Vector(speed.x, speed.y) * 0.5f));
	    }
    }
  }

  auto badguy = dynamic_cast<BadGuy*>(&other);
  if (badguy && affects_badguys && badguy->can_be_affected_by_wind())
  {
    badguy->add_wind_velocity(speed * acceleration * dt_sec, speed);
  }

  auto rock = dynamic_cast<Rock*>(&other);
  if (rock && affects_objects)
  {
    rock->add_wind_velocity(speed * acceleration * dt_sec, speed);
  }

  return ABORT_MOVE;
}

void
Wind::start()
{
  blowing = true;
}

void
Wind::stop()
{
  blowing = false;
}

/* EOF */
