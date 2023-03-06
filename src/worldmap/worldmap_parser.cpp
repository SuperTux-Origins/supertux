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
#include "object/path_gameobject.hpp"
#include "object/tilemap.hpp"
#include "physfs/util.hpp"
#include "supertux/tile_manager.hpp"
#include "util/file_system.hpp"
#include "util/log.hpp"
#include "util/reader.hpp"
#include "util/reader_document.hpp"
#include "util/reader_mapping.hpp"
#include "util/reader_object.hpp"
#include "worldmap/level_tile.hpp"
#include "worldmap/spawn_point.hpp"
#include "worldmap/special_tile.hpp"
#include "worldmap/sprite_change.hpp"
#include "worldmap/teleporter.hpp"
#include "worldmap/tux.hpp"
#include "worldmap/worldmap.hpp"
#include "worldmap/worldmap_parser.hpp"
#include "worldmap/worldmap_screen.hpp"

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

    level_.read("name", m_worldmap.m_name);

    std::string tileset_name;
    if (level_.read("tileset", tileset_name)) {
      if (m_worldmap.m_tileset != nullptr) {
        log_warning << "multiple tilesets specified in level_" << std::endl;
      } else {
        m_worldmap.m_tileset = TileManager::current()->get_tileset(tileset_name);
      }
    }
    /* load default tileset */
    if (m_worldmap.m_tileset == nullptr) {
      m_worldmap.m_tileset = TileManager::current()->get_tileset("images/ice_world.strf");
    }

    ReaderMapping sector;
    if (level_.read("sector", sector))
    {
      ReaderMapping tilemap_mapping;
      if (sector.read("tilemap", tilemap_mapping)) {
        m_worldmap.add<TileMap>(m_worldmap.m_tileset, tilemap_mapping);
      }

      ReaderMapping background_mapping;
      if (sector.read("background", background_mapping)) {
        m_worldmap.add<Background>(background_mapping);
      }

      std::string music_file;
      if (sector.read("music", music_file)) {
        m_worldmap.add<MusicObject>().set_music(music_file);
      }

      ReaderMapping music_mapping;
      if (sector.read("music", music_mapping)) {
        m_worldmap.add<MusicObject>(music_mapping);
      }

      sector.read("init-script", m_worldmap.m_init_script);

      ReaderMapping worldmap_spawnpoint_mapping;
      if (sector.read("worldmap-spawnpoint", worldmap_spawnpoint_mapping)) {
        auto sp = std::make_unique<SpawnPoint>(worldmap_spawnpoint_mapping);
        m_worldmap.m_spawn_points.push_back(std::move(sp));
      }

      ReaderMapping level_mapping;
      if (sector.read("level", level_mapping)) {
        auto& level = m_worldmap.add<LevelTile>(m_worldmap.m_levels_path, level_mapping);
        load_level_information(level);
      }

      ReaderMapping special_tile_mapping;
      if (sector.read("special-tile", special_tile_mapping)) {
        m_worldmap.add<SpecialTile>(special_tile_mapping);
      }

      ReaderMapping sprite_change_mapping;
      if (sector.read("sprite-change", sprite_change_mapping)) {
        m_worldmap.add<SpriteChange>(sprite_change_mapping);
      }

      ReaderMapping teleporter_mapping;
      if (sector.read("teleporter", teleporter_mapping)) {
        m_worldmap.add<Teleporter>(teleporter_mapping);
      }

      ReaderMapping decal_mapping;
      if (sector.read("decal", decal_mapping)) {
        m_worldmap.add<Decal>(decal_mapping);
      }

      ReaderMapping path_mapping;
      if (sector.read("path", path_mapping)) {
        m_worldmap.add<PathGameObject>(path_mapping);
      }

      ReaderMapping ambient_light_mapping;
      if (sector.read("ambient-light", ambient_light_mapping)) {
        m_worldmap.add<AmbientLight>(ambient_light_mapping);
      }
    }

    m_worldmap.flush_game_objects();

    if (m_worldmap.get_solid_tilemaps().empty())
      log_warning << "No solid tilemap specified" << std::endl;

    m_worldmap.move_to_spawnpoint("main");

  } catch(std::exception& e) {
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
      log_warning << "Level file '" << filename << "' does not exist. Skipping." << std::endl;
      return;
    }
    if (physfsutil::is_directory(filename))
    {
      log_warning << "Level file '" << filename << "' is a directory. Skipping." << std::endl;
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
    log_warning << "Problem when reading level information: " << e.what() << std::endl;
    return;
  }
}

} // namespace worldmap

/* EOF */
