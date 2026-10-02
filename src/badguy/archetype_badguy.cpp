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

#include <entt/entity/registry.hpp>

#include "ecs/archetype.hpp"
#include "ecs/badguy_components.hpp"
#include "sprite/sprite.hpp"
#include "supertux/sector.hpp"

namespace {

std::string sprite_of(Archetype const& archetype)
{
  std::string sprite;
  if (!archetype.get_properties().read("sprite", sprite)) {
    throw std::runtime_error("archetype '" + archetype.get_name() + "' has no sprite");
  }
  return sprite;
}

char const* dir_suffix(Direction dir)
{
  return dir == Direction::LEFT ? "-left" : "-right";
}

} // namespace

ArchetypeBadguy::ArchetypeBadguy(ReaderMapping const& reader, Archetype const& archetype) :
  WalkingBadguy(reader, sprite_of(archetype), "left", "right"),
  m_prototypes(archetype.resolve_components(reader)),
  m_registry(nullptr),
  m_freezable(false),
  m_flammable(true)
{
  read_properties(archetype);
}

ArchetypeBadguy::ArchetypeBadguy(Vector const& pos, Direction dir, Archetype const& archetype,
                                 std::string const& dead_script) :
  WalkingBadguy(pos, dir, sprite_of(archetype), "left", "right"),
  m_prototypes(archetype.clone_components()),
  m_registry(nullptr),
  m_freezable(false),
  m_flammable(true)
{
  m_dead_script = dead_script;
  read_properties(archetype);
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
  props.read("walk-speed", walk_speed);
  props.read("max-drop-height", max_drop_height);
  props.read("freezable", m_freezable);
  props.read("flammable", m_flammable);
}

void
ArchetypeBadguy::create_components(entt::registry& registry)
{
  m_registry = &registry;
  for (auto const& prototype : m_prototypes) {
    prototype->emplace(registry, get_entity());
  }
}

template<typename T>
T const*
ArchetypeBadguy::find() const
{
  return m_registry ? m_registry->try_get<T>(get_entity()) : nullptr;
}

void
ArchetypeBadguy::active_update(float dt_sec)
{
  if (auto const* floater = find<Floater>()) {
    update_floater(*floater);
  }

  if (auto const* patrol = find<Patrol>()) {
    // Turn around before leaving the patrol area
    float target = m_dir == Direction::LEFT ? -walk_speed : walk_speed;
    if (m_dir != Direction::LEFT && get_pos().x > (m_start_position.x + patrol->radius - 20.f))
      target = -walk_speed;
    if (m_dir != Direction::RIGHT && get_pos().x < (m_start_position.x - patrol->radius + 20.f))
      target = walk_speed;

    if (!patrol->slowdown_action.empty()) {
      bool const slow = std::abs(m_physic.get_velocity_x()) < walk_speed;
      std::string const action = slow ? patrol->slowdown_action + dir_suffix(m_dir)
                                      : (m_dir == Direction::LEFT ? "left" : "right");
      set_action(action, /* loops = */ -1);
    }

    WalkingBadguy::active_update(dt_sec, target, patrol->acceleration);
  } else {
    WalkingBadguy::active_update(dt_sec);
  }
}

void
ArchetypeBadguy::update_floater(Floater const& floater)
{
  if (m_frozen || m_ignited)
    return;

  Rectf floatbox = get_bbox();
  floatbox.set_bottom(get_bbox().get_bottom() + 8.f);
  bool const float_here = Sector::get().is_free_of_statics(floatbox);
  if (!float_here) {
    m_sprite->set_action(m_dir == Direction::LEFT ? "left" : "right");
  } else {
    m_sprite->set_action(m_dir == Direction::LEFT ? "float-left" : "float-right");
    if (m_physic.get_velocity_y() >= floater.max_fall_speed) {
      m_physic.set_velocity_y(floater.max_fall_speed);
    }
  }
}

bool
ArchetypeBadguy::collision_squished(GameObject& object)
{
  auto const* squish = find<SquishReaction>();
  if (!squish || m_frozen)
    return WalkingBadguy::collision_squished(object);

  if (squish->anchor_bottom) {
    // MovingSprite::set_action() also adapts the hitbox to the new action
    set_action(squish->action + dir_suffix(m_dir), /* loops = */ -1, ANCHOR_BOTTOM);
  } else {
    m_sprite->set_action(squish->action, m_dir);
  }

  if (!squish->particles.empty()) {
    spawn_explosion_sprites(squish->particle_count, squish->particles);
  }

  kill_squished(object);

  if (squish->stop) {
    m_physic.set_gravity_modifier(1.f);
    m_physic.set_velocity_x(0.0);
    m_physic.set_acceleration_x(0.0);
  }
  return true;
}

/* EOF */
