//  SuperTux
//  Copyright (C) 2009 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_SUPERTUX_OBJECT_BRICK_HPP
#define HEADER_SUPERTUX_OBJECT_BRICK_HPP

#include "badguy/crusher.hpp"
#include "object/block.hpp"

class Brick : public Block
{
public:
  Brick(Vector const& pos, int data, std::string const& spriteName);
  Brick(ReaderMapping const& mapping, std::string const& spriteName = "images/objects/bonus_block/brick.sprite");

  HitResponse collision(GameObject& other, CollisionHit const& hit) override;


  void try_break(Player* player);
  void break_for_crusher(Crusher* icecrusher);

protected:
  void hit(Player& player) override;

private:
  bool m_breakable;
  int m_coin_counter;

private:
  Brick(Brick const&) = delete;
  Brick& operator=(Brick const&) = delete;
};

class HeavyBrick : public Brick
{
public:
  HeavyBrick(Vector const& pos, int data, std::string const& spriteName);
  HeavyBrick(ReaderMapping const& mapping);

  HitResponse collision(GameObject& other, CollisionHit const& hit) override;

private:
  void ricochet(GameObject* collider);

protected:
  void hit(Player& player) override;
};

#endif

/* EOF */
