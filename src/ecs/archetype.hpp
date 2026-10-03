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
#ifndef HEADER_SUPERTUX_ECS_ARCHETYPE_HPP
#define HEADER_SUPERTUX_ECS_ARCHETYPE_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ecs/badguy_behavior.hpp"
#include "ecs/component_prototype.hpp"
#include "util/reader_document.hpp"

/** A data-defined object type, loaded from data/archetypes/*.archetype:

      (supertux-archetype
        (name "crystallo")
        (aliases "crystal")                ; optional, extra factory names
        (base "badguy")
        (properties
          (sprite "images/creatures/crystallo/crystallo.sprite")
          (flammable #f))
        (components
          (walker (speed 80) (max-drop-height 16))
          (patrol (radius 100) (acceleration 2) (slowdown-action "slowdown"))
          (squish-reaction (action "shattered") (anchor-bottom #t))))

    "base" selects the C++ shell class and "properties" configure it.
    "components" go into the ECS registry; a level object can override
    component fields with top-level keys, e.g. (crystallo (radius 150)). */
class Archetype final
{
public:
  Archetype(ReaderMapping const& mapping);

  std::string const& get_name() const { return m_name; }
  std::string const& get_base() const { return m_base; }
  std::vector<std::string> const& get_aliases() const { return m_aliases; }

  /** Base properties of the shell class (sprite, walk-speed, ...) */
  ReaderMapping const& get_properties() const { return m_properties; }

  /** Emplace the archetype's components on an entity, with fields
      overridden by the level object's mapping if given. Returns the
      behaviors of those components, in archetype order. */
  std::vector<BadGuyBehavior const*> emplace_components(entt::entity entity, ReaderMapping const* overrides) const;

private:
  std::string m_name;
  std::string m_base;
  std::vector<std::string> m_aliases;
  ReaderMapping m_properties;
  struct Component
  {
    std::unique_ptr<ComponentPrototype> prototype;
    BadGuyBehavior const* behavior;
  };
  std::vector<Component> m_components;

private:
  Archetype(Archetype const&) = delete;
  Archetype& operator=(Archetype const&) = delete;
};

/** All archetypes found in data/archetypes/. */
class ArchetypeRegistry final
{
public:
  static ArchetypeRegistry& instance();

  /** Returns nullptr if there is no archetype of that name */
  Archetype const* get(std::string const& name) const;

  std::vector<Archetype const*> get_archetypes() const;

private:
  ArchetypeRegistry();

  void load(std::string const& filename);

private:
  /** keeps the parsed documents alive, Archetype holds ReaderMappings into them */
  std::vector<std::unique_ptr<ReaderDocument>> m_documents;
  std::map<std::string, std::unique_ptr<Archetype>> m_archetypes;

private:
  ArchetypeRegistry(ArchetypeRegistry const&) = delete;
  ArchetypeRegistry& operator=(ArchetypeRegistry const&) = delete;
};

#endif

/* EOF */
