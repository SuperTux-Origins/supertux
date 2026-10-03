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
#include "badguy/archetype_badguy.hpp"

#include "ecs/archetype.hpp"

namespace {

std::string sprite_of(Archetype const& archetype)
{
  std::string sprite;
  if (!archetype.get_properties().read("sprite", sprite)) {
    throw std::runtime_error("archetype '" + archetype.get_name() + "' has no sprite");
  }
  return sprite;
}

constexpr char const* default_light_sprite = "images/objects/lightmap_light/lightmap_light-medium.sprite";

} // namespace

ArchetypeBadguy::ArchetypeBadguy(ReaderMapping const& reader, Archetype const& archetype) :
  BadGuy(reader, sprite_of(archetype), LAYER_OBJECTS, default_light_sprite),
  m_behaviors(),
  m_freezable(false),
  m_flammable(true)
{
  read_properties(archetype);
  m_behaviors = archetype.emplace_components(get_entity(), &reader);
}

ArchetypeBadguy::ArchetypeBadguy(Vector const& pos, Direction dir, Archetype const& archetype,
                                 std::string const& dead_script) :
  BadGuy(pos, dir, sprite_of(archetype), LAYER_OBJECTS, default_light_sprite),
  m_behaviors(),
  m_freezable(false),
  m_flammable(true)
{
  m_dead_script = dead_script;
  read_properties(archetype);
  m_behaviors = archetype.emplace_components(get_entity(), nullptr);
}

ArchetypeBadguy::ArchetypeBadguy(ReaderMapping const& reader, std::string const& sprite_name, int layer,
                                 std::string const& light_sprite_name) :
  BadGuy(reader, sprite_name, layer, light_sprite_name),
  m_behaviors(),
  m_freezable(false),
  m_flammable(true)
{
}

ArchetypeBadguy::ArchetypeBadguy(Vector const& pos, std::string const& sprite_name, int layer,
                                 std::string const& light_sprite_name) :
  BadGuy(pos, sprite_name, layer, light_sprite_name),
  m_behaviors(),
  m_freezable(false),
  m_flammable(true)
{
}

ArchetypeBadguy::ArchetypeBadguy(Vector const& pos, Direction dir, std::string const& sprite_name, int layer,
                                 std::string const& light_sprite_name) :
  BadGuy(pos, dir, sprite_name, layer, light_sprite_name),
  m_behaviors(),
  m_freezable(false),
  m_flammable(true)
{
}

ArchetypeBadguy::~ArchetypeBadguy()
{
}

std::unique_ptr<ArchetypeBadguy>
ArchetypeBadguy::create(std::string const& name, Vector const& pos, Direction dir,
                        std::string const& dead_script)
{
  Archetype const* archetype = ArchetypeRegistry::instance().get(name);
  if (!archetype) {
    throw std::runtime_error("unknown archetype '" + name + "'");
  }
  return std::make_unique<ArchetypeBadguy>(pos, dir, *archetype, dead_script);
}

void
ArchetypeBadguy::read_properties(Archetype const& archetype)
{
  ReaderMapping const& props = archetype.get_properties();
  props.read("freezable", m_freezable);
  props.read("flammable", m_flammable);
}

void
ArchetypeBadguy::initialize()
{
  for (auto const* behavior : m_behaviors) {
    if (behavior->initialize) {
      behavior->initialize(*this);
    }
  }
}

void
ArchetypeBadguy::active_update(float dt_sec)
{
  for (auto const* behavior : m_behaviors) {
    if (behavior->update) {
      behavior->update(*this, dt_sec);
    }
  }

  BadGuy::active_update(dt_sec);

  for (auto const* behavior : m_behaviors) {
    if (behavior->after_move) {
      behavior->after_move(*this, dt_sec);
    }
  }
}

void
ArchetypeBadguy::collision_solid(CollisionHit const& hit)
{
  for (auto const* behavior : m_behaviors) {
    if (behavior->collision_solid) {
      behavior->collision_solid(*this, hit);
      return;
    }
  }
  BadGuy::collision_solid(hit);
}

HitResponse
ArchetypeBadguy::collision_badguy(BadGuy& other, CollisionHit const& hit)
{
  for (auto const* behavior : m_behaviors) {
    if (behavior->collision_badguy) {
      return behavior->collision_badguy(*this, other, hit);
    }
  }
  return BadGuy::collision_badguy(other, hit);
}

bool
ArchetypeBadguy::collision_squished(GameObject& object)
{
  for (auto const* behavior : m_behaviors) {
    if (behavior->collision_squished) {
      return behavior->collision_squished(*this, object);
    }
  }
  return BadGuy::collision_squished(object);
}

/* EOF */
