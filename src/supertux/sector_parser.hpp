//  SuperTux
//  Copyright (C) 2015 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_SUPERTUX_SUPERTUX_SECTOR_PARSER_HPP
#define HEADER_SUPERTUX_SUPERTUX_SECTOR_PARSER_HPP

#include <memory>
#include <string>

#include "util/reader_mapping.hpp"

class GameObject;
class Level;
class Sector;

class SectorParser final
{
public:
  static std::unique_ptr<Sector> from_reader(Level& level, ReaderMapping const& sector);
  static std::unique_ptr<Sector> from_nothing(Level& level);

private:
  SectorParser(Sector& sector);

  void parse(ReaderMapping const& sector);
  void create_sector();
  std::unique_ptr<GameObject> parse_object(std::string const& name_, ReaderMapping const& reader);

private:
  Sector& m_sector;

private:
  SectorParser(SectorParser const&) = delete;
  SectorParser& operator=(SectorParser const&) = delete;
};

#endif

/* EOF */
