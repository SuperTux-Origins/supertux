//  SuperTux
//  Copyright (C) 2006 Matthias Braun <matze@braunis.de>
//                2018 Ingo Ruhnke <grumbel@gmail.com>
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

#include "squirrel/squirrel_vm.hpp"


#include "squirrel/squirrel_util.hpp"

SquirrelVM::SquirrelVM() :
  m_vm()
{
  m_vm = sq_open(64);
  if (m_vm == nullptr)
    throw std::runtime_error("Couldn't initialize squirrel vm");
}

SquirrelVM::~SquirrelVM()
{
#ifdef ENABLE_SQDBG
  if (debugger != nullptr) {
    sq_rdbg_shutdown(debugger);
    debugger = nullptr;
  }
#endif

  sq_close(m_vm);
}

void
SquirrelVM::begin_table(char const* name)
{
  sq_pushstring(m_vm, name, -1);
  sq_newtable(m_vm);
}

void
SquirrelVM::end_table(char const* name)
{
  if (SQ_FAILED(sq_createslot(m_vm, -3)))
    throw SquirrelError(m_vm, "Failed to create '" + std::string(name) + "' table entry");
}

void
SquirrelVM::create_empty_table(char const* name)
{
  begin_table(name);
  end_table(name);
}

bool
SquirrelVM::has_property(char const* name)
{
  sq_pushstring(m_vm, name, -1);
  if (SQ_FAILED(sq_get(m_vm, -2))) return false;
  sq_pop(m_vm, 1);
  return true;
}

void
SquirrelVM::store_bool(char const* name, bool val)
{
  sq_pushstring(m_vm, name, -1);
  sq_pushbool(m_vm, val ? SQTrue : SQFalse);
  if (SQ_FAILED(sq_createslot(m_vm, -3)))
    throw SquirrelError(m_vm, "Couldn't add float value to table");
}

void
SquirrelVM::store_int(char const* name, int val)
{
  sq_pushstring(m_vm, name, -1);
  sq_pushinteger(m_vm, val);
  if (SQ_FAILED(sq_createslot(m_vm, -3)))
    throw SquirrelError(m_vm, "Couldn't add int value to table");
}

void
SquirrelVM::store_float(char const* name, float val)
{
  sq_pushstring(m_vm, name, -1);
  sq_pushfloat(m_vm, val);
  if (SQ_FAILED(sq_createslot(m_vm, -3)))
    throw SquirrelError(m_vm, "Couldn't add float value to table");
}

void
SquirrelVM::store_string(char const* name, std::string const& val)
{
  sq_pushstring(m_vm, name, -1);
  sq_pushstring(m_vm, val.c_str(), val.length());
  if (SQ_FAILED(sq_createslot(m_vm, -3)))
    throw SquirrelError(m_vm, "Couldn't add float value to table");
}

void
SquirrelVM::store_object(char const* name, HSQOBJECT const& val)
{
  sq_pushstring(m_vm, name, -1);
  sq_pushobject(m_vm, val);
  if (SQ_FAILED(sq_createslot(m_vm, -3)))
    throw SquirrelError(m_vm, "Couldn't add object value to table");
}

bool
SquirrelVM::get_bool(char const* name, bool& val)
{
  if (!has_property(name)) return false;
  val = read_bool(name);
  return true;
}

bool
SquirrelVM::get_int(char const* name, int& val)
{
  if (!has_property(name)) return false;
  val = read_int(name);
  return true;
}

bool
SquirrelVM::get_float(char const* name, float& val)
{
  if (!has_property(name)) return false;
  val = read_float(name);
  return true;
}

bool
SquirrelVM::get_string(char const* name, std::string& val)
{
  if (!has_property(name)) return false;
  val = read_string(name);
  return true;
}

bool
SquirrelVM::read_bool(char const* name)
{
  get_table_entry(name);

  SQBool result;
  if (SQ_FAILED(sq_getbool(m_vm, -1, &result))) {
    std::ostringstream msg;
    msg << "Couldn't get bool value for '" << name << "' from table";
    throw SquirrelError(m_vm, msg.str());
  }
  sq_pop(m_vm, 1);

  return result == SQTrue;
}

