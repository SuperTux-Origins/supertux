//  SuperTux
//  Copyright (C) 2008 Matthias Braun <matze@braunis.de>
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

#include "supertux/tile_set_parser.hpp"

#include <sstream>
#include <sexp/value.hpp>
#include <sexp/io.hpp>

#include <prio/sexpr_reader_impl.hpp>

#include "supertux/autotile_parser.hpp"
#include "supertux/gameconfig.hpp"
#include "supertux/globals.hpp"
#include "supertux/tile_set.hpp"
#include "util/file_system.hpp"
#include "util/log.hpp"
#include "util/reader_collection.hpp"
#include "util/reader_document.hpp"
#include "util/reader_iterator.hpp"
#include "util/reader_mapping.hpp"
#include "video/surface.hpp"

TileSetParser::TileSetParser(TileSet& tileset, std::string const& filename) :
  m_tileset(tileset),
  m_filename(filename),
  m_tiles_path()
{
}

void
TileSetParser::parse()
{
  m_tiles_path = FileSystem::dirname(m_filename);

  auto const& doc = load_reader_document(m_filename);
  auto const& root = doc.get_root();
  auto const& mapping = doc.get_mapping();

  if (root.get_name() != "supertux-tileset") {
    throw std::runtime_error("file is not a supertux-tileset file.");
  }

  ReaderCollection tiles_collection;
  if (mapping.read("tiles", tiles_collection))
  {
    for (auto const& tiledef : tiles_collection.get_objects())
    {
      if (tiledef.get_name() == "tile") {
        ReaderMapping tile_mapping = tiledef.get_mapping();
        parse_tile(tile_mapping);
      } else if (tiledef.get_name() == "tiles") {
        ReaderMapping tiles_mapping = tiledef.get_mapping();
        parse_tiles(tiles_mapping);
      }
    }
  }

  ReaderCollection tilegroups_collection;
  if (mapping.read("tilegroups", tilegroups_collection))
  {
    for (auto const& tilegroup_obj : tiles_collection.get_objects())
    {
      if (tilegroup_obj.get_name() == "tilegroup") {
        ReaderMapping reader = tilegroup_obj.get_mapping();
        Tilegroup tilegroup;
        reader.read("name", tilegroup.name);
        reader.read("tiles", tilegroup.tiles);
        m_tileset.add_tilegroup(tilegroup);
      }
    }
  }

  ReaderCollection autotilesets_collection;
  if (mapping.read("autotilesets", autotilesets_collection))
  {
    for (auto const& autotileset_obj : autotilesets_collection.get_objects())
    {
      if (autotileset_obj.get_name() == "autotileset") {
        ReaderMapping reader = autotileset_obj.get_mapping();
        std::string autotile_filename;
        if (!reader.read("source", autotile_filename))
        {
          log_warning << "No source path for autotiles in file '" << m_filename << "'" << std::endl;
        }
        else
        {
          AutotileParser* parser = new AutotileParser(m_tileset.m_autotilesets,
                                                      FileSystem::normalize(m_tiles_path + autotile_filename));
          parser->parse();
        }
      }
    }
  }

  if (g_config->developer_mode)
  {
    m_tileset.add_unassigned_tilegroup();
  }
}

