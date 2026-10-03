//  SuperTux
//  Copyright (C) 2018 Ingo Ruhnke <grumbel@gmail.com>
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

#include "supertux/game_object_factory.hpp"

#include "badguy/angrystone.hpp"
#include "badguy/archetype_badguy.hpp"
#include "badguy/dispenser.hpp"
#include "badguy/fish_chasing.hpp"
#include "badguy/fish_harmless.hpp"
#include "badguy/ghosttree.hpp"
#include "badguy/ghoul.hpp"
#include "badguy/kamikazesnowball.hpp"
#include "badguy/kugelblitz.hpp"
#include "badguy/plant.hpp"
#include "badguy/rcrystallo.hpp"
#include "badguy/scrystallo.hpp"
#include "badguy/totem.hpp"
#include "badguy/walking_candle.hpp"
#include "badguy/willowisp.hpp"
#include "badguy/yeti.hpp"
#include "badguy/yeti_stalactite.hpp"
#include "object/ambient_light.hpp"
#include "object/ambient_sound.hpp"
#include "object/background.hpp"
#include "object/bicycle_platform.hpp"
#include "object/archetype_object.hpp"
#include "object/bonus_block.hpp"
#include "object/brick.hpp"
#include "object/bumper.hpp"
#include "object/camera.hpp"
#include "object/candle.hpp"
#include "object/circleplatform.hpp"
#include "object/cloud_particle_system.hpp"
#include "object/coin.hpp"
#include "object/custom_particle_system_file.hpp"
#include "object/decal.hpp"
#include "object/explosion.hpp"
#include "object/fallblock.hpp"
#include "object/firefly.hpp"
#include "object/ghost_particle_system.hpp"
#include "object/gradient.hpp"
#include "object/hurting_platform.hpp"
#include "object/infoblock.hpp"
#include "object/invisible_block.hpp"
#include "object/invisible_wall.hpp"
#include "object/ispy.hpp"
#include "object/lantern.hpp"
#include "object/level_time.hpp"
#include "object/lit_object.hpp"
#include "object/magicblock.hpp"
#include "object/music_object.hpp"
#include "object/pneumatic_platform.hpp"
#include "object/powerup.hpp"
#include "object/pushbutton.hpp"
#include "object/rain_particle_system.hpp"
#include "object/rublight.hpp"
#include "object/rusty_trampoline.hpp"
#include "object/scripted_object.hpp"
#include "object/shard.hpp"
#include "object/skull_tile.hpp"
#include "object/snow_particle_system.hpp"
#include "object/spawnpoint.hpp"
#include "object/spotlight.hpp"
#include "object/text_array_object.hpp"
#include "object/textscroller.hpp"
#include "object/thunderstorm.hpp"
#include "object/tilemap.hpp"
#include "object/torch.hpp"
#include "object/trampoline.hpp"
#include "object/wind.hpp"
#include "ecs/archetype.hpp"
#include "supertux/level.hpp"
#include "supertux/tile_manager.hpp"
#include "trigger/climbable.hpp"
#include "trigger/door.hpp"
#include "trigger/scripttrigger.hpp"
#include "trigger/secretarea_trigger.hpp"
#include "trigger/sequence_trigger.hpp"
#include "trigger/switch.hpp"
#include "trigger/text_area.hpp"
#include "util/reader_document.hpp"
#include "util/reader_mapping.hpp"
#include <sstream>

GameObjectFactory&
GameObjectFactory::instance()
{
  static GameObjectFactory instance_;
  return instance_;
}

GameObjectFactory::GameObjectFactory()
{
  init_factories();
}

void
GameObjectFactory::add_archetypes(std::string const& base)
{
  for (Archetype const* archetype : ArchetypeRegistry::instance().get_archetypes())
  {
    if (archetype->get_base() != "badguy" && archetype->get_base() != "object") {
      throw std::runtime_error("archetype '" + archetype->get_name() + "': unknown base '" + archetype->get_base() + "'");
    }
    if (archetype->get_base() != base) {
      continue;
    }

    std::function<std::unique_ptr<GameObject> (ReaderMapping const&)> factory;
    if (base == "badguy") {
      factory = [archetype](ReaderMapping const& reader) -> std::unique_ptr<GameObject> {
        return std::make_unique<ArchetypeBadguy>(reader, *archetype);
      };
    } else {
      factory = [archetype](ReaderMapping const& reader) -> std::unique_ptr<GameObject> {
        return std::make_unique<ArchetypeObject>(reader, *archetype);
      };
    }
    add_factory(archetype->get_name().c_str(), factory);
    for (auto const& alias : archetype->get_aliases()) {
      add_factory(alias.c_str(), factory);
    }
  }
}

