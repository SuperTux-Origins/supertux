//  SuperTux
//  Copyright (C) 2008 Matthias Braun <matze@braunis.de>,
//                     Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_SUPERTUX_SUPERTUX_TILE_SET_PARSER_HPP
#define HEADER_SUPERTUX_SUPERTUX_TILE_SET_PARSER_HPP

#include <string>
#include <vector>
#include <optional>

#include "math/rect.hpp"
#include "supertux/tile.hpp"
#include "util/reader_fwd.hpp"

class TileSet;

class TileSetParser final
{
private:
  TileSet&    m_tileset;
  std::string m_filename;
  std::string m_tiles_path;

public:
  TileSetParser(TileSet& tileset, std::string const& filename);

  void parse(int32_t start = 0, int32_t end = 0, int32_t offset = 0, bool imported = false);

private:
  void parse_tile(ReaderMapping const& reader, int32_t min, int32_t max, int32_t offset);
  void parse_tiles(ReaderMapping const& reader, int32_t min, int32_t max, int32_t offset);
  std::vector<SurfacePtr> parse_imagespecs(ReaderMapping const& cur,
                                           std::optional<Rect> const& region = std::nullopt) const;

private:
  TileSetParser(TileSetParser const&) = delete;
  TileSetParser& operator=(TileSetParser const&) = delete;
};

#endif

/* EOF */
