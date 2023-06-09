//  Copyright (C) 2020 Daniel Ward <weluvgoatz@gmail.com>
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

#include "object/circleplatform.hpp"

#include "math/util.hpp"
#include "util/reader_mapping.hpp"

CirclePlatform::CirclePlatform(ReaderMapping const& reader) :
  MovingSprite(reader, "images/objects/platforms/icebridge1.png", LAYER_OBJECTS, COLGROUP_STATIC),
  start_position(m_col.m_bbox.p1()),
  angle(0.0),
  radius(),
  speed(),
  timer(),
  time(0.0)
{
  radius = reader.get("radius", 100.0f);
  speed = reader.get("speed", 2.0f);
  time = reader.get("time", 0.0f);

  m_col.m_bbox.set_pos(Vector(start_position.x + cosf(angle) * radius,
                              start_position.y + sinf(angle) * radius));
  initialize();
}

HitResponse
CirclePlatform::collision(GameObject& other, CollisionHit const& )
{
  return FORCE_MOVE;
}

void
CirclePlatform::update(float dt_sec)
{
  if (timer.get_timeleft() <= 0.00f)
  {
    time = 0;
    timer.stop();
    angle = fmodf(angle + dt_sec * speed, math::TAU);

    Vector newpos(start_position.x + cosf(angle) * radius,
                  start_position.y + sinf(angle) * radius);
    m_col.set_movement(newpos - get_pos());
    m_col.propagate_movement(newpos - get_pos());
  }
}

void
CirclePlatform::initialize()
{
  timer.start(time);
}
