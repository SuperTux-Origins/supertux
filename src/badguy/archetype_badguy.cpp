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

#include <algorithm>

#include "ecs/archetype.hpp"
#include "sprite/sprite.hpp"

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

int layer_of(Archetype const& archetype)
{
  std::string layer;
  if (!archetype.get_properties().read("layer", layer) || layer == "objects") {
    return LAYER_OBJECTS;
  } else if (layer == "behind-tiles") {
    return LAYER_TILES - 1;
  } else if (layer == "floatingobjects") {
    return LAYER_FLOATINGOBJECTS;
  } else {
    throw std::runtime_error("archetype '" + archetype.get_name() + "': unknown layer '" + layer + "'");
  }
}

std::string light_sprite_of(Archetype const& archetype)
{
  std::string light_sprite = default_light_sprite;
  archetype.get_properties().read("light-sprite", light_sprite);
  return light_sprite;
}

} // namespace

ArchetypeBadguy::ArchetypeBadguy(ReaderMapping const& reader, Archetype const& archetype) :
  BadGuy(reader, sprite_of(archetype), layer_of(archetype), light_sprite_of(archetype)),
  m_behaviors(),
  m_freezable(false),
  m_flammable(true),
  m_fall_immune(false),
  m_hurtable(true),
  m_spawned(false)
{
  read_properties(archetype);
  m_behaviors = archetype.emplace_components(get_entity(), &reader);
  construct();
}

ArchetypeBadguy::ArchetypeBadguy(Vector const& pos, Direction dir, Archetype const& archetype,
                                 std::string const& dead_script, std::string const& sprite) :
  BadGuy(pos, dir, sprite.empty() ? sprite_of(archetype) : sprite, layer_of(archetype), light_sprite_of(archetype)),
  m_behaviors(),
  m_freezable(false),
  m_flammable(true),
  m_fall_immune(false),
  m_hurtable(true),
  m_spawned(false)
{
  m_spawned = true;
  m_dead_script = dead_script;
  read_properties(archetype);
  m_behaviors = archetype.emplace_components(get_entity(), nullptr);
  construct();
}

ArchetypeBadguy::ArchetypeBadguy(ReaderMapping const& reader, std::string const& sprite_name, int layer,
                                 std::string const& light_sprite_name) :
  BadGuy(reader, sprite_name, layer, light_sprite_name),
  m_behaviors(),
  m_freezable(false),
  m_flammable(true),
  m_fall_immune(false),
  m_hurtable(true),
  m_spawned(false)
{
}

ArchetypeBadguy::ArchetypeBadguy(Vector const& pos, std::string const& sprite_name, int layer,
                                 std::string const& light_sprite_name) :
  BadGuy(pos, sprite_name, layer, light_sprite_name),
  m_behaviors(),
  m_freezable(false),
  m_flammable(true),
  m_fall_immune(false),
  m_hurtable(true),
  m_spawned(false)
{
}

ArchetypeBadguy::ArchetypeBadguy(Vector const& pos, Direction dir, std::string const& sprite_name, int layer,
                                 std::string const& light_sprite_name) :
  BadGuy(pos, dir, sprite_name, layer, light_sprite_name),
  m_behaviors(),
  m_freezable(false),
  m_flammable(true),
  m_fall_immune(false),
  m_hurtable(true),
  m_spawned(false)
{
}

ArchetypeBadguy::~ArchetypeBadguy()
{
}

std::unique_ptr<ArchetypeBadguy>
ArchetypeBadguy::create(std::string const& name, Vector const& pos, Direction dir,
                        std::string const& dead_script, std::string const& sprite)
{
  Archetype const* archetype = ArchetypeRegistry::instance().get(name);
  if (!archetype) {
    throw std::runtime_error("unknown archetype '" + name + "'");
  }
  return std::make_unique<ArchetypeBadguy>(pos, dir, *archetype, dead_script, sprite);
}

void
ArchetypeBadguy::read_properties(Archetype const& archetype)
{
  ReaderMapping const& props = archetype.get_properties();
  props.read("freezable", m_freezable);
  props.read("flammable", m_flammable);
  props.read("fall-immune", m_fall_immune);
  props.read("hurtable", m_hurtable);
  props.read("count-me", m_countMe);
  props.read("glowing", m_glowing);

  Color light_color;
  if (props.read("light-color", light_color)) {
    m_lightsprite->set_color(light_color);
  }

  bool gravity;
  if (props.read("gravity", gravity)) {
    m_physic.enable_gravity(gravity);
  }

  std::string colgroup;
  if (props.read("colgroup", colgroup)) {
    if (colgroup == "touchable") {
      set_colgroup_active(COLGROUP_TOUCHABLE);
    } else if (colgroup == "disabled") {
      set_colgroup_active(COLGROUP_DISABLED);
    } else {
      throw std::runtime_error("archetype '" + archetype.get_name() + "': unknown colgroup '" + colgroup + "'");
    }
  }

  std::string initial_action;
  if (props.read("initial-action", initial_action)) {
    m_sprite->set_action(initial_action);
  }
}

void
ArchetypeBadguy::construct()
{
  for (auto const* behavior : m_behaviors) {
    if (behavior->construct) {
      behavior->construct(*this);
    }
  }
}

