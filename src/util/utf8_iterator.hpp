//  SuperTux
//  Copyright (C) 2006 Matthias Braun <matze@braunis.de>
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

#ifndef HEADER_SUPERTUX_UTIL_UTF8_ITERATOR_HPP
#define HEADER_SUPERTUX_UTIL_UTF8_ITERATOR_HPP

#include <stdint.h>
#include <string>

class UTF8Iterator
{
public:
  std::string const&     text;
  std::string::size_type pos;
  uint32_t chr;

  UTF8Iterator(std::string const& text_);

  bool done() const;
  UTF8Iterator& operator++();
  uint32_t operator*() const;
};

#endif

/* EOF */
