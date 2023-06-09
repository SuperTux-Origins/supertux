//  SuperTux
//  Copyright (C) 2006 Matthias Braun <matze@braunis.de>
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

#include "supertux/world.hpp"

#include <physfs.h>

#include "physfs/util.hpp"
#include "supertux/gameconfig.hpp"
#include "supertux/globals.hpp"
#include "util/file_system.hpp"
#include "util/log.hpp"
#include "util/reader_document.hpp"
#include "util/reader_mapping.hpp"
#include "util/writer.hpp"

std::unique_ptr<World>
World::from_directory(std::string const& directory)
{
  std::unique_ptr<World> world(new World(directory));

  std::string info_filename = FileSystem::join(directory, "info");

  try
  {
    auto doc = load_reader_document(info_filename);
    auto root = doc.get_root();

    if (root.get_name() != "supertux-world" &&
        root.get_name() != "supertux-level-subset")
    {
      throw std::runtime_error("File is not a world or levelsubset file");
    }

    auto info = root.get_mapping();

    info.read("title", world->m_title);
    info.read("description", world->m_description);
    world->m_is_levelset = info.get("levelset", true);
    world->m_hide_from_contribs = info.get("hide-from-contribs", false);
    world->m_contrib_type = info.get("contrib-type", std::string("user"));
    return world;
  }
  catch (std::exception const& err)
  {
    log_warning("Failed to load {}:{}", info_filename, err.what());

    world->m_hide_from_contribs = true;

    return world;
  }
}

std::unique_ptr<World>
World::create(std::string const& title, std::string const& desc)
{
  // Limit the charset to numbers and alphabet.
  std::string base = title;

  for (size_t i = 0; i < base.length(); i++) {
    if (!isalnum(base[i])) {
      base[i] = '_';
    }
  }

  base = FileSystem::join("levels", base);

  // Find a non-existing fitting directory name
  std::string dirname = base;
  if (PHYSFS_exists(dirname.c_str())) {
    int num = 1;
    do {
      num++;
      dirname = base + std::to_string(num);
    } while (PHYSFS_exists(dirname.c_str()));
  }

  std::unique_ptr<World> world(new World(dirname));

  world->m_title = title;
  world->m_description = desc;

  return world;
}

World::World(std::string const& directory) :
  m_title(),
  m_description(),
  m_is_levelset(true),
  m_basedir(directory),
  m_hide_from_contribs(false),
  m_contrib_type()
{
}

void
World::save(bool retry)
{
  std::string filepath = FileSystem::join(m_basedir, "/info");

  try
  {
    { // make sure the levelset directory exists
      std::string dirname = FileSystem::dirname(filepath);
      if (!PHYSFS_exists(dirname.c_str()))
      {
        if (!PHYSFS_mkdir(dirname.c_str()))
        {
          std::ostringstream msg;
          msg << "Couldn't create directory for levelset '"
              << dirname << "': " <<PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode());
          throw std::runtime_error(msg.str());
        }
      }

      if (!physfsutil::is_directory(dirname))
      {
        std::ostringstream msg;
        msg << "Levelset path '" << dirname << "' is not a directory";
        throw std::runtime_error(msg.str());
      }
    }

    Writer writer(filepath);
    writer.start_list("supertux-level-subset");

    writer.write("title", m_title, true);
    writer.write("description", m_description, true);
    writer.write("levelset", m_is_levelset);
    writer.write("contrib-type", "user");
    writer.write("hide-from-contribs", m_hide_from_contribs);

    writer.end_list("supertux-level-subset");
    log_warning("Levelset info saved as {}.", filepath);
  }
  catch(std::exception& e)
  {
    if (retry) {
      std::stringstream msg;
      msg << "Problem when saving levelset info '" << filepath << "': " << e.what();
      throw std::runtime_error(msg.str());
    } else {
      log_warning("Failed to save the levelset info, retrying...");
      { // create the levelset directory again
        std::string dirname = FileSystem::dirname(filepath);
        if (!PHYSFS_mkdir(dirname.c_str()))
        {
          std::ostringstream msg;
          msg << "Couldn't create directory for levelset '"
              << dirname << "': " <<PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode());
          throw std::runtime_error(msg.str());
        }
      }
      save(true);
    }
  }
}

std::string
World::get_worldmap_filename() const
{
  return FileSystem::join(m_basedir, "worldmap.stwm");
}

std::string
World::get_savegame_filename() const
{
  const std::string worlddirname = FileSystem::basename(m_basedir);
  std::ostringstream stream;
  stream << "profile" << g_config->profile << "/" << worlddirname << ".stsg";
  return stream.str();
}

/* EOF */