void
ArchetypeBadguy::freeze()
{
  for (auto const* behavior : m_behaviors) {
    if (behavior->freeze) {
      behavior->freeze(*this);
      return;
    }
  }
  BadGuy::freeze();
}

void
ArchetypeBadguy::unfreeze(bool melt)
{
  for (auto const* behavior : m_behaviors) {
    if (behavior->unfreeze) {
      behavior->unfreeze(*this, melt);
      return;
    }
  }
  BadGuy::unfreeze(melt);
}

void
ArchetypeBadguy::collision_tile(uint32_t tile_attributes)
{
  for (auto const* behavior : m_behaviors) {
    if (behavior->collision_tile) {
      behavior->collision_tile(*this, tile_attributes);
      return;
    }
  }
  BadGuy::collision_tile(tile_attributes);
}

void
ArchetypeBadguy::ignite()
{
  for (auto const* behavior : m_behaviors) {
    if (behavior->ignite) {
      behavior->ignite(*this);
      return;
    }
  }
  BadGuy::ignite();
}

void
ArchetypeBadguy::kill_fall()
{
  for (auto const* behavior : m_behaviors) {
    if (behavior->kill_fall) {
      behavior->kill_fall(*this);
      return;
    }
  }
  default_kill_fall();
}

void
ArchetypeBadguy::default_kill_fall()
{
  if (m_fall_immune)
    return;
  BadGuy::kill_fall();
}

bool
ArchetypeBadguy::is_portable() const
{
  for (auto const* behavior : m_behaviors) {
    if (behavior->is_portable) {
      return behavior->is_portable(*this);
    }
  }
  return BadGuy::is_portable();
}

bool
ArchetypeBadguy::can_break() const
{
  for (auto const* behavior : m_behaviors) {
    if (behavior->can_break) {
      return behavior->can_break(*this);
    }
  }
  return BadGuy::can_break();
}

void
ArchetypeBadguy::grab(MovingObject& object, Vector const& pos, Direction dir)
{
  for (auto const* behavior : m_behaviors) {
    if (behavior->grab) {
      behavior->grab(*this, object, pos, dir);
      return;
    }
  }
  BadGuy::grab(object, pos, dir);
}

void
ArchetypeBadguy::ungrab(MovingObject& object, Direction dir)
{
  for (auto const* behavior : m_behaviors) {
    if (behavior->ungrab) {
      behavior->ungrab(*this, object, dir);
      return;
    }
  }
  BadGuy::ungrab(object, dir);
}

HitResponse
ArchetypeBadguy::collision(GameObject& other, CollisionHit const& hit)
{
  for (auto const* behavior : m_behaviors) {
    if (behavior->collision) {
      return behavior->collision(*this, other, hit);
    }
  }
  return BadGuy::collision(other, hit);
}

void
ArchetypeBadguy::draw(DrawingContext& context)
{
  for (auto const* behavior : m_behaviors) {
    if (behavior->draw) {
      behavior->draw(*this, context);
      return;
    }
  }
  BadGuy::draw(context);
}

HitResponse
ArchetypeBadguy::collision_bullet(Bullet& bullet, CollisionHit const& hit)
{
  for (auto const* behavior : m_behaviors) {
    if (behavior->collision_bullet) {
      return behavior->collision_bullet(*this, bullet, hit);
    }
  }
  return BadGuy::collision_bullet(bullet, hit);
}

HitResponse
ArchetypeBadguy::collision_player(Player& player, CollisionHit const& hit)
{
  for (auto const* behavior : m_behaviors) {
    if (behavior->collision_player) {
      return behavior->collision_player(*this, player, hit);
    }
  }
  return BadGuy::collision_player(player, hit);
}

void
ArchetypeBadguy::stop_looping_sounds()
{
  for (auto const* behavior : m_behaviors) {
    if (behavior->stop_looping_sounds) {
      behavior->stop_looping_sounds(*this);
    }
  }
}

void
ArchetypeBadguy::play_looping_sounds()
{
  for (auto const* behavior : m_behaviors) {
    if (behavior->play_looping_sounds) {
      behavior->play_looping_sounds(*this);
    }
  }
}

void
ArchetypeBadguy::activate()
{
  for (auto const* behavior : m_behaviors) {
    if (behavior->activate) {
      behavior->activate(*this);
    }
  }
}

void
ArchetypeBadguy::deactivate()
{
  for (auto const* behavior : m_behaviors) {
    if (behavior->deactivate) {
      behavior->deactivate(*this);
    }
  }
}

void
ArchetypeBadguy::initialize()
{
  for (auto const* behavior : m_behaviors) {
    if (behavior->initialize) {
      behavior->initialize(*this);
      return;
    }
  }
}

bool
ArchetypeBadguy::is_flammable() const
{
  for (auto const* behavior : m_behaviors) {
    if (behavior->is_flammable) {
      return behavior->is_flammable(*this);
    }
  }
  return m_flammable;
}

void
ArchetypeBadguy::active_update(float dt_sec)
{
  for (auto const* behavior : m_behaviors) {
    if (behavior->update && !behavior->update(*this, dt_sec)) {
      return;
    }
  }

  auto move = std::find_if(m_behaviors.begin(), m_behaviors.end(),
                           [](BadGuyBehavior const* behavior) { return behavior->move != nullptr; });
  if (move != m_behaviors.end()) {
    (*move)->move(*this, dt_sec);
  } else {
    BadGuy::active_update(dt_sec);
  }

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
