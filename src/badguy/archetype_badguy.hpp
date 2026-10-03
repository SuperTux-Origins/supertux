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
#ifndef HEADER_SUPERTUX_BADGUY_ARCHETYPE_BADGUY_HPP
#define HEADER_SUPERTUX_BADGUY_ARCHETYPE_BADGUY_HPP

#include <memory>
#include <vector>

#include "badguy/walking_badguy.hpp"
#include "ecs/registry.hpp"

class Archetype;
struct Floater;

/** Generic shell for data-defined walking badguys ("base" =
    "walking-badguy" in data/archetypes/). Behavior beyond plain walking
    comes from ECS components (ecs/badguy_components.hpp), not subclasses. */
class ArchetypeBadguy final : public WalkingBadguy
{
public:
  ArchetypeBadguy(ReaderMapping const& reader, Archetype const& archetype);
  ArchetypeBadguy(Vector const& pos, Direction dir, Archetype const& archetype,
                  std::string const& dead_script = {});
  ~ArchetypeBadguy() override;

  /** Spawn the named archetype at runtime, e.g. a snowball from a snowman */
  static std::unique_ptr<ArchetypeBadguy> create(std::string const& name, Vector const& pos, Direction dir,
                                                 std::string const& dead_script = {});

  void active_update(float dt_sec) override;
  bool is_freezable() const override { return m_freezable; }
  bool is_flammable() const override { return m_flammable; }

protected:
  bool collision_squished(GameObject& object) override;

private:
  void read_properties(Archetype const& archetype);

  template<typename T>
  T const* find() const { return ecs::try_get<T>(get_entity()); }

  void update_floater(Floater const& floater);

private:
  bool m_freezable;
  bool m_flammable;

private:
  ArchetypeBadguy(ArchetypeBadguy const&) = delete;
  ArchetypeBadguy& operator=(ArchetypeBadguy const&) = delete;
};

#endif

/* EOF */
