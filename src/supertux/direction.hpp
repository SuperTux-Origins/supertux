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

#ifndef HEADER_SUPERTUX_SUPERTUX_DIRECTION_HPP
#define HEADER_SUPERTUX_SUPERTUX_DIRECTION_HPP

#include <format>
#include <iostream>
#include <string>
#include <string_view>

class ObjectOption;

enum class Direction { AUTO, LEFT, RIGHT, UP, DOWN };

std::ostream& operator<<(std::ostream& o, Direction const& dir);

std::string dir_to_string(Direction const& dir);
Direction string_to_dir(std::string const& dir_str);

/** std::format support (logmich uses std::vformat). */
template<>
struct std::formatter<Direction> : std::formatter<std::string_view>
{
  auto format(Direction dir, auto& ctx) const
  {
    std::string_view s;
    switch (dir) {
      case Direction::LEFT:  s = "left";  break;
      case Direction::RIGHT: s = "right"; break;
      case Direction::UP:    s = "up";    break;
      case Direction::DOWN:  s = "down";  break;
      case Direction::AUTO:  s = "auto";  break;
      default:               s = "unknown"; break;
    }
    return std::formatter<std::string_view>::format(s, ctx);
  }
};

#endif

/* EOF */
