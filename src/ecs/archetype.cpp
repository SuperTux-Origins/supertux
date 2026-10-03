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
#include "ecs/archetype.hpp"

#include <physfs.h>
#include <stdexcept>

#include "ecs/badguy_behaviors.hpp"
#include "ecs/object_behaviors.hpp"
#include "ecs/registry.hpp"
#include "util/file_system.hpp"
#include "util/log.hpp"
#include "util/reader_collection.hpp"
#include "util/reader_object.hpp"
#include "video/layer.hpp"

namespace {

struct ComponentType
{
  std::unique_ptr<ComponentPrototype> (*read)(ReaderMapping const&);
  BadGuyBehavior const* badguy_behavior;
  ObjectBehavior const* object_behavior;
};

/** a component of badguy archetypes */
template<typename T>
ComponentType component_type()
{
  return { &ComponentPrototypeT<T>::from_reader, &behavior_of<T>(), nullptr };
}

/** a component of both badguy and object archetypes */
template<typename T>
ComponentType shared_component_type()
{
  return { &ComponentPrototypeT<T>::from_reader, &behavior_of<T>(), &object_behavior_of<T>() };
}

/** a component of object archetypes */
template<typename T>
ComponentType object_component_type()
{
  return { &ComponentPrototypeT<T>::from_reader, nullptr, &object_behavior_of<T>() };
}

/** Component names usable in an archetype's (components ...) section */
std::map<std::string, ComponentType> const& component_types()
{
  static std::map<std::string, ComponentType> const types = {
    { "walker", component_type<Walker>() },
    { "floater", component_type<Floater>() },
    { "patrol", component_type<Patrol>() },
    { "squish-reaction", component_type<SquishReaction>() },
    { "jumper", component_type<Jumper>() },
    { "bouncer", component_type<Bouncer>() },
    { "circler", component_type<Circler>() },
    { "flyer", component_type<Flyer>() },
    { "elemental-fade", component_type<ElementalFade>() },
    { "looping-sound", component_type<LoopingSound>() },
    { "bomb-carrier", component_type<BombCarrier>() },
    { "fuse", component_type<Fuse>() },
    { "stalactite", component_type<Stalactite>() },
    { "iceblock", component_type<IceBlock>() },
    { "boarder", component_type<Boarder>() },
    { "sleeper", component_type<Sleeper>() },
    { "bullet-shy", component_type<BulletShy>() },
    { "firecracker", component_type<Firecracker>() },
    { "snail", component_type<Snail>() },
    { "snowman", component_type<Snowman>() },
    { "mrtree", component_type<MrTree>() },
    { "stumpy", component_type<Stumpy>() },
    { "jumping-fish", component_type<JumpingFish>() },
    { "hopper", component_type<Hopper>() },
    { "bobber", component_type<Bobber>() },
    { "dart-shooter", component_type<DartShooter>() },
    { "projectile", component_type<Projectile>() },
    { "toad", component_type<Toad>() },
    { "mole", component_type<Mole>() },
    { "skydive", component_type<Skydive>() },
    { "owl", component_type<Owl>() },
    { "ghoul", component_type<Ghoul>() },
    { "dispenser", component_type<Dispenser>() },
    { "willowisp", component_type<WillOWisp>() },
    { "yeti", component_type<Yeti>() },
    { "ghosttree", component_type<GhostTree>() },
    { "ghosttree-willowisp", component_type<TreeWillOWisp>() },
    { "ghosttree-root", component_type<GhostTreeRoot>() },

    // objects
    { "unstable-tile", object_component_type<UnstableTile>() },
    { "weak-block", object_component_type<WeakBlock>() },
    { "magicblock", object_component_type<MagicBlock>() },
    { "reset-point", object_component_type<ResetPoint>() },
    { "block", object_component_type<Block>() },
    { "bonus-block", object_component_type<BonusBlock>() },
    { "brick", object_component_type<Brick>() },
    { "invisible-block", object_component_type<InvisibleBlock>() },
    { "infoblock", object_component_type<InfoBlock>() },
    { "path-follower", shared_component_type<PathFollower>() },
    { "platform", object_component_type<Platform>() },
    { "candle", object_component_type<Candle>() },
    { "torch", object_component_type<Torch>() },
    { "pushbutton", object_component_type<PushButton>() },
    { "ispy", object_component_type<Ispy>() },
    { "hurting", object_component_type<Hurting>() },
    { "coin", object_component_type<Coin>() },
    { "heavy-coin", object_component_type<HeavyCoin>() },
    { "rock", object_component_type<Rock>() },
    { "trampoline", object_component_type<Trampoline>() },
    { "rusty-trampoline", object_component_type<RustyTrampoline>() },
    { "glow", object_component_type<Glow>() },
    { "powerup", object_component_type<PowerUp>() },
    { "growup", object_component_type<GrowUp>() },
    { "flower-bonus", object_component_type<FlowerBonus>() },
    { "star", object_component_type<Star>() },
    { "oneup", object_component_type<OneUp>() },
    { "crusher", object_component_type<Crusher>() },
    { "crusher-root", object_component_type<CrusherRoot>() },
    { "diver", component_type<Diver>() },
    { "haywire", component_type<Haywire>() },
    { "goldbomb", component_type<GoldBomb>() },
    { "livefire", component_type<LiveFire>() },
  };
  return types;
}

} // namespace

