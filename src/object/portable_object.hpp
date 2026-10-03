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
#ifndef HEADER_SUPERTUX_OBJECT_PORTABLE_OBJECT_HPP
#define HEADER_SUPERTUX_OBJECT_PORTABLE_OBJECT_HPP

#include "object/archetype_object.hpp"
#include "object/portable.hpp"

/** An ArchetypeObject that the player can carry (base "portable"),
    e.g. rocks and trampolines. Kept separate from ArchetypeObject, as
    some code treats every Portable as something being carried. */
class PortableObject : public ArchetypeObject,
                       public Portable
{
public:
  PortableObject(ReaderMapping const& reader, Archetype const& archetype);
  PortableObject(Vector const& pos, Archetype const& archetype);

  /** Spawn the named archetype at runtime */
  static std::unique_ptr<PortableObject> create(std::string const& name, Vector const& pos);

  bool is_portable() const override;
  void grab(MovingObject& object, Vector const& pos, Direction dir) override;
  void ungrab(MovingObject& object, Direction dir) override;

  /** The Portable implementations, for behaviors that extend them */
  void default_grab(MovingObject& object, Vector const& pos, Direction dir) { Portable::grab(object, pos, dir); }
  void default_ungrab(MovingObject& object, Direction dir) { Portable::ungrab(object, dir); }

protected:
  PortableObject(ReaderMapping const& reader, std::string const& sprite_name, int layer,
                 CollisionGroup collision_group);
  PortableObject(Vector const& pos, std::string const& sprite_name, int layer,
                 CollisionGroup collision_group);

private:
  PortableObject(PortableObject const&) = delete;
  PortableObject& operator=(PortableObject const&) = delete;
};

#endif

/* EOF */
