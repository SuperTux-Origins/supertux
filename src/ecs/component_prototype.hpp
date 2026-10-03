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
#ifndef HEADER_SUPERTUX_ECS_COMPONENT_PROTOTYPE_HPP
#define HEADER_SUPERTUX_ECS_COMPONENT_PROTOTYPE_HPP

#include <memory>

#include <entt/entity/registry.hpp>

#include "util/reader_mapping.hpp"

/** The value of one component as defined by an archetype. */
class ComponentPrototype
{
public:
  virtual ~ComponentPrototype() = default;

  /** Emplace a copy of the value on the entity. Keys in the optional
      level object mapping override fields, e.g. "(crystallo (radius 150))". */
  virtual void emplace(entt::registry& registry, entt::entity entity,
                       ReaderMapping const* overrides) const = 0;
};

/** Component types provide "void read_component(ReaderMapping const&, T&)",
    which must only overwrite the fields whose keys are present. */
template<typename T>
class ComponentPrototypeT final : public ComponentPrototype
{
public:
  explicit ComponentPrototypeT(T value) : m_value(std::move(value)) {}

  static std::unique_ptr<ComponentPrototype> from_reader(ReaderMapping const& mapping)
  {
    T value{};
    read_component(mapping, value);
    return std::make_unique<ComponentPrototypeT<T>>(std::move(value));
  }

  void emplace(entt::registry& registry, entt::entity entity,
               ReaderMapping const* overrides) const override
  {
    T value = m_value;
    if (overrides) {
      read_component(*overrides, value);
    }
    registry.emplace_or_replace<T>(entity, std::move(value));
  }

private:
  T m_value;
};

#endif

/* EOF */
