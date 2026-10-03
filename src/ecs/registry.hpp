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
#ifndef HEADER_SUPERTUX_ECS_REGISTRY_HPP
#define HEADER_SUPERTUX_ECS_REGISTRY_HPP

#include <entt/entity/registry.hpp>

namespace ecs {

/** The registry holding the entities of all GameObjects. Each GameObject
    creates its entity on construction, so components can be emplaced in
    constructors, before the object is added to a Sector. */
entt::registry& registry();

template<typename T, typename... Args>
T& emplace(entt::entity entity, Args&&... args)
{
  return registry().emplace<T>(entity, std::forward<Args>(args)...);
}

template<typename T>
T& get(entt::entity entity)
{
  return registry().get<T>(entity);
}

template<typename T>
T* try_get(entt::entity entity)
{
  return registry().try_get<T>(entity);
}

} // namespace ecs

#endif

/* EOF */
