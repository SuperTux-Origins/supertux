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
#ifndef HEADER_SUPERTUX_OBJECT_ARCHETYPE_OBJECT_HPP
#define HEADER_SUPERTUX_OBJECT_ARCHETYPE_OBJECT_HPP

#include <memory>
#include <vector>

#include "ecs/object_behavior.hpp"
#include "ecs/registry.hpp"
#include "object/moving_sprite.hpp"
#include "squirrel/script_interface.hpp"

class Archetype;

/** Generic non-badguy object whose behavior comes from ECS components
    with ObjectBehavior handlers (ecs/object_behaviors.cpp), configured
    by data/archetypes/ with base "object". The public section below is
    the state that behavior handlers may use. */
class ArchetypeObject : public MovingSprite,
                        public virtual ScriptInterface
{
public:
  ArchetypeObject(ReaderMapping const& reader, Archetype const& archetype);
  /** for objects spawned at runtime */
  ArchetypeObject(Vector const& pos, Archetype const& archetype);
  ~ArchetypeObject() override;

  /** Spawn the named archetype at runtime */
  static std::unique_ptr<ArchetypeObject> create(std::string const& name, Vector const& pos);

  void finish_construction() override;
  void expose(HSQUIRRELVM vm, SQInteger table_idx) override;
  void unexpose(HSQUIRRELVM vm, SQInteger table_idx) override;
  void move_to(Vector const& pos) override;
  void update(float dt_sec) override;
  void draw(DrawingContext& context) override;
  HitResponse collision(GameObject& other, CollisionHit const& hit) override;
  void collision_solid(CollisionHit const& hit) override;
  bool collides(GameObject& other, CollisionHit const& hit) const override;

  /** Hit by the player, e.g. a block from below */
  void hit(Player& player);

  /** MovingSprite::draw(), for behaviors that wrap it */
  void default_draw(DrawingContext& context) { MovingSprite::draw(context); }

  // state available to behaviors
  using MovingSprite::m_sprite;
  using MovingSprite::m_sprite_name;
  using MovingSprite::m_default_sprite_name;
  using MovingSprite::m_layer;
  using MovingSprite::m_flip;
  using MovingSprite::set_action;
  using MovingObject::m_col;
  using MovingObject::set_group;

protected:
  /** For hand-written subclasses that add their behaviors in code */
  ArchetypeObject(ReaderMapping const& reader, std::string const& sprite_name, int layer,
                  CollisionGroup collision_group);
  ArchetypeObject(Vector const& pos, std::string const& sprite_name, int layer,
                  CollisionGroup collision_group);

  /** Emplace a behavior component; behaviors run in the order added.
      The read handler runs if a level object mapping is given. */
  template<typename T>
  T& add_behavior(T value, ReaderMapping const* reader = nullptr)
  {
    T& component = ecs::emplace<T>(get_entity(), std::move(value));
    ObjectBehavior const& behavior = object_behavior_of<T>();
    m_behaviors.push_back(&behavior);
    if (reader && behavior.read) {
      behavior.read(*this, *reader);
    }
    if (behavior.construct) {
      behavior.construct(*this);
    }
    return component;
  }

  std::vector<ObjectBehavior const*> const& get_behaviors() const { return m_behaviors; }

private:
  void construct();

private:
  std::vector<ObjectBehavior const*> m_behaviors;

private:
  ArchetypeObject(ArchetypeObject const&) = delete;
  ArchetypeObject& operator=(ArchetypeObject const&) = delete;
};

#endif

/* EOF */
