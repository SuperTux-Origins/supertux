// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Replacement for fmt::streamed(T) — convert operator<< types for std::format.

#ifndef HEADER_SUPERTUX_UTIL_STREAM_FORMAT_HPP
#define HEADER_SUPERTUX_UTIL_STREAM_FORMAT_HPP

#include <sstream>
#include <string>

namespace supertux {

/** Convert a streamable value to std::string for use with std::format / logmich. */
template<typename T>
std::string stream_str(T const& value)
{
  std::ostringstream out;
  out << value;
  return out.str();
}

} // namespace supertux

#endif

/* EOF */
