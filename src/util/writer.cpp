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

#include "util/writer.hpp"

#include <sexp/io.hpp>

#include "physfs/ofile_stream.hpp"
#include "util/log.hpp"

Writer::Writer(std::string const& filename) :
  m_filename(filename),
  out(new OFileStream(filename)),
  out_owned(true),
  indent_depth(0),
  lists()
{
  out->precision(7);
}

Writer::Writer(std::ostream& newout) :
  m_filename("<stream>"),
  out(&newout),
  out_owned(false),
  indent_depth(0),
  lists()
{
  out->precision(7);
}

Writer::~Writer()
{
  if (lists.size() > 0) {
    log_warning("{}: Not all sections closed in Writer", m_filename);
  }
  if (out_owned)
    delete out;
}

void
Writer::write_comment(std::string const& comment)
{
  *out << "; " << comment << "\n";
}

void
Writer::start_list(std::string const& listname, bool string)
{
  indent();
  *out << '(';
  if (string)
    write_escaped_string(listname);
  else
    *out << listname;
  *out << '\n';
  indent_depth += 2;

  lists.push_back(listname);
}

void
Writer::end_list(std::string const& listname)
{
  if (lists.size() == 0) {
    log_warning("{}: Trying to close list '{}', which is not open", m_filename, listname);
    return;
  }
  if (lists.back() != listname) {
    log_warning("{}: trying to close list '{}' while list '{}' is open", m_filename, listname, lists.back());
    return;
  }
  lists.pop_back();

  indent_depth -= 2;
  indent();
  *out << ")\n";
}

void
Writer::write(std::string const& name, int value)
{
  indent();
  *out << '(' << name << ' ' << value << ")\n";
}

void
Writer::write(std::string const& name, float value)
{
  indent();
  *out << '(' << name << ' ' << value << ")\n";
}

/** This function is needed to properly resolve the overloaded write()
    function, without it the call write("foo", "bar") would call
    write(name, bool), not write(name, string, bool) */
void
Writer::write(std::string const& name, char const* value)
{
  write(name, value, false);
}

void
Writer::write(std::string const& name, std::string const& value,
              bool translatable)
{
  indent();
  *out << '(' << name;
  if (translatable) {
    *out << " (_ ";
    write_escaped_string(value);
    *out << "))\n";
  } else {
    *out << " ";
    write_escaped_string(value);
    *out << ")\n";
  }
}

void
Writer::write(std::string const& name, bool value)
{
  indent();
  *out << '(' << name << ' ' << (value ? "#t" : "#f") << ")\n";
}

void
Writer::write(std::string const& name,
              std::vector<int> const& value)
{
  indent();
  *out << '(' << name;
  for (auto const& i : value)
    *out << " " << i;
  *out << ")\n";
}

void
Writer::write(std::string const& name,
              const std::vector<unsigned int>& value,
              int width)
{
  indent();
  *out << '(' << name;
  if (!width)
  {
    for (auto const& i : value)
      *out << " " << i;
  }
  else
  {
    *out << "\n";
    indent();
    int count = 0;
    for (auto const& i : value) {
      *out << i;
      count += 1;
      if (count >= width) {
        *out << "\n";
        indent();
        count = 0;
      } else {
        *out << " ";
      }
    }
  }
  *out << ")\n";
}

void
Writer::write(std::string const& name,
              std::vector<float> const& value)
{
  indent();
  *out << '(' << name;
  for (auto const& i : value)
    *out << " " << i;
  *out << ")\n";
}

void
Writer::write(std::string const& name,
              std::vector<std::string> const& value)
{
  indent();
  *out << '(' << name;
  for (auto const& i : value) {
    *out << " ";
    write_escaped_string(i);
  }
  *out << ")\n";
}

void
Writer::write_sexp(sexp::Value const& value, bool fudge)
{
  if (value.is_array()) {
    if (fudge) {
      indent_depth -= 1;
      indent();
      indent_depth += 1;
    } else {
      indent();
    }
    *out << "(";
    auto& arr = value.as_array();
    for(size_t i = 0; i < arr.size(); ++i) {
      write_sexp(arr[i], false);
      if (i != arr.size() - 1) {
        *out << " ";
      }
    }
    *out << ")\n";
  } else {
    *out << value;
  }
}

void
Writer::write(std::string const& name, sexp::Value const& value)
{
  indent();
  *out << '(' << name << "\n";
  indent_depth += 4;
  write_sexp(value, true);
  indent_depth -= 4;
  indent();
  *out << ")\n";
}

void
Writer::write_escaped_string(std::string const& str)
{
  *out << '"';
  for (char const* c = str.c_str(); *c != 0; ++c) {
    if (*c == '\"')
      *out << "\\\"";
    else if (*c == '\\')
      *out << "\\\\";
    else
      *out << *c;
  }
  *out << '"';
}

void
Writer::indent()
{
  for (int i = 0; i<indent_depth; ++i)
    *out << ' ';
}

/* EOF */
