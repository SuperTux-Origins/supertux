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

#ifndef HEADER_SUPERTUX_OBJECT_INVISIBLE_WALL_HPP
#define HEADER_SUPERTUX_OBJECT_INVISIBLE_WALL_HPP

#include "supertux/moving_object.hpp"

#include "video/layer.hpp"

/** A tile that starts falling down if tux stands to long on it */
class InvisibleWall final : public MovingObject
{
public:
  InvisibleWall(ReaderMapping const& mapping);

  HitResponse collision(GameObject& other, CollisionHit const& hit) override;
  void draw(DrawingContext& context) override;


  bool has_variable_size() const override { return true; }

  int get_layer() const override { return LAYER_OBJECTS; }

private:
  void update(float dt_sec) override;

private:
  float width;
  float height;

private:
  InvisibleWall(InvisibleWall const&) = delete;
  InvisibleWall& operator=(InvisibleWall const&) = delete;
};

#endif

/* EOF */
