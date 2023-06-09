//  SuperTux
//  Copyright (C) 2013 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_SUPERTUX_SUPERTUX_GAME_MANAGER_HPP
#define HEADER_SUPERTUX_SUPERTUX_GAME_MANAGER_HPP

#include <memory>
#include <optional>
#include <string>

#include "math/vector.hpp"
#include "util/currenton.hpp"

class Savegame;
class World;

class GameManager final : public Currenton<GameManager>
{
public:
  GameManager();

  void start_worldmap(World const& world, std::string const& spawnpoint = "", std::string const& worldmap_filename = "");
  void start_level(World const& world, std::string const& level_filename,
                   const std::optional<std::pair<std::string, Vector>>& start_pos = std::nullopt);

  bool load_next_worldmap();
  void set_next_worldmap(std::string const& worldmap, std::string const&spawnpoint);

private:
  std::unique_ptr<Savegame> m_savegame;

  std::string m_next_worldmap;
  std::string m_next_spawnpoint;

private:
  GameManager(GameManager const&) = delete;
  GameManager& operator=(GameManager const&) = delete;
};

#endif

/* EOF */
