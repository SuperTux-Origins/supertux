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

#ifndef HEADER_SUPERTUX_OBJECT_INVISIBLE_BLOCK_HPP
#define HEADER_SUPERTUX_OBJECT_INVISIBLE_BLOCK_HPP

#include "object/block.hpp"

class InvisibleBlock final : public Block
{
public:
  InvisibleBlock(Vector const& pos);
  InvisibleBlock(ReaderMapping const& mapping);

  static std::string class_name() { return "invisible_block"; }
  std::string get_class_name() const override { return class_name(); }
  static std::string display_name() { return _("Invisible Block"); }
  std::string get_display_name() const override { return display_name(); }

  void draw(DrawingContext& context) override;
  bool collides(GameObject& other, CollisionHit const& hit) const override;
  HitResponse collision(GameObject& other, CollisionHit const& hit) override;

private:
  void hit(Player& player) override;

private:
  bool visible;

private:
  InvisibleBlock(InvisibleBlock const&) = delete;
  InvisibleBlock& operator=(InvisibleBlock const&) = delete;
};

#endif

/* EOF */
