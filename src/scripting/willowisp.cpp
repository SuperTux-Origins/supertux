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

#include "scripting/willowisp.hpp"

#include "badguy/archetype_badguy.hpp"
#include "ecs/badguy_behaviors.hpp"

namespace scripting {

void
WillOWisp::goto_node(int node_no)
{
  SCRIPT_GUARD_VOID_T(ArchetypeBadguy);
  willowisp::goto_node(object, node_no);
}

void
WillOWisp::set_state(std::string const& state)
{
  SCRIPT_GUARD_VOID_T(ArchetypeBadguy);
  willowisp::set_state(object, state);
}

void
WillOWisp::start_moving()
{
  SCRIPT_GUARD_VOID_T(ArchetypeBadguy);
  willowisp::start_moving(object);
}

void
WillOWisp::stop_moving()
{
  SCRIPT_GUARD_VOID_T(ArchetypeBadguy);
  willowisp::stop_moving(object);
}

} // namespace scripting

/* EOF */
