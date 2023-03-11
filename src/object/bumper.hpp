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

#ifndef HEADER_SUPERTUX_OBJECT_BUMPER_HPP
#define HEADER_SUPERTUX_OBJECT_BUMPER_HPP

#include "object/moving_sprite.hpp"
#include "supertux/physic.hpp"

class Player;

class Bumper final : public MovingSprite
{
public:
  Bumper(ReaderMapping const& reader);

  void update(float dt_sec) override;
  HitResponse collision(GameObject& other, CollisionHit const& hit) override;

  Physic physic;

private:
  bool left;

private:
  Bumper(Bumper const&) = delete;
  Bumper& operator=(Bumper const&) = delete;
};

#endif

/* EOF */
