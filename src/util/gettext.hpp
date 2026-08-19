//  SuperTux
//  Copyright (C) 2006 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_SUPERTUX_UTIL_GETTEXT_HPP
#define HEADER_SUPERTUX_UTIL_GETTEXT_HPP

#include <memory>

/*
 * If you need to do a nontrivial substitution of values into a pattern, use
 * supertux::format_rt (runtime / gettext) or std::format (compile-time) rather
 * than ad-hoc concatenation.  That way, translators can translate the format
 * string as a whole (and rearrange values if needed) instead of multiple pieces.
 *
 * Bad:
 *     std::string greeting = _("Hello ") + name + _("!");
 * Good:
 *     #include "util/format.hpp"
 *     std::string greeting = supertux::format_rt(_("Hello {}!"), name);
 *
 * Plural forms: use __ instead of _ with supertux::format_rt when needed.
 * https://www.gnu.org/software/gettext/manual/html_node/Plural-forms.html
 *
 * Bad:
 *     std::cout << _("You collected ") << num << _(" coins");
 * Good:
 *     #include "util/format.hpp"
 *     std::cout << supertux::format_rt(__("You collected {} coin",
 *                                         "You collected {} coins", num), num);
 */


static inline std::string _(std::string const& message)
{
  return message;
}

static inline std::string __(std::string const& message,
                             std::string const& message_plural, int num)
{
  if (num == 1) {
    return message;
  } else {
    return message_plural;
  }
}

#endif

/* EOF */
