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

#ifndef HEADER_SUPERTUX_SUPERTUX_LEVEL_PARSER_HPP
#define HEADER_SUPERTUX_SUPERTUX_LEVEL_PARSER_HPP

#include <memory>
#include <string>

#include "util/reader_fwd.hpp"

class Level;

class LevelParser final
{
public:
  static std::unique_ptr<Level> from_stream(std::istream& stream, std::string const& context, bool worldmap, bool editable);
  static std::unique_ptr<Level> from_file(std::string const& filename, bool worldmap, bool editable);
  static std::unique_ptr<Level> from_nothing(std::string const& basedir);
  static std::unique_ptr<Level> from_nothing_worldmap(std::string const& basedir, std::string const& name);

  static std::string get_level_name(std::string const& filename);

private:
  LevelParser(Level& level, bool worldmap, bool editable);

  void load(ReaderDocument const& doc);
  void load(std::istream& stream, std::string const& context);
  void load(std::string const& filepath);
  void create(std::string const& filepath, std::string const& levelname);

private:
  Level& m_level;
  bool m_worldmap;
  bool m_editable;

private:
  LevelParser(LevelParser const&) = delete;
  LevelParser& operator=(LevelParser const&) = delete;
};

#endif

/* EOF */
