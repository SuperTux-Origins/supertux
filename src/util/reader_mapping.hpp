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

#ifndef HEADER_SUPERTUX_UTIL_READER_MAPPING_HPP
#define HEADER_SUPERTUX_UTIL_READER_MAPPING_HPP

#include <prio/reader_mapping.hpp>

#include "video/color.hpp"

using prio::ReaderMapping;

namespace prio {

template<>
inline bool read_custom(ReaderMapping const& mapping, std::string_view key, std::vector<uint32_t>& value)
{
  std::vector<int> tmpvalues;
  if (!mapping.read(key, tmpvalues)) {
    return false;
  } else {
    value.reserve(tmpvalues.size());
    std::transform(tmpvalues.begin(), tmpvalues.end(), std::back_inserter(value),
                   [](int v) { return static_cast<uint32_t>(v); });
    return true;
  }
}

template<>
inline bool read_custom(ReaderMapping const& mapping, std::string_view key, uint32_t& value)
{
  int tmp;
  if (!mapping.read(key, tmp)) {
    return false;
  } else {
    value = static_cast<uint32_t>(tmp);
    return true;
  }
}

template<>
inline bool read_custom(ReaderMapping const& mapping, std::string_view key, Color& color)
{
  std::vector<float> values;
  if (!mapping.read(key, values)) {
    return false;
  } else {
    if (values.size() == 3) {
      color = Color(values[0], values[1], values[2]);
      return true;
    } else if (values.size() == 4) {
      color = Color(values[0], values[1], values[2], values[3]);
      return true;
    } else {
      return false;
    }
  }
}

} // namespace prio

#endif

/* EOF */