void
TileSetParser::parse_tile(ReaderMapping const& reader)
{
  uint32_t id;
  if (!reader.read("id", id))
  {
    throw std::runtime_error("Missing tile-id.");
  }

  uint32_t attributes = 0;

  bool value = false;
  if (reader.read("solid", value) && value)
    attributes |= Tile::SOLID;
  if (reader.read("unisolid", value) && value)
    attributes |= Tile::UNISOLID | Tile::SOLID;
  if (reader.read("brick", value) && value)
    attributes |= Tile::BRICK;
  if (reader.read("ice", value) && value)
    attributes |= Tile::ICE;
  if (reader.read("water", value) && value)
    attributes |= Tile::WATER;
  if (reader.read("hurts", value) && value)
    attributes |= Tile::HURTS;
  if (reader.read("fire", value) && value)
    attributes |= Tile::FIRE;
  if (reader.read("walljump", value) && value)
    attributes |= Tile::WALLJUMP;
  if (reader.read("fullbox", value) && value)
    attributes |= Tile::FULLBOX;
  if (reader.read("coin", value) && value)
    attributes |= Tile::COIN;
  if (reader.read("goal", value) && value)
    attributes |= Tile::GOAL;

  uint32_t data = 0;

  if (reader.read("north", value) && value)
    data |= Tile::WORLDMAP_NORTH;
  if (reader.read("south", value) && value)
    data |= Tile::WORLDMAP_SOUTH;
  if (reader.read("west", value) && value)
    data |= Tile::WORLDMAP_WEST;
  if (reader.read("east", value) && value)
    data |= Tile::WORLDMAP_EAST;
  if (reader.read("stop", value) && value)
    data |= Tile::WORLDMAP_STOP;

  reader.read("data", data);

  float fps = 10;
  reader.read("fps", fps);

  std::string object_name, object_data;
  reader.read("object-name", object_name);
  reader.read("object-data", object_data);

  if (reader.read("slope-type", data))
  {
    attributes |= Tile::SOLID | Tile::SLOPE;
  }

  std::vector<SurfacePtr> editor_surfaces;
  ReaderMapping editor_images_mapping;
  if (reader.read("editor-images", editor_images_mapping)) {
    editor_surfaces = parse_imagespecs(editor_images_mapping);
  }

  std::vector<SurfacePtr> surfaces;
  ReaderMapping images_mapping;
  if (reader.read("images", images_mapping)) {
    surfaces = parse_imagespecs(images_mapping);
  }

  bool deprecated = false;
  reader.read("deprecated", deprecated);

  auto tile = std::make_unique<Tile>(surfaces, editor_surfaces,
                                     attributes, data, fps,
                                     object_name, object_data, deprecated);
  m_tileset.add_tile(id, std::move(tile));
}

void
TileSetParser::parse_tiles(ReaderMapping const& reader)
{
  // List of ids (use 0 if the tile should be ignored)
  std::vector<uint32_t> ids;
  // List of attributes of the tile
  std::vector<uint32_t> attributes;
  // List of data for the tiles
  std::vector<uint32_t> datas;

  // width and height of the image in tile units, this is used for two
  // purposes:
  //  a) so we don't have to load the image here to know its dimensions
  //  b) so that the resulting 'tiles' entry is more robust,
  //  ie. enlarging the image won't break the tile id mapping
  // FIXME: height is actually not used, since width might be enough for
  // all purposes, still feels somewhat more natural this way
  unsigned int width  = 0;
  unsigned int height = 0;

  bool has_ids = reader.read("ids", ids);
  bool has_attributes = reader.read("attributes", attributes);
  bool has_datas = reader.read("datas", datas);

  reader.read("width", width);
  reader.read("height", height);

  bool shared_surface = false;
  reader.read("shared-surface", shared_surface);

  float fps = 10;
  reader.read("fps",     fps);

  if (ids.empty() || !has_ids)
  {
    throw std::runtime_error("No IDs specified.");
  }

  if (width == 0)
  {
    throw std::runtime_error("Width is zero.");
  }
  else if (height == 0)
  {
    throw std::runtime_error("Height is zero.");
  }
  else if (fps < 0)
  {
    throw std::runtime_error("Negative fps.");
  }
  else if (ids.size() != width*height)
  {
    std::ostringstream err;
    err
      << dynamic_cast<prio::SExprReaderMappingImpl const&>(reader.get_impl()).get_sx().get_line()
      << ": Number of ids (" << ids.size() <<  ") and "
      "dimensions of image (" << width << "x" << height << " = " << width*height << ") "
      "differ";

    throw std::runtime_error(err.str());
  }
  else if (has_attributes && (ids.size() != attributes.size()))
  {
    std::ostringstream err;
    err << "Number of ids (" << ids.size() <<  ") and attributes (" << attributes.size()
        << ") mismatch, but must be equal";
    throw std::runtime_error(err.str());
  }
  else if (has_datas && ids.size() != datas.size())
  {
    std::ostringstream err;
    err << "Number of ids (" << ids.size() <<  ") and datas (" << datas.size()
        << ") mismatch, but must be equal";
    throw std::runtime_error(err.str());
  }
  else
  {
    if (shared_surface)
    {
      std::vector<SurfacePtr> editor_surfaces;
      ReaderMapping editor_surfaces_mapping;
      if (reader.read("editor-images", editor_surfaces_mapping)) {
        editor_surfaces = parse_imagespecs(editor_surfaces_mapping);
      }

      std::vector<SurfacePtr> surfaces;
      ReaderMapping surfaces_mapping;
      if (reader.read("image", surfaces_mapping) ||
          reader.read("images", surfaces_mapping)) {
        surfaces = parse_imagespecs(surfaces_mapping);
      }

      for (size_t i = 0; i < ids.size(); ++i)
      {
        if (ids[i] != 0)
        {
          const int x = static_cast<int>(32 * (i % width));
          const int y = static_cast<int>(32 * (i / width));

          std::vector<SurfacePtr> regions;
          regions.reserve(surfaces.size());
          std::transform(surfaces.begin(), surfaces.end(), std::back_inserter(regions),
                         [x, y] (const SurfacePtr& surface) { 
                           return surface->region(Rect(x, y, Size(32, 32)));
                         });

          std::vector<SurfacePtr> editor_regions;
          editor_regions.reserve(editor_surfaces.size());
          std::transform(editor_surfaces.begin(), editor_surfaces.end(), std::back_inserter(editor_regions),
                         [x, y] (SurfacePtr const& surface) { 
                           return surface->region(Rect(x, y, Size(32, 32)));
                         });

          auto tile = std::make_unique<Tile>(regions,
                                             editor_regions,
                                             (has_attributes ? attributes[i] : 0),
                                             (has_datas ? datas[i] : 0),
                                             fps);

          m_tileset.add_tile(ids[i], std::move(tile));
        }
      }
    }
    else // (!shared_surface)
    {
      for (size_t i = 0; i < ids.size(); ++i)
      {
        if (ids[i] != 0)
        {
          int x = static_cast<int>(32 * (i % width));
          int y = static_cast<int>(32 * (i / width));

          std::vector<SurfacePtr> surfaces;
          ReaderMapping surfaces_mapping;
          if (reader.read("image", surfaces_mapping) ||
              reader.read("images", surfaces_mapping)) {
            surfaces = parse_imagespecs(surfaces_mapping, Rect(x, y, Size(32, 32)));
          }

          std::vector<SurfacePtr> editor_surfaces;
          ReaderMapping editor_surfaces_mapping;
          if (reader.read("editor-images", editor_surfaces_mapping)) {
            editor_surfaces = parse_imagespecs(editor_surfaces_mapping, Rect(x, y, Size(32, 32)));
          }

          auto tile = std::make_unique<Tile>(surfaces,
                                             editor_surfaces,
                                             (has_attributes ? attributes[i] : 0),
                                             (has_datas ? datas[i] : 0),
                                             fps);

          m_tileset.add_tile(ids[i], std::move(tile));
        }
      }
    }
  }
}

