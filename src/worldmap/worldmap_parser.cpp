//  SuperTux
//  Copyright (C) 2018 Ingo Ruhnke <grumbel@gmail.com>
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

#include "worldmap/worldmap_parser.hpp"

#include <physfs.h>


#include "object/ambient_light.hpp"
#include "object/background.hpp"
#include "object/decal.hpp"
#include "object/music_object.hpp"
#include "object/tilemap.hpp"
#include "physfs/util.hpp"
#include "supertux/tile_manager.hpp"
#include "util/file_system.hpp"
#include "util/reader_collection.hpp"
#include "util/reader_document.hpp"
#include "util/reader_mapping.hpp"
#include "worldmap/level_tile.hpp"
#include "worldmap/special_tile.hpp"
#include "worldmap/sprite_change.hpp"
#include "worldmap/teleporter.hpp"
#include "worldmap/worldmap.hpp"

namespace worldmap {

WorldMapParser::WorldMapParser(WorldMap& worldmap) :
  m_worldmap(worldmap)
{
}

void
WorldMapParser::load_worldmap(std::string const& filename)
{
  m_worldmap.m_map_filename = physfsutil::realpath(filename);
  m_worldmap.m_levels_path = FileSystem::dirname(m_worldmap.m_map_filename);

  try {
    auto doc = load_reader_document(m_worldmap.m_map_filename);
    auto root = doc.get_root();

    if (root.get_name() != "supertux-level")
      throw std::runtime_error("file isn't a supertux-level file.");

    auto level_ = root.get_mapping();

    m_worldmap.m_tileset = TileManager::current()->get_tileset("images/ice_world.stts");

    level_.read("name", m_worldmap.m_name);

    ReaderCollection sectors_collection;
    if (level_.read("sectors", sectors_collection))
    {
      for (auto const& sectors_item : sectors_collection.get_objects())
      {
        if (sectors_item.get_name() == "sector")
        {
          ReaderMapping const& sector = sectors_item.get_mapping();
          sector.read("init-script", m_worldmap.m_init_script);

          ReaderCollection sector_objects;
          if (sector.read("objects", sector_objects))
          {
            for (auto const& obj : sector_objects.get_objects())
            {
              if (obj.get_name() == "music") {
                m_worldmap.add<MusicObject>(obj.get_mapping());
              } else if (obj.get_name() == "worldmap-spawnpoint") {
                auto sp = std::make_unique<SpawnPoint>(obj.get_mapping());
                m_worldmap.m_spawn_points.push_back(std::move(sp));
              } else if (obj.get_name() == "tilemap") {
                m_worldmap.add<TileMap>(m_worldmap.m_tileset, obj.get_mapping());
              } else if (obj.get_name() == "background") {
                m_worldmap.add<Background>(obj.get_mapping());
              } else if (obj.get_name() == "level") {
                auto& level = m_worldmap.add<LevelTile>(m_worldmap.m_levels_path, obj.get_mapping());
                load_level_information(level);
              } else if (obj.get_name() == "special-tile") {
                m_worldmap.add<SpecialTile>(obj.get_mapping());
              } else if (obj.get_name() == "sprite-change") {
                m_worldmap.add<SpriteChange>(obj.get_mapping());
              } else if (obj.get_name() == "teleporter") {
                m_worldmap.add<Teleporter>(obj.get_mapping());
              } else if (obj.get_name() == "decal") {
                m_worldmap.add<Decal>(obj.get_mapping());
              } else if (obj.get_name() == "path") {
                m_worldmap.add<PathGameObject>(obj.get_mapping());
              } else if (obj.get_name() == "ambient-light") {
                m_worldmap.add<AmbientLight>(obj.get_mapping());
              } else {
                throw std::runtime_error(fmt::format("unknown worldmap object: '{}'", obj.get_name()));
              }
            }
          }
        }
      }
    }

    m_worldmap.flush_game_objects();

    if (m_worldmap.get_solid_tilemaps().empty())
      log_warning("No solid tilemap specified");

    m_worldmap.move_to_spawnpoint("main");

  } catch(std::exception const& e) {
    std::stringstream msg;
    msg << "Problem when parsing worldmap '" << m_worldmap.m_map_filename << "': " <<
      e.what();
    throw std::runtime_error(msg.str());
  }

  m_worldmap.finish_construction();
}

void
WorldMapParser::load_level_information(LevelTile& level)
{
  /** get special_tile's title */
  level.m_title = _("<no title>");
  level.m_target_time = 0.0f;

  try {
    std::string filename = m_worldmap.m_levels_path + level.get_level_filename();

    if (m_worldmap.m_levels_path == "./")
      filename = level.get_level_filename();

    if (!PHYSFS_exists(filename.c_str()))
    {
      log_warning("Level file '{}' does not exist. Skipping.", filename);
      return;
    }
    if (physfsutil::is_directory(filename))
    {
      log_warning("Level file '{}' is a directory. Skipping.", filename);
      return;
    }

    auto doc = load_reader_document(filename);
    auto root = doc.get_root();
    if (root.get_name() != "supertux-level") {
      return;
    } else {
      auto level_mapping = root.get_mapping();
      level_mapping.read("name", level.m_title);
      level_mapping.read("target-time", level.m_target_time);
    }
  } catch(std::exception& e) {
    log_warning("Problem when reading level information: {}", e.what());
    return;
  }
}

} // namespace worldmap

/* EOF */
