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
#include "object/portable_object.hpp"

#include <stdexcept>

#include "ecs/archetype.hpp"

PortableObject::PortableObject(ReaderMapping const& reader, Archetype const& archetype) :
  ArchetypeObject(reader, archetype)
{
}

PortableObject::PortableObject(Vector const& pos, Archetype const& archetype) :
  ArchetypeObject(pos, archetype)
{
}

PortableObject::PortableObject(ReaderMapping const& reader, std::string const& sprite_name, int layer,
                               CollisionGroup collision_group) :
  ArchetypeObject(reader, sprite_name, layer, collision_group)
{
}

PortableObject::PortableObject(Vector const& pos, std::string const& sprite_name, int layer,
                               CollisionGroup collision_group) :
  ArchetypeObject(pos, sprite_name, layer, collision_group)
{
}

std::unique_ptr<PortableObject>
PortableObject::create(std::string const& name, Vector const& pos)
{
  Archetype const* archetype = ArchetypeRegistry::instance().get(name);
  if (!archetype) {
    throw std::runtime_error("unknown archetype '" + name + "'");
  }
  return std::make_unique<PortableObject>(pos, *archetype);
}

bool
PortableObject::is_portable() const
{
  for (auto const* behavior : get_behaviors()) {
    if (behavior->is_portable) {
      return behavior->is_portable(*this);
    }
  }
  return Portable::is_portable();
}

void
PortableObject::grab(MovingObject& object, Vector const& pos, Direction dir)
{
  for (auto const* behavior : get_behaviors()) {
    if (behavior->grab) {
      behavior->grab(*this, object, pos, dir);
      return;
    }
  }
  Portable::grab(object, pos, dir);
}

void
PortableObject::ungrab(MovingObject& object, Direction dir)
{
  for (auto const* behavior : get_behaviors()) {
    if (behavior->ungrab) {
      behavior->ungrab(*this, object, dir);
      return;
    }
  }
  Portable::ungrab(object, dir);
}

/* EOF */
