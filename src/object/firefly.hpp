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

#ifndef HEADER_SUPERTUX_OBJECT_FIREFLY_HPP
#define HEADER_SUPERTUX_OBJECT_FIREFLY_HPP

#include "object/moving_sprite.hpp"
#include "sprite/sprite_ptr.hpp"

/**
 * A Firefly: When tux touches it, it begins buzzing and you will respawn at this
 * position.
 */
class Firefly final : public MovingSprite
{
public:
  Firefly(ReaderMapping const& mapping);

  void draw(DrawingContext& context) override;

  HitResponse collision(GameObject& other, CollisionHit const& hit) override;

  void on_flip(float height) override;

private:
  void reactivate();

private:
  SpritePtr m_sprite_light;
  bool activated;
  Vector initial_position; /**< position as in level file. This is where Tux will have to respawn, as the level is reset every time */

private:
  Firefly(Firefly const&) = delete;
  Firefly& operator=(Firefly const&) = delete;
};

#endif

/* EOF */
