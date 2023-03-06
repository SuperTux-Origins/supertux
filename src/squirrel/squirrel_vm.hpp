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

#ifndef HEADER_SUPERTUX_SQUIRREL_SQUIRREL_VM_HPP
#define HEADER_SUPERTUX_SQUIRREL_SQUIRREL_VM_HPP

#include <string>
#include <vector>

#include <squirrel.h>

/** Basic wrapper around HSQUIRRELVM with some utility functions, not
    to be confused with SquirrelVirtualMachine. The classes might be
    merged in the future. */
class SquirrelVM
{
public:
  SquirrelVM();
  ~SquirrelVM();

  HSQUIRRELVM get_vm() const { return m_vm; }

  void begin_table(char const* name);
  void end_table(char const* name);

  /** Creates an empty table with given name
      @param vm VM to create table on
      @param name Name of the table to create */
  void create_empty_table(char const* name);

  bool has_property(char const* name);

  void store_bool(char const* name, bool val);
  void store_int(char const* name, int val);
  void store_float(char const* name, float val);
  void store_string(char const* name, std::string const& val);
  void store_object(char const* name, HSQOBJECT const& val);

  bool get_bool(char const* name, bool& val);
  bool get_int(char const* name, int& val);
  bool get_float(char const* name, float& val);
  bool get_string(char const* name, std::string& val);

  bool read_bool(char const* name);
  int read_int(char const* name);
  float read_float(char const* name);
  std::string read_string(char const* name);

  void get_table_entry(std::string const& name);
  void get_or_create_table_entry(std::string const& name);
  void delete_table_entry(char const* name);
  void rename_table_entry(char const* oldname, char const* newname);
  std::vector<std::string> get_table_keys();

  HSQOBJECT create_thread();

private:
  HSQUIRRELVM m_vm;

private:
  SquirrelVM(SquirrelVM const&) = delete;
  SquirrelVM& operator=(SquirrelVM const&) = delete;
};

#endif

/* EOF */
