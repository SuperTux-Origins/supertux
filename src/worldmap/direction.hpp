//  SuperTux - Worldmap Direction
//  Copyright (C) 2006 Christoph Sommer <christoph.sommer@2006.expires.deltadevelopment.de>
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

#ifndef HEADER_SUPERTUX_WORLDMAP_DIRECTION_HPP
#define HEADER_SUPERTUX_WORLDMAP_DIRECTION_HPP

#include <format>
#include <memory>
#include <string>
#include <string_view>

class ObjectOption;

namespace worldmap {

enum class Direction { NONE, WEST, EAST, NORTH, SOUTH };

Direction reverse_dir(Direction direction);
Direction string_to_direction(std::string const& directory);
std::string direction_to_string(Direction direction);

} // namespace worldmap

template<>
struct std::formatter<worldmap::Direction, char> : std::formatter<std::string_view>
{
  auto format(worldmap::Direction dir, auto& ctx) const
  {
    using worldmap::Direction;
    std::string_view s;
    switch (dir) {
      case Direction::WEST:  s = "west";  break;
      case Direction::EAST:  s = "east";  break;
      case Direction::NORTH: s = "north"; break;
      case Direction::SOUTH: s = "south"; break;
      case Direction::NONE:  s = "none";  break;
      default:               s = "unknown"; break;
    }
    return std::formatter<std::string_view>::format(s, ctx);
  }
};

#endif

/* EOF */