std::vector<SurfacePtr>
TileSetParser::parse_imagespecs(ReaderMapping const& images_mapping,
                                std::optional<Rect> const& surface_region) const
{
  std::vector<SurfacePtr> surfaces;

  // (images "foo.png" "foo.bar" ...)
  // (images (region "foo.png" 0 0 32 32))
  ReaderIterator iter(images_mapping);
  while (iter.next())
  {
    if (iter.is_string())
    {
      std::string file = iter.as_string_item();
      surfaces.push_back(Surface::from_file(FileSystem::join(m_tiles_path, file), surface_region));
    }
    else if (iter.is_pair() && iter.get_key() == "surface")
    {
      surfaces.push_back(Surface::from_reader(iter.as_mapping(), surface_region));
    }
    else if (iter.is_pair() && iter.get_key() == "region")
    {
      auto const& sx = dynamic_cast<prio::SExprReaderMappingImpl const&>(iter.as_mapping().get_impl()).get_sx();
      auto const& arr = sx.as_array();
      if (arr.size() != 6)
      {
        log_warning << "(region X Y WIDTH HEIGHT) tag malformed: " << sx << std::endl;
      }
      else
      {
        const std::string file = arr[1].as_string();
        const int x = arr[2].as_int();
        const int y = arr[3].as_int();
        const int w = arr[4].as_int();
        const int h = arr[5].as_int();

        Rect rect(x, y, x + w, y + h);

        if (surface_region)
        {
          rect.left += surface_region->left;
          rect.top += surface_region->top;

          rect.right = rect.left + surface_region->get_width();
          rect.bottom = rect.top + surface_region->get_height();
        }

        surfaces.push_back(Surface::from_file(FileSystem::join(m_tiles_path, file),
                                              rect));
      }
    }
    else
    {
      log_warning << "Expected string or list in images tag" << std::endl;
    }
  }

  return surfaces;
}

/* EOF */
