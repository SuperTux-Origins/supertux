//  SuperTux
//  Copyright (C) 2006 Matthias Braun <matze@braunis.de>
//                2014 Ingo Ruhnke <grumbel@gmail.com>
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

#include "supertux/savegame.hpp"

#include <physfs.h>

#include "control/input_manager.hpp"
#include "physfs/util.hpp"
#include "squirrel/serialize.hpp"
#include "supertux/player_status.hpp"
#include "util/file_system.hpp"
#include "util/log.hpp"
#include "util/reader_document.hpp"
#include "util/reader_mapping.hpp"
#include "worldmap/worldmap.hpp"

namespace {

std::vector<LevelState> get_level_states(SquirrelVM& vm)
{
  std::vector<LevelState> results;

  sq_pushnull(vm.get_vm());
  while (SQ_SUCCEEDED(sq_next(vm.get_vm(), -2)))
  {
    //here -1 is the value and -2 is the key
    char const* result;
    if (SQ_FAILED(sq_getstring(vm.get_vm(), -2, &result)))
    {
      std::ostringstream msg;
      msg << "Couldn't get string value";
      throw SquirrelError(vm.get_vm(), msg.str());
    }
    else
    {
      LevelState level_state;
      level_state.filename = result;
      vm.get_bool("solved", level_state.solved);
      vm.get_bool("perfect", level_state.perfect);

      results.push_back(level_state);
    }

    // pops key and val before the next iteration
    sq_pop(vm.get_vm(), 2);
  }

  return results;
}

} // namespace

void
LevelsetState::store_level_state(LevelState const& in_state)
{
  auto it = std::find_if(level_states.begin(), level_states.end(),
                         [&in_state](LevelState const& state)
                         {
                           return state.filename == in_state.filename;
                         });
  if (it != level_states.end())
  {
    *it = in_state;
  }
  else
  {
    level_states.push_back(in_state);
  }
}

LevelState
LevelsetState::get_level_state(std::string const& filename) const
{
  auto it = std::find_if(level_states.begin(), level_states.end(),
                         [filename](LevelState const& state)
                         {
                           return state.filename == filename;
                         });
  if (it != level_states.end())
  {
    return *it;
  }
  else
  {
    log_info("creating new level state for {}", filename);
    LevelState state;
    state.filename = filename;
    return state;
  }
}

std::unique_ptr<Savegame>
Savegame::from_file(std::string const& filename)
{
  std::unique_ptr<Savegame> savegame(new Savegame(filename));
  savegame->load();
  return savegame;
}

Savegame::Savegame(std::string const& filename) :
  m_filename(filename),
  m_player_status(new PlayerStatus(InputManager::current()->get_num_users()))
{
}

bool
Savegame::is_title_screen() const
{
  // bit of a hack, TileScreen uses a dummy savegame without a filename
  return m_filename.empty();
}

void
Savegame::load()
{
  if (m_filename.empty())
  {
    log_debug("no filename set for savegame, skipping load");
    return;
  }

  clear_state_table();

  if (!PHYSFS_exists(m_filename.c_str()))
  {
    log_info("{} doesn't exist, not loading state", m_filename);
  }
  else
  {
    if (physfsutil::is_directory(m_filename))
    {
      log_info("{} is a directory, not loading state", m_filename);
      return;
    }
    log_debug("loading savegame from {}", m_filename);

    try
    {
      SquirrelVM& vm = SquirrelVirtualMachine::current()->get_vm();

      auto doc = load_reader_document(m_filename);
      auto root = doc.get_root();

      if (root.get_name() != "supertux-savegame")
      {
        throw std::runtime_error("file is not a supertux-savegame file");
      }
      else
      {
        auto mapping = root.get_mapping();

        int version = 1;
        mapping.read("version", version);
        if (version != 1)
        {
          throw std::runtime_error("incompatible savegame version");
        }
        else
        {
          ReaderMapping tux;
          if (!mapping.read("tux", tux))
          {
            throw std::runtime_error("No tux section in savegame");
          }
          {
            m_player_status->read(tux);
          }

          ReaderMapping state;
          if (!mapping.read("state", state))
          {
            throw std::runtime_error("No state section in savegame");
          }
          else
          {
            sq_pushroottable(vm.get_vm());
            vm.get_table_entry("state");
            load_squirrel_table(vm.get_vm(), -1, state);
            sq_pop(vm.get_vm(), 2);
          }
        }
      }
    }
    catch(std::exception const& e)
    {
      log_fatal("Couldn't load savegame: {}", e.what());
    }
  }
}

void
Savegame::clear_state_table()
{
  SquirrelVM& vm = SquirrelVirtualMachine::current()->get_vm();

  // delete existing state table, if it exists
  sq_pushroottable(vm.get_vm());
  {
    // create a new empty state table
    vm.create_empty_table("state");
  }
  sq_pop(vm.get_vm(), 1);
}

