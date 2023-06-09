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

#ifndef HEADER_SUPERTUX_OBJECT_FLOATING_TEXT_HPP
#define HEADER_SUPERTUX_OBJECT_FLOATING_TEXT_HPP

#include "math/vector.hpp"
#include "supertux/game_object.hpp"
#include "supertux/timer.hpp"
#include "video/color.hpp"

class FloatingText final : public GameObject
{
  static Color text_color;
public:
  FloatingText(Vector const& pos, std::string const& text_);
  FloatingText(Vector const& pos, int s);  // use this for score, for instance
  bool is_saveable() const override {
    return false;
  }

  void update(float dt_sec) override;
  void draw(DrawingContext& context) override;

private:
  Vector position;
  std::string text;
  Timer timer;
};

#endif

/* EOF */
