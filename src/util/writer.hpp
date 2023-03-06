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

#ifndef HEADER_SUPERTUX_UTIL_WRITER_HPP
#define HEADER_SUPERTUX_UTIL_WRITER_HPP

#include <string>
#include <vector>

namespace sexp {
class Value;
} // namespace sexp

class Writer final
{
public:
  Writer(std::string const& filename);
  Writer(std::ostream& out);
  ~Writer();

  void write_comment(std::string const& comment);

  void start_list(std::string const& listname, bool string = false);

  void write(std::string const& name, bool value);
  void write(std::string const& name, int value);
  void write(std::string const& name, float value);
  void write(std::string const& name, char const* value);
  void write(std::string const& name, std::string const& value, bool translatable = false);
  void write(std::string const& name, std::vector<int> const& value);
  void write(std::string const& name, const std::vector<unsigned int>& value, int width = 0);
  void write(std::string const& name, std::vector<float> const& value);
  void write(std::string const& name, std::vector<std::string> const& value);
  void write(std::string const& name, sexp::Value const& value);
  // add more write-functions when needed...

  void end_list(std::string const& listname);

private:
  void write_escaped_string(std::string const& str);
  void write_sexp(sexp::Value const& value, bool fudge);
  void indent();

private:
  std::string m_filename;
  std::ostream* out;
  bool out_owned;
  int indent_depth;
  std::vector<std::string> lists;

private:
  Writer(Writer const&) = delete;
  Writer & operator=(Writer const&) = delete;
};

#endif

/* EOF */