int
SquirrelVM::read_int(char const* name)
{
  get_table_entry(name);

  SQInteger result;
  if (SQ_FAILED(sq_getinteger(m_vm, -1, &result))) {
    std::ostringstream msg;
    msg << "Couldn't get int value for '" << name << "' from table";
    throw SquirrelError(m_vm, msg.str());
  }
  sq_pop(m_vm, 1);

  return static_cast<int>(result);
}

float
SquirrelVM::read_float(char const* name)
{
  get_table_entry(name);

  float result;
  if (SQ_FAILED(sq_getfloat(m_vm, -1, &result))) {
    std::ostringstream msg;
    msg << "Couldn't get float value for '" << name << "' from table";
    throw SquirrelError(m_vm, msg.str());
  }
  sq_pop(m_vm, 1);

  return result;
}

std::string
SquirrelVM::read_string(char const* name)
{
  get_table_entry(name);

  char const* result;
  if (SQ_FAILED(sq_getstring(m_vm, -1, &result))) {
    std::ostringstream msg;
    msg << "Couldn't get string value for '" << name << "' from table";
    throw SquirrelError(m_vm, msg.str());
  }
  sq_pop(m_vm, 1);

  return std::string(result);
}

void
SquirrelVM::get_table_entry(std::string const& name)
{
  sq_pushstring(m_vm, name.c_str(), -1);
  if (SQ_FAILED(sq_get(m_vm, -2)))
  {
    std::ostringstream msg;
    msg << "failed to get '" << name << "' table entry";
    throw SquirrelError(m_vm, msg.str());
  }
  else
  {
    // successfully placed result on stack
  }
}

void
SquirrelVM::get_or_create_table_entry(std::string const& name)
{
  try
  {
    get_table_entry(name);
  }
  catch(std::exception&)
  {
    create_empty_table(name.c_str());
    get_table_entry(name);
  }
}

void
SquirrelVM::delete_table_entry(char const* name)
{
  sq_pushstring(m_vm, name, -1);
  if (SQ_FAILED(sq_deleteslot(m_vm, -2, false)))
  {
    // Something failed while deleting the table entry.
    // Key doesn't exist?
  }
}

void
SquirrelVM::rename_table_entry(char const* oldname, char const* newname)
{
  SQInteger oldtop = sq_gettop(m_vm);

  // push key
  sq_pushstring(m_vm, newname, -1);

  // push value and delete old oldname
  sq_pushstring(m_vm, oldname, -1);
  if (SQ_FAILED(sq_deleteslot(m_vm, oldtop, SQTrue))) {
    sq_settop(m_vm, oldtop);
    throw SquirrelError(m_vm, "Couldn't find 'oldname' entry in table");
  }

  // create new entry
  sq_createslot(m_vm, -3);

  sq_settop(m_vm, oldtop);
}

std::vector<std::string>
SquirrelVM::get_table_keys()
{
  auto old_top = sq_gettop(m_vm);
  std::vector<std::string> keys;

  sq_pushnull(m_vm);
  while (SQ_SUCCEEDED(sq_next(m_vm, -2)))
  {
    //here -1 is the value and -2 is the key
    char const* result;
    if (SQ_FAILED(sq_getstring(m_vm, -2, &result)))
    {
      throw SquirrelError(m_vm, "Couldn't get string value for key");
    }
    else
    {
      keys.push_back(result);
    }

    // pops key and val before the next iteration
    sq_pop(m_vm, 2);
  }

  sq_settop(m_vm, old_top);

  return keys;
}

HSQOBJECT
SquirrelVM::create_thread()
{
  HSQUIRRELVM new_vm = sq_newthread(m_vm, 64);
  if (new_vm == nullptr)
    throw SquirrelError(m_vm, "Couldn't create new VM");

  HSQOBJECT vm_object;
  sq_resetobject(&vm_object);
  if (SQ_FAILED(sq_getstackobj(m_vm, -1, &vm_object)))
    throw SquirrelError(m_vm, "Couldn't get squirrel thread from stack");
  sq_addref(m_vm, &vm_object);

  sq_pop(m_vm, 1);

  return vm_object;
}

/* EOF */
