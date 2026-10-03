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
#include "object/archetype_object.hpp"

#include <stdexcept>

#include "ecs/archetype.hpp"

namespace {

CollisionGroup colgroup_of(Archetype const& archetype)
{
  std::string colgroup = "moving";
  archetype.get_properties().read("colgroup", colgroup);

  if (colgroup == "moving") return COLGROUP_MOVING;
  if (colgroup == "static") return COLGROUP_STATIC;
  if (colgroup == "moving-static") return COLGROUP_MOVING_STATIC;
  if (colgroup == "moving-only-static") return COLGROUP_MOVING_ONLY_STATIC;
  if (colgroup == "touchable") return COLGROUP_TOUCHABLE;
  if (colgroup == "disabled") return COLGROUP_DISABLED;
  throw std::runtime_error("archetype '" + archetype.get_name() + "': unknown colgroup '" + colgroup + "'");
}

} // namespace

ArchetypeObject::ArchetypeObject(ReaderMapping const& reader, Archetype const& archetype) :
  MovingSprite(reader, archetype.get_sprite(), archetype.get_layer(), colgroup_of(archetype)),
  m_behaviors(archetype.emplace_object_components(get_entity(), &reader))
{
  for (auto const* behavior : m_behaviors) {
    if (behavior->read) {
      behavior->read(*this, reader);
    }
  }
  construct();
}

ArchetypeObject::ArchetypeObject(Vector const& pos, Archetype const& archetype, std::string const& sprite) :
  MovingSprite(pos, sprite.empty() ? archetype.get_sprite() : sprite, archetype.get_layer(), colgroup_of(archetype)),
  m_behaviors(archetype.emplace_object_components(get_entity(), nullptr))
{
  construct();
}

ArchetypeObject::ArchetypeObject(ReaderMapping const& reader, std::string const& sprite_name, int layer,
                                 CollisionGroup collision_group) :
  MovingSprite(reader, sprite_name, layer, collision_group),
  m_behaviors()
{
}

ArchetypeObject::ArchetypeObject(Vector const& pos, std::string const& sprite_name, int layer,
                                 CollisionGroup collision_group) :
  MovingSprite(pos, sprite_name, layer, collision_group),
  m_behaviors()
{
}

ArchetypeObject::~ArchetypeObject()
{
}

std::unique_ptr<ArchetypeObject>
ArchetypeObject::create(std::string const& name, Vector const& pos, std::string const& sprite)
{
  Archetype const* archetype = ArchetypeRegistry::instance().get(name);
  if (!archetype) {
    throw std::runtime_error("unknown archetype '" + name + "'");
  }
  return std::make_unique<ArchetypeObject>(pos, *archetype, sprite);
}

void
ArchetypeObject::construct()
{
  for (auto const* behavior : m_behaviors) {
    if (behavior->construct) {
      behavior->construct(*this);
    }
  }
}

void
ArchetypeObject::finish_construction()
{
  for (auto const* behavior : m_behaviors) {
    if (behavior->finish_construction) {
      behavior->finish_construction(*this);
    }
  }
}

void
ArchetypeObject::expose(HSQUIRRELVM vm, SQInteger table_idx)
{
  for (auto const* behavior : m_behaviors) {
    if (behavior->expose) {
      behavior->expose(*this, vm, table_idx);
    }
  }
}

void
ArchetypeObject::unexpose(HSQUIRRELVM vm, SQInteger table_idx)
{
  for (auto const* behavior : m_behaviors) {
    if (behavior->unexpose) {
      behavior->unexpose(*this, vm, table_idx);
    }
  }
}

void
ArchetypeObject::move_to(Vector const& pos)
{
  for (auto const* behavior : m_behaviors) {
    if (behavior->move_to) {
      behavior->move_to(*this, pos);
      return;
    }
  }
  MovingSprite::move_to(pos);
}

void
ArchetypeObject::update(float dt_sec)
{
  for (auto const* behavior : m_behaviors) {
    if (behavior->update) {
      behavior->update(*this, dt_sec);
    }
  }
}

void
ArchetypeObject::draw(DrawingContext& context)
{
  for (auto const* behavior : m_behaviors) {
    if (behavior->draw) {
      behavior->draw(*this, context);
      return;
    }
  }
  MovingSprite::draw(context);
}

HitResponse
ArchetypeObject::collision(GameObject& other, CollisionHit const& hit)
{
  for (auto const* behavior : m_behaviors) {
    if (behavior->collision) {
      return behavior->collision(*this, other, hit);
    }
  }
  return FORCE_MOVE;
}

bool
ArchetypeObject::collides(GameObject& other, CollisionHit const& hit) const
{
  for (auto const* behavior : m_behaviors) {
    if (behavior->collides) {
      return behavior->collides(*this, other, hit);
    }
  }
  return MovingSprite::collides(other, hit);
}

void
ArchetypeObject::hit(Player& player)
{
  for (auto const* behavior : m_behaviors) {
    if (behavior->hit) {
      behavior->hit(*this, player);
      return;
    }
  }
}

void
ArchetypeObject::collision_solid(CollisionHit const& hit)
{
  for (auto const* behavior : m_behaviors) {
    if (behavior->collision_solid) {
      behavior->collision_solid(*this, hit);
      return;
    }
  }
}

/* EOF */