void
Savegame::save()
{
  if (m_filename.empty())
  {
    log_debug("no filename set for savegame, skipping save");
    return;
  }

  log_debug("saving savegame to {}", m_filename);

  { // make sure the savegame directory exists
    std::string dirname = FileSystem::dirname(m_filename);
    if (!PHYSFS_exists(dirname.c_str()))
    {
      if (!PHYSFS_mkdir(dirname.c_str()))
      {
        std::ostringstream msg;
        msg << "Couldn't create directory for savegames '"
            << dirname << "': " <<PHYSFS_getErrorByCode(PHYSFS_getLastErrorCode());
        throw std::runtime_error(msg.str());
      }
    }

    if (!physfsutil::is_directory(dirname))
    {
      std::ostringstream msg;
      msg << "Savegame path '" << dirname << "' is not a directory";
      throw std::runtime_error(msg.str());
    }
  }

  SquirrelVM& vm = SquirrelVirtualMachine::current()->get_vm();

  Writer writer(m_filename);

  writer.start_list("supertux-savegame");
  writer.write("version", 1);

  using namespace worldmap;
  if (WorldMap::current() != nullptr)
  {
    std::ostringstream title;
    title << WorldMap::current()->get_title();
    title << " (" << WorldMap::current()->solved_level_count()
          << "/" << WorldMap::current()->level_count() << ")";
    writer.write("title", title.str());
  }

  writer.start_list("tux");
  m_player_status->write(writer);
  writer.end_list("tux");

  writer.start_list("state");

  sq_pushroottable(vm.get_vm());
  try
  {
    vm.get_table_entry("state"); // Push "state"
    save_squirrel_table(vm.get_vm(), -1, writer);
    sq_pop(vm.get_vm(), 1); // Pop "state"
  }
  catch(std::exception const&)
  {
  }
  sq_pop(vm.get_vm(), 1); // Pop root table
  writer.end_list("state");

  writer.end_list("supertux-savegame");
}

std::vector<std::string>
Savegame::get_worldmaps()
{
  std::vector<std::string> worlds;

  SquirrelVM& vm = SquirrelVirtualMachine::current()->get_vm();
  SQInteger oldtop = sq_gettop(vm.get_vm());

  try
  {
    sq_pushroottable(vm.get_vm());
    vm.get_table_entry("state");
    vm.get_or_create_table_entry("worlds");
    worlds = vm.get_table_keys();
  }
  catch(std::exception const& err)
  {
    log_warning("{}", err.what());
  }

  sq_settop(vm.get_vm(), oldtop);

  // ensure that the loaded worldmap names have their canonical form
  std::transform(worlds.begin(), worlds.end(), worlds.begin(), physfsutil::realpath);

  return worlds;
}

WorldmapState
Savegame::get_worldmap_state(std::string const& name)
{
  WorldmapState result;

  SquirrelVM& vm = SquirrelVirtualMachine::current()->get_vm();
  SQInteger oldtop = sq_gettop(vm.get_vm());

  try
  {
    sq_pushroottable(vm.get_vm());
    vm.get_table_entry("state");
    vm.get_or_create_table_entry("worlds");

    // if a non-canonical entry is present, replace them with a canonical one
    if (name != "/levels/world2/worldmap.stwm") {
      std::string old_map_filename = name.substr(1);
      if (vm.has_property(old_map_filename.c_str())) {
        vm.rename_table_entry(old_map_filename.c_str(), name.c_str());
      }
    }

    vm.get_or_create_table_entry(name);
    vm.get_or_create_table_entry("levels");

    result.level_states = get_level_states(vm);
  }
  catch(std::exception const& err)
  {
    log_warning("{}", err.what());
  }

  sq_settop(vm.get_vm(), oldtop);

  return result;
}

std::vector<std::string>
Savegame::get_levelsets()
{
  std::vector<std::string> results;

  SquirrelVM& vm = SquirrelVirtualMachine::current()->get_vm();
  SQInteger oldtop = sq_gettop(vm.get_vm());

  try
  {
    sq_pushroottable(vm.get_vm());
    vm.get_table_entry("state");
    vm.get_or_create_table_entry("levelsets");
    results = vm.get_table_keys();
  }
  catch(std::exception const& err)
  {
    log_warning("{}", err.what());
  }

  sq_settop(vm.get_vm(), oldtop);

  return results;
}

LevelsetState
Savegame::get_levelset_state(std::string const& basedir)
{
  LevelsetState result;

  SquirrelVM& vm = SquirrelVirtualMachine::current()->get_vm();
  SQInteger oldtop = sq_gettop(vm.get_vm());

  try
  {
    sq_pushroottable(vm.get_vm());
    vm.get_table_entry("state");
    vm.get_or_create_table_entry("levelsets");
    vm.get_or_create_table_entry(basedir);
    vm.get_or_create_table_entry("levels");

    result.level_states = get_level_states(vm);
  }
  catch(std::exception const& err)
  {
    log_warning("{}", err.what());
  }

  sq_settop(vm.get_vm(), oldtop);

  return result;
}

void
Savegame::set_levelset_state(std::string const& basedir,
                             std::string const& level_filename,
                             bool solved)
{
  LevelsetState state = get_levelset_state(basedir);

  SquirrelVM& vm = SquirrelVirtualMachine::current()->get_vm();
  SQInteger oldtop = sq_gettop(vm.get_vm());

  try
  {
    sq_pushroottable(vm.get_vm());
    vm.get_table_entry("state");
    vm.get_or_create_table_entry("levelsets");
    vm.get_or_create_table_entry(basedir);
    vm.get_or_create_table_entry("levels");
    vm.get_or_create_table_entry(level_filename);

    bool old_solved = false;
    vm.get_bool("solved", old_solved);
    vm.store_bool("solved", solved || old_solved);
  }
  catch(std::exception const& err)
  {
    log_warning("{}", err.what());
  }

  sq_settop(vm.get_vm(), oldtop);
}

/* EOF */