void
GameObjectFactory::init_factories()
{
  // badguys
  m_adding_badguys = true;
  add_factory<AngryStone>("angrystone");
  add_factory<Dispenser>("dispenser");
  add_factory<FishChasing>("fish-chasing");
  add_factory<FishHarmless>("fish-harmless");
  add_factory<FishSwimming>("fish-swimming");
  add_factory<GhostTree>("ghosttree");
  add_factory<Ghoul>("ghoul");
  add_factory<KamikazeSnowball>("kamikazesnowball");
  add_factory<Kugelblitz>("kugelblitz");
  add_factory<LeafShot>("leafshot");
  add_factory<Plant>("plant");
  add_factory<RCrystallo>("rcrystallo");
  add_factory<SCrystallo>("scrystallo");
  add_factory<Totem>("totem");
  add_factory<WalkingCandle>("walking_candle");
  add_factory<WillOWisp>("willowisp");
  add_factory<Yeti>("yeti");
  add_factory<YetiStalactite>("yeti_stalactite");
  add_archetypes("badguy");
  m_adding_badguys = false;

  // other objects
  add_factory<AmbientLight>("ambient-light");
  add_factory<AmbientSound>("ambient_sound"); // backward compatibilty
  add_factory<AmbientSound>("ambient-sound");
  add_factory<Background>("background");
  add_factory<PathGameObject>("path");
  add_factory<BicyclePlatform>("bicycle-platform");
  add_factory<BonusBlock>("bonusblock");
  add_factory<Brick>("brick");
  add_factory<Bumper>("bumper");
  add_factory<Camera>("camera");
  add_factory<Candle>("candle");
  add_factory<CirclePlatform>("circleplatform");
  add_factory<CloudParticleSystem>("particles-clouds");
  add_factory<Crusher>("icecrusher"); // backward compatibility
  add_factory<Crusher>("crusher");
  add_factory<CustomParticleSystem>("particles-custom");
  add_factory<CustomParticleSystemFile>("particles-custom-file");
  add_factory<Coin>("coin");
  add_factory<Decal>("decal");
  add_factory<Explosion>("explosion");
  add_factory<FallBlock>("fallblock");
  add_factory<Firefly>("firefly");
  add_factory<GhostParticleSystem>("particles-ghosts");
  add_factory<Gradient>("gradient");
  add_factory<HeavyBrick>("heavy-brick");
  add_factory<HeavyCoin>("heavycoin");
  add_factory<HurtingPlatform>("hurting_platform");
  add_factory<InfoBlock>("infoblock");
  add_factory<InvisibleBlock>("invisible_block");
  add_factory<InvisibleWall>("invisible_wall");
  add_factory<Ispy>("ispy");
  add_factory<Lantern>("lantern", RegisteredObjectParam::OBJ_PARAM_PORTABLE);
  add_factory<LevelTime>("leveltime");
  add_factory<LitObject>("lit-object");
  add_factory<MagicBlock>("magicblock");
  add_factory<MusicObject>("music");
  add_factory<ParticleZone>("particle-zone");
  add_factory<Platform>("platform");
  add_factory<PneumaticPlatform>("pneumatic-platform");
  add_factory<PowerUp>("powerup");
  add_factory<PushButton>("pushbutton");
  add_factory<RainParticleSystem>("particles-rain");
  add_factory<Rock>("rock", RegisteredObjectParam::OBJ_PARAM_PORTABLE);
  add_factory<RubLight>("rublight");
  add_factory<ScriptedObject>("scriptedobject");
  add_factory<Shard>("shard");
  add_factory<SkullTile>("skull_tile");
  add_factory<SnowParticleSystem>("particles-snow");
  add_factory<Spotlight>("spotlight");
  add_factory<TextScroller>("textscroller");
  add_factory<TextArrayObject>("text-array");
  add_factory<Thunderstorm>("thunderstorm");
  add_factory<Torch>("torch");
  add_factory<Trampoline>("trampoline", RegisteredObjectParam::OBJ_PARAM_PORTABLE);
  add_factory<RustyTrampoline>("rustytrampoline", RegisteredObjectParam::OBJ_PARAM_PORTABLE);
  add_factory<Wind>("wind");
  add_factory<TextArea>("text-area");

  // trigger
  add_factory<Climbable>("climbable");
  add_factory<Door>("door");
  add_factory<ScriptTrigger>("scripttrigger");
  add_factory<SecretAreaTrigger>("secretarea");
  add_factory<SequenceTrigger>("sequencetrigger");
  add_factory<Switch>("switch");

  // editor stuff
  add_factory<SpawnPointMarker>("spawnpoint");

  add_factory("tilemap", [](ReaderMapping const& reader) {
    auto tileset = TileManager::current()->get_tileset(Level::current()->get_tileset());
    return std::make_unique<TileMap>(tileset, reader);
  });

  add_archetypes("object");
}

std::unique_ptr<GameObject>
GameObjectFactory::create(std::string const& name, Vector const& pos, Direction const& dir, std::string const& data) const
{
  std::stringstream lisptext;
  lisptext << "(" << name << "\n"
           << " (x " << pos.x << ")"
           << " (y " << pos.y << ")" << data;
  if (dir != Direction::AUTO) {
    lisptext << " (direction \"" << dir << "\"))";
  } else {
    lisptext << ")";
  }

  auto doc = ReaderDocument::from_stream(lisptext);
  return create(name, doc.get_root().get_mapping());
}

/* EOF */
