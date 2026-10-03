//  SuperTux
//  Copyright (C) 2026 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_SUPERTUX_ECS_RUNTIME_STATE_HPP
#define HEADER_SUPERTUX_ECS_RUNTIME_STATE_HPP

/** State that only exists at runtime and cannot be copied (owned
    objects). Copies, i.e. archetype prototypes, start out empty. */
template<typename T>
struct RuntimeState
{
  T value = {};

  RuntimeState() = default;
  RuntimeState(RuntimeState const&) : value() {}
  RuntimeState& operator=(RuntimeState const&) { value = T(); return *this; }
  RuntimeState(RuntimeState&&) = default;
  RuntimeState& operator=(RuntimeState&&) = default;
};

#endif

/* EOF */
