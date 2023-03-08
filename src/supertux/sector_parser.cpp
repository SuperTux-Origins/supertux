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

#include <iostream>
#include <physfs.h>
#include <sexp/value.hpp>

#include "badguy/fish_jumping.hpp"
#include "badguy/jumpy.hpp"
#include "object/ambient_light.hpp"
#include "object/background.hpp"
#include "object/camera.hpp"
#include "object/cloud_particle_system.hpp"
#include "object/custom_particle_system.hpp"
#include "object/gradient.hpp"
#include "object/music_object.hpp"
#include "object/rain_particle_system.hpp"
#include "object/snow_particle_system.hpp"
#include "object/spawnpoint.hpp"
#include "object/tilemap.hpp"
#include "supertux/game_object_factory.hpp"
#include "supertux/level.hpp"
#include "supertux/sector.hpp"
#include "supertux/tile.hpp"
#include "supertux/tile_manager.hpp"
#include "util/reader_collection.hpp"
#include "util/reader_mapping.hpp"
#include "util/reader_object.hpp"
#include "util/reader_iterator.hpp"

static const std::string DEFAULT_BG = "images/background/antarctic/arctis2.png";

std::unique_ptr<Sector>
SectorParser::from_reader(Level& level, ReaderMapping const& reader, bool editable)
{
  auto sector = std::make_unique<Sector>(level);
  BIND_SECTOR(*sector);
  SectorParser parser(*sector, editable);
  parser.parse(reader);
  return sector;
}

std::unique_ptr<Sector>
SectorParser::from_nothing(Level& level)
{
  auto sector = std::make_unique<Sector>(level);
  BIND_SECTOR(*sector);
  SectorParser parser(*sector, false);
  parser.create_sector();
  return sector;
}

SectorParser::SectorParser(Sector& sector, bool editable) :
  m_sector(sector),
  m_editable(editable)
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
      log_warning << e.what() << "" << std::endl;
      return {};
    }
  }
}

void
SectorParser::parse(ReaderMapping const& sector)
{
  ReaderIterator iter(sector);
  while (iter.next()) {
    if (iter.get_key() == "name") {
      std::string value;
      iter.get(value);
      m_sector.set_name(value);
    } else if (iter.get_key() == "gravity") {
      float value;
      iter.get(value);
      m_sector.set_gravity(value);
    } else if (iter.get_key() == "music") {
      auto const& sx = iter.get_sexp();
      if (sx.is_array() && sx.as_array().size() == 2 && sx.as_array()[1].is_string()) {
        std::string value;
        iter.get(value);
        m_sector.add<MusicObject>().set_music(value);
      } else {
        m_sector.add<MusicObject>(iter.as_mapping());
      }
    } else if (iter.get_key() == "init-script") {
      std::string value;
      iter.get(value);
      m_sector.set_init_script(value);
    } else if (iter.get_key() == "ambient-light") {
      auto const& sx = iter.get_sexp();
      if (sx.is_array() && sx.as_array().size() >= 3 &&
          sx.as_array()[1].is_real() && sx.as_array()[2].is_real() && sx.as_array()[3].is_real())
      {
        // for backward compatibilty
        std::vector<float> vColor;
        bool hasColor = sector.read("ambient-light", vColor);
        if (vColor.size() < 3 || !hasColor) {
          log_warning << "(ambient-light) requires a color as argument" << std::endl;
        } else {
          m_sector.add<AmbientLight>(Color(vColor));
        }
      } else {
        // modern format
        m_sector.add<AmbientLight>(iter.as_mapping());
      }
    } else {
      auto object = parse_object(iter.get_key(), iter.as_mapping());
      if (object) {
        m_sector.add_object(std::move(object));
      }
    }
  }

  m_sector.finish_construction(m_editable);
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

  m_sector.finish_construction(m_editable);
}

/* EOF */
