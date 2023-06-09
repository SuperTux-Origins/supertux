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

#ifndef HEADER_SUPERTUX_OBJECT_CIRCLEPLATFORM_HPP
#define HEADER_SUPERTUX_OBJECT_CIRCLEPLATFORM_HPP

#include "object/moving_sprite.hpp"
#include "supertux/timer.hpp"

class CirclePlatform : public MovingSprite
{
public:
  CirclePlatform(ReaderMapping const& reader);

  HitResponse collision(GameObject& other, CollisionHit const& hit) override;

  void update(float dt_sec) override;

private:
  virtual void initialize();

protected:
  Vector start_position;
  float angle;
  float radius;
  float speed;

  Timer timer;
  float time;

private:
  CirclePlatform(CirclePlatform const&) = delete;
  CirclePlatform& operator=(CirclePlatform const&) = delete;
};

#endif

/* EOF */
