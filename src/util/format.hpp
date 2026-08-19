// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
//
// std::format helpers replacing libfmt.  Runtime format strings (gettext)
// use std::vformat; compile-time strings use std::format.

#ifndef HEADER_SUPERTUX_UTIL_FORMAT_HPP
#define HEADER_SUPERTUX_UTIL_FORMAT_HPP

#include <format>
#include <string>
#include <string_view>
#include <utility>

namespace supertux {

/** Format with a compile-time format string (type-checked). */
template<class... Args>
std::string format(std::format_string<Args...> fmt, Args&&... args)
{
  return std::format(fmt, std::forward<Args>(args)...);
}

/** Format with a runtime format string (e.g. gettext `_("… {} …")`).
 *  Prefer this over std::format when the format string is not a literal. */
template<class... Args>
std::string format_rt(std::string_view fmt, Args&&... args)
{
  return std::vformat(fmt, std::make_format_args(args...));
}

} // namespace supertux

#endif

/* EOF */