Archetype::Archetype(ReaderMapping const& mapping) :
  m_name(),
  m_base(),
  m_aliases(),
  m_properties(),
  m_components()
{
  if (!mapping.read("name", m_name)) {
    throw std::runtime_error("archetype without name");
  }
  if (!mapping.read("base", m_base)) {
    throw std::runtime_error("archetype '" + m_name + "' without base");
  }

  mapping.read("aliases", m_aliases);
  mapping.read("properties", m_properties);

  ReaderCollection components;
  if (mapping.read("components", components)) {
    for (auto const& component : components.get_objects()) {
      auto const& types = component_types();
      auto it = types.find(component.get_name());
      if (it == types.end()) {
        throw std::runtime_error("archetype '" + m_name + "': unknown component '" + component.get_name() + "'");
      }
      m_components.push_back({ it->second.read(component.get_mapping()),
                               it->second.badguy_behavior, it->second.object_behavior });
    }
  }
}

std::string
Archetype::get_sprite() const
{
  std::string sprite;
  if (!m_properties.read("sprite", sprite)) {
    throw std::runtime_error("archetype '" + m_name + "' has no sprite");
  }
  return sprite;
}

int
Archetype::get_layer() const
{
  std::string layer = "objects";
  m_properties.read("layer", layer);

  int offset = 0;
  std::string base = layer;
  if (auto pos = layer.find_last_of("+-"); pos != std::string::npos && pos > 0) {
    base = layer.substr(0, pos);
    offset = std::stoi(layer.substr(pos));
  }

  if (base == "objects") {
    return LAYER_OBJECTS + offset;
  } else if (base == "floatingobjects") {
    return LAYER_FLOATINGOBJECTS + offset;
  } else if (base == "tiles") {
    return LAYER_TILES + offset;
  } else if (base == "backgroundtiles") {
    return LAYER_BACKGROUNDTILES + offset;
  } else {
    throw std::runtime_error("archetype '" + m_name + "': unknown layer '" + layer + "'");
  }
}

std::vector<BadGuyBehavior const*>
Archetype::emplace_components(entt::entity entity, ReaderMapping const* overrides) const
{
  std::vector<BadGuyBehavior const*> behaviors;
  for (auto const& component : m_components) {
    if (!component.badguy_behavior) {
      throw std::runtime_error("archetype '" + m_name + "': component is not for badguys");
    }
    component.prototype->emplace(ecs::registry(), entity, overrides);
    behaviors.push_back(component.badguy_behavior);
  }
  return behaviors;
}

std::vector<ObjectBehavior const*>
Archetype::emplace_object_components(entt::entity entity, ReaderMapping const* overrides) const
{
  std::vector<ObjectBehavior const*> behaviors;
  for (auto const& component : m_components) {
    if (!component.object_behavior) {
      throw std::runtime_error("archetype '" + m_name + "': component is not for objects");
    }
    component.prototype->emplace(ecs::registry(), entity, overrides);
    behaviors.push_back(component.object_behavior);
  }
  return behaviors;
}

ArchetypeRegistry&
ArchetypeRegistry::instance()
{
  static ArchetypeRegistry instance_;
  return instance_;
}

ArchetypeRegistry::ArchetypeRegistry() :
  m_documents(),
  m_archetypes()
{
  char** files = PHYSFS_enumerateFiles("archetypes");
  if (!files) {
    log_warning("Couldn't read archetypes directory");
    return;
  }

  for (char const* const* filename = files; *filename != nullptr; ++filename) {
    if (std::string_view(*filename).ends_with(".archetype")) {
      load(FileSystem::join("archetypes", *filename));
    }
  }
  PHYSFS_freeList(files);
}

void
ArchetypeRegistry::load(std::string const& filename)
{
  auto doc = std::make_unique<ReaderDocument>(load_reader_document(filename));
  auto root = doc->get_root();
  if (root.get_name() != "supertux-archetype") {
    throw std::runtime_error(filename + ": not a supertux-archetype file");
  }

  auto archetype = std::make_unique<Archetype>(root.get_mapping());
  std::string const name = archetype->get_name();
  if (m_archetypes.contains(name)) {
    throw std::runtime_error(filename + ": duplicate archetype '" + name + "'");
  }
  m_archetypes[name] = std::move(archetype);
  m_documents.push_back(std::move(doc));
}

Archetype const*
ArchetypeRegistry::get(std::string const& name) const
{
  auto it = m_archetypes.find(name);
  return it != m_archetypes.end() ? it->second.get() : nullptr;
}

std::vector<Archetype const*>
ArchetypeRegistry::get_archetypes() const
{
  std::vector<Archetype const*> result;
  for (auto const& [name, archetype] : m_archetypes) {
    result.push_back(archetype.get());
  }
  return result;
}

/* EOF */
