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

#ifndef HEADER_SUPERTUX_OBJECT_BULLET_HPP
#define HEADER_SUPERTUX_OBJECT_BULLET_HPP

#include "sprite/sprite_ptr.hpp"
#include "supertux/direction.hpp"
#include "supertux/moving_object.hpp"
#include "supertux/physic.hpp"
#include "supertux/player_status.hpp"
#include "video/layer.hpp"

class Player;

class Bullet final : public MovingObject
{
public:
  Bullet(Vector const& pos, Vector const& xm, Direction dir, BonusType type, Player& player);

  void update(float dt_sec) override;
  void draw(DrawingContext& context) override;
  void collision_solid(CollisionHit const& hit) override;
  HitResponse collision(GameObject& other, CollisionHit const& hit) override;
  bool is_saveable() const override { return false; }

  int get_layer() const override { return LAYER_OBJECTS; }

  /** Makes bullet bounce off an object (that got hit). To be called
      by the collision handler of that object. Note that the @c hit
      parameter is filled in as perceived by the object, not by the
      bullet. */
  void ricochet(GameObject& other, CollisionHit const& hit);

  BonusType get_type() const { return type; }

  Player& get_player() const { return m_player; }

private:
  Player& m_player;
  Physic physic;
  int life_count;
  SpritePtr sprite;
  SpritePtr lightsprite;
  BonusType type;

private:
  Bullet(Bullet const&) = delete;
  Bullet& operator=(Bullet const&) = delete;
};

#endif

/* EOF */
