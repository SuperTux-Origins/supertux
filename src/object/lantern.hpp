//  SuperTux - Lantern
//  Copyright (C) 2006 Wolfgang Becker <uafr@gmx.de>
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

#ifndef HEADER_SUPERTUX_OBJECT_LANTERN_HPP
#define HEADER_SUPERTUX_OBJECT_LANTERN_HPP

#include "object/rock.hpp"

/** Lantern. A portable Light Source. */
class Lantern final : public Rock
{
public:
  Lantern(Vector const& pos);
  Lantern(ReaderMapping const& reader);

  void draw(DrawingContext& context) override;

  HitResponse collision(GameObject& other, CollisionHit const& hit) override;

  void grab(MovingObject& object, Vector const& pos, Direction dir) override;
  void ungrab(MovingObject& object, Direction dir) override;


  /** returns true if lamp is currently open */
  bool is_open() const;

  /** returns the lamp's color */
  Color get_color() const { return lightcolor; }
  void add_color(Color const& c);

private:
  Color lightcolor;
  SpritePtr lightsprite;
  void updateColor();

private:
  Lantern(Lantern const&) = delete;
  Lantern& operator=(Lantern const&) = delete;
};

#endif

/* EOF */
