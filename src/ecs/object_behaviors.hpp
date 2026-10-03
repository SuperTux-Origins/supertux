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
#ifndef HEADER_SUPERTUX_ECS_OBJECT_BEHAVIORS_HPP
#define HEADER_SUPERTUX_ECS_OBJECT_BEHAVIORS_HPP

#include "ecs/object_behavior.hpp"
#include "ecs/object_components.hpp"

template<> ObjectBehavior const& object_behavior_of<UnstableTile>();
template<> ObjectBehavior const& object_behavior_of<WeakBlock>();
template<> ObjectBehavior const& object_behavior_of<MagicBlock>();
template<> ObjectBehavior const& object_behavior_of<ResetPoint>();

namespace weak_block {

/** Set a weak block on fire (or melting), e.g. by an explosion */
void start_burning(ArchetypeObject& self);

} // namespace weak_block

#endif

/* EOF */
