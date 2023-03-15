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

#include "supertux/sector_parser.hpp"


#include "badguy/fish_jumping.hpp"
#include "badguy/jumpy.hpp"
#include "object/background.hpp"
#include "object/camera.hpp"
#include "object/gradient.hpp"
#include "object/music_object.hpp"
#include "object/spawnpoint.hpp"
#include "object/tilemap.hpp"
#include "supertux/game_object_factory.hpp"
#include "supertux/level.hpp"
#include "supertux/sector.hpp"
#include "supertux/tile_manager.hpp"
#include "util/reader_collection.hpp"
#include "util/reader_object.hpp"

static const std::string DEFAULT_BG = "images/background/antarctic/arctis2.png";

std::unique_ptr<Sector>
SectorParser::from_reader(Level& level, ReaderMapping const& reader)
{
  auto sector = std::make_unique<Sector>(level);
  BIND_SECTOR(*sector);
  SectorParser parser(*sector);
  parser.parse(reader);
  return sector;
}

std::unique_ptr<Sector>
SectorParser::from_nothing(Level& level)
{
  auto sector = std::make_unique<Sector>(level);
  BIND_SECTOR(*sector);
  SectorParser parser(*sector);
  parser.create_sector();
  return sector;
}

SectorParser::SectorParser(Sector& sector) :
  m_sector(sector)
{
}

std::unique_ptr<GameObject>
SectorParser::parse_object(std::string const& name_, ReaderMapping const& reader)
{
  if (name_ == "money") { // for compatibility with old maps
    return std::make_unique<Jumpy>(reader);
  } else if (name_ == "fish") { //because the "fish" was renamed to "fish-jumping"
    return std::make_unique<FishJumping>(reader);
  } else {
    try {
      return GameObjectFactory::instance().create(name_, reader);
    } catch(std::exception& e) {
      log_warning("{}", e.what());
      return {};
    }
  }
}

void
SectorParser::parse(ReaderMapping const& sector)
{
  std::string sector_name;
  if (sector.read("name", sector_name)) {
    m_sector.set_name(sector_name);
  }

  float sector_gravity;
  if (sector.read("gravity", sector_gravity)) {
    m_sector.set_gravity(sector_gravity);
  }

  std::string sector_init_script;
  if (sector.read("init-script", sector_init_script)) {
    m_sector.set_init_script(sector_init_script);
  }

  ReaderCollection sector_objects;
  if (sector.read("objects", sector_objects)) {
    for (auto const& obj : sector_objects.get_objects()) {
      auto object = parse_object(obj.get_name(), obj.get_mapping());
      if (object) {
        m_sector.add_object(std::move(object));
      }
    }
  }

  m_sector.finish_construction();
}

void
SectorParser::create_sector()
{
  auto tileset = TileManager::current()->get_tileset(m_sector.get_level().get_tileset());
  bool worldmap = m_sector.get_level().is_worldmap();
  if (!worldmap)
  {
    auto& background = m_sector.add<Background>();
    background.set_image(DEFAULT_BG);
    background.set_speed(0.5);

    auto& bkgrd = m_sector.add<TileMap>(tileset);
    bkgrd.resize(100, 35);
    bkgrd.set_layer(-100);
    bkgrd.set_solid(false);

    auto& frgrd = m_sector.add<TileMap>(tileset);
    frgrd.resize(100, 35);
    frgrd.set_layer(100);
    frgrd.set_solid(false);

    // Add background gradient to sector:
    auto& gradient = m_sector.add<Gradient>();
    gradient.set_gradient(Color(0.3f, 0.4f, 0.75f), Color::WHITE);
    gradient.set_layer(-301);
  }
  else
  {
    auto& water = m_sector.add<TileMap>(tileset);
    water.resize(100, 35, 1);
    water.set_layer(-100);
    water.set_solid(false);
  }

  auto& intact = m_sector.add<TileMap>(tileset);
  if (worldmap) {
    intact.resize(100, 100, 0);
  } else {
    intact.resize(100, 35, 0);
  }
  intact.set_layer(0);
  intact.set_solid(true);

  if (worldmap) {
    // m_sector.add<worldmap_editor::WorldmapSpawnPoint>("main", Vector(4, 4));
  } else {
    m_sector.add<SpawnPointMarker>("main", Vector(64, 480));
  }

  m_sector.add<Camera>("Camera");
  m_sector.add<MusicObject>();

  m_sector.flush_game_objects();

  m_sector.finish_construction();
}

/* EOF */
