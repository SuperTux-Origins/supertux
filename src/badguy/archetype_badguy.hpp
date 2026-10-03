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

#include "badguy/badguy.hpp"
#include "ecs/badguy_behavior.hpp"
#include "ecs/registry.hpp"

class Archetype;

/** Generic badguy whose behavior comes from ECS components with
    BadGuyBehavior handlers (ecs/badguy_behaviors.cpp), configured by
    data/archetypes/ instead of a subclass per enemy.

    Remaining hand-written badguys can derive from it as well and add
    behaviors in code (see WalkingBadguy). The public section below is
    the BadGuy state that behavior handlers may use. */
class ArchetypeBadguy : public BadGuy
{
public:
  ArchetypeBadguy(ReaderMapping const& reader, Archetype const& archetype);
  /** sprite overrides the archetype's sprite if not empty */
  ArchetypeBadguy(Vector const& pos, Direction dir, Archetype const& archetype,
                  std::string const& dead_script = {}, std::string const& sprite = {});
  ~ArchetypeBadguy() override;

  /** Spawn the named archetype at runtime, e.g. a snowball from a snowman */
  static std::unique_ptr<ArchetypeBadguy> create(std::string const& name, Vector const& pos, Direction dir,
                                                 std::string const& dead_script = {},
                                                 std::string const& sprite = {});

  bool is_freezable() const override { return m_freezable; }
  bool is_flammable() const override { return m_flammable; }
  void freeze() override;
  void ignite() override;
  void kill_fall() override;
  bool is_portable() const override;
  void grab(MovingObject& object, Vector const& pos, Direction dir) override;
  void ungrab(MovingObject& object, Direction dir) override;
  HitResponse collision(GameObject& other, CollisionHit const& hit) override;
  void stop_looping_sounds() override;
  void play_looping_sounds() override;

  void collision_solid(CollisionHit const& hit) override;

  /** The BadGuy implementations, for behaviors that fall back to them */
  void default_collision_solid(CollisionHit const& hit) { BadGuy::collision_solid(hit); }
  bool default_collision_squished(GameObject& object) { return BadGuy::collision_squished(object); }
  void default_freeze() { BadGuy::freeze(); }
  void default_ignite() { BadGuy::ignite(); }
  void default_kill_fall();
  HitResponse default_collision(GameObject& other, CollisionHit const& hit) { return BadGuy::collision(other, hit); }
  HitResponse default_collision_player(Player& player, CollisionHit const& hit) { return BadGuy::collision_player(player, hit); }

  // BadGuy state available to behaviors
  using BadGuy::State;
  using BadGuy::STATE_INIT;
  using BadGuy::STATE_INACTIVE;
  using BadGuy::STATE_ACTIVE;
  using BadGuy::m_physic;
  using BadGuy::m_dir;
  using BadGuy::m_frozen;
  using BadGuy::m_ignited;
  using BadGuy::m_start_position;
  using BadGuy::get_state;
  using BadGuy::get_nearest_player;
  using BadGuy::is_active;
  using BadGuy::might_fall;
  using BadGuy::on_ground;
  using BadGuy::update_on_ground_flag;
  using BadGuy::kill_squished;
  using BadGuy::run_dead_script;
  using MovingSprite::m_sprite;
  using MovingSprite::m_sprite_name;
  using BadGuy::set_colgroup_active;
  using MovingSprite::set_action;
  using MovingObject::m_col;
  using MovingObject::set_group;

protected:
  /** For hand-written subclasses that add their behaviors in code */
  ArchetypeBadguy(ReaderMapping const& reader, std::string const& sprite_name, int layer,
                  std::string const& light_sprite_name);
  ArchetypeBadguy(Vector const& pos, std::string const& sprite_name, int layer,
                  std::string const& light_sprite_name);
  ArchetypeBadguy(Vector const& pos, Direction dir, std::string const& sprite_name, int layer,
                  std::string const& light_sprite_name);

  /** Emplace a behavior component; behaviors run in the order added */
  template<typename T>
  T& add_behavior(T value)
  {
    T& component = ecs::emplace<T>(get_entity(), std::move(value));
    m_behaviors.push_back(&behavior_of<T>());
    return component;
  }

  void initialize() override;
  void activate() override;
  void deactivate() override;
  void active_update(float dt_sec) override;
  HitResponse collision_player(Player& player, CollisionHit const& hit) override;
  HitResponse collision_badguy(BadGuy& other, CollisionHit const& hit) override;
  bool collision_squished(GameObject& object) override;

private:
  void read_properties(Archetype const& archetype);
  void construct();

private:
  std::vector<BadGuyBehavior const*> m_behaviors;
  bool m_freezable;
  bool m_flammable;
  bool m_fall_immune;

private:
  ArchetypeBadguy(ArchetypeBadguy const&) = delete;
  ArchetypeBadguy& operator=(ArchetypeBadguy const&) = delete;
};

#endif

/* EOF */
