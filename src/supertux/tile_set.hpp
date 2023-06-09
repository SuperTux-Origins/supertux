//  SuperTux
//  Copyright (C) 2008 Matthias Braun <matze@braunis.de>
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

#ifndef HEADER_SUPERTUX_SUPERTUX_TILE_SET_HPP
#define HEADER_SUPERTUX_SUPERTUX_TILE_SET_HPP

#include <memory>
#include <stdint.h>
#include <string>

#include "math/fwd.hpp"
#include "video/color.hpp"
#include "video/surface_ptr.hpp"

class Canvas;
class DrawingContext;
class Tile;

class Tilegroup final
{
public:
  Tilegroup();

  bool developers_group = false;
  std::string name;
  std::vector<int> tiles;
};

class TileSet final
{
public:
  static std::unique_ptr<TileSet> from_file(std::string const& filename);

public:
  TileSet();
  ~TileSet();

  void add_tile(int id, std::unique_ptr<Tile> tile);

  /** Adds a group of tiles that haven't
      been assigned to any other group */
  void add_unassigned_tilegroup();

  void add_tilegroup(Tilegroup const& tilegroup);

  Tile const& get(const uint32_t id) const;

  uint32_t get_max_tileid() const {
    return static_cast<uint32_t>(m_tiles.size());
  }

  std::vector<Tilegroup> const& get_tilegroups() const {
    return m_tilegroups;
  }

  void print_debug_info(std::string const& filename);

private:
  std::vector<std::unique_ptr<Tile> > m_tiles;
  std::vector<Tilegroup> m_tilegroups;

private:
  TileSet(TileSet const&) = delete;
  TileSet& operator=(TileSet const&) = delete;
};

#endif

/* EOF */
