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
#ifndef HEADER_SUPERTUX_ECS_BADGUY_COMPONENTS_HPP
#define HEADER_SUPERTUX_ECS_BADGUY_COMPONENTS_HPP

#include <memory>
#include <optional>
#include <string>

#include "math/vector.hpp"
#include "audio/sound_source.hpp"
#include "supertux/timer.hpp"
#include "util/reader_mapping.hpp"

/** Walks along the floor, turning at walls and optionally at ledges. */
struct Walker
{
  /** WalkingBadguy holds a reference to its Walker */
  static constexpr auto in_place_delete = true;

  float speed = 80.0f;
  /** maximum drop before turning around at a ledge, -1 to walk off any ledge */
  int max_drop_height = -1;
  std::string left_action = "left";
  std::string right_action = "right";

  /** too many turns within a second make the badguy dizzy, it falls off */
  Timer turn_around_timer = {};
  int turn_around_counter = 0;

  /** Set by other behaviors (Patrol) for the current frame only:
      velocity to walk towards instead of +/-speed, and the acceleration
      multiplier. */
  std::optional<float> target_velocity = {};
  float acceleration = 1.0f;
};

inline void read_component(ReaderMapping const& mapping, Walker& walker)
{
  mapping.read("speed", walker.speed);
  mapping.read("max-drop-height", walker.max_drop_height);
  mapping.read("left-action", walker.left_action);
  mapping.read("right-action", walker.right_action);
}

/** Floats down slowly when there is no ground directly below
    (walkingleaf, viciousivy). */
struct Floater
{
  float max_fall_speed = 35.0f;
};

inline void read_component(ReaderMapping const& mapping, Floater& floater)
{
  mapping.read("max-fall-speed", floater.max_fall_speed);
}

/** Walks back and forth around the start position, slowing down before
    turning (crystallo). Requires a Walker. */
struct Patrol
{
  float radius = 100.0f;
  /** walk acceleration multiplier */
  float acceleration = 1.0f;
  /** action prefix used while slower than walk speed, empty for none */
  std::string slowdown_action;
};

inline void read_component(ReaderMapping const& mapping, Patrol& patrol)
{
  mapping.read("radius", patrol.radius);
  mapping.read("acceleration", patrol.acceleration);
  mapping.read("slowdown-action", patrol.slowdown_action);
}

/** Jumps in place whenever it lands, facing the nearest player
    (jumpy). Sprite actions: left/right-up, -middle, -down, and "editor"
    before the first landing. */
struct Jumper
{
  float jump_speed = -600.0f;
  /** height above the landing position that separates the up, middle
      and down actions */
  float mid_tolerance = 4.0f;
  float low_tolerance = 2.0f;

  // state
  Vector ground_pos = {};
  bool ground_pos_set = false;
};

inline void read_component(ReaderMapping const& mapping, Jumper& jumper)
{
  mapping.read("jump-speed", jumper.jump_speed);
  mapping.read("mid-tolerance", jumper.mid_tolerance);
  mapping.read("low-tolerance", jumper.low_tolerance);
}

/** Bounces along the floor, losing some speed per bounce but never
    jumping lower than jump_speed (bouncingsnowball). Sprite actions:
    left/right, left/right-up after a bounce, left/right-down before
    landing. */
struct Bouncer
{
  float speed = 80.0f;
  float jump_speed = -450.0f;
  float bounce_factor = 0.8f;
};

inline void read_component(ReaderMapping const& mapping, Bouncer& bouncer)
{
  mapping.read("speed", bouncer.speed);
  mapping.read("jump-speed", bouncer.jump_speed);
  mapping.read("bounce-factor", bouncer.bounce_factor);
}

/** Orbits its start position (flame). spin rotates the sprite by
    that many degrees per degree of orbit (iceflame). */
struct Circler
{
  float radius = 100.0f;
  /** radians per second */
  float speed = 2.0f;
  float spin = 0.0f;

  // state
  float angle = 0.0f;
};

inline void read_component(ReaderMapping const& mapping, Circler& circler)
{
  mapping.read("radius", circler.radius);
  mapping.read("speed", circler.speed);
  mapping.read("spin", circler.spin);
}

/** Flies a fixed up and down curve around its start height, facing the
    nearest player and puffing smoke (flyingsnowball). */
struct Flyer
{
  float amplitude = 100.0f;
  float rate = 0.8f;
  float puff_interval_min = 4.0f;
  float puff_interval_max = 8.0f;

  // state
  float elapsed = 0.0f;
  Timer puff_timer = {};
};

inline void read_component(ReaderMapping const& mapping, Flyer& flyer)
{
  mapping.read("amplitude", flyer.amplitude);
  mapping.read("rate", flyer.rate);
  mapping.read("puff-interval-min", flyer.puff_interval_min);
  mapping.read("puff-interval-max", flyer.puff_interval_max);
}

/** Fizzles out (sizzle, "fade" action, smoke) instead of freezing or
    burning, depending on trigger: "freeze" (flame) or "ignite"
    (iceflame). The badguy is removed when the fade animation ends. */
struct ElementalFade
{
  std::string trigger = "freeze";
};

inline void read_component(ReaderMapping const& mapping, ElementalFade& fade)
{
  mapping.read("trigger", fade.trigger);
}

/** Plays a looping sound at the badguy's position while it is active. */
struct LoopingSound
{
  std::string sound;
  float gain = 1.0f;
  float reference_distance = 32.0f;

  // state, shared_ptr keeps the prototype copyable
  std::shared_ptr<SoundSource> source = {};
};

inline void read_component(ReaderMapping const& mapping, LoopingSound& sound)
{
  mapping.read("sound", sound.sound);
  mapping.read("gain", sound.gain);
  mapping.read("reference-distance", sound.reference_distance);
}

/** Stomping leaves a ticking bomb (the "bomb" archetype, using this
    badguy's sprite); falling or burning makes it explode. Can be carried
    while frozen (mrbomb). */
struct BombCarrier
{
  std::string bomb = "bomb";
};

inline void read_component(ReaderMapping const& mapping, BombCarrier& carrier)
{
  mapping.read("bomb", carrier.bomb);
}

/** A ticking bomb that explodes when its "ticking" animation ends, or
    when it falls or burns. Can be carried and thrown (bomb). */
struct Fuse
{
  std::string sound = "sounds/fizz.wav";

  // state, shared_ptr keeps the prototype copyable
  std::shared_ptr<SoundSource> ticking = {};
};

inline void read_component(ReaderMapping const& mapping, Fuse& fuse)
{
  mapping.read("sound", fuse.sound);
}

/** Hangs from the ceiling, shakes when a player passes below (or a
    bullet hits it) and falls, freezing or killing badguys it hits.
    type is "ice" or "rock" (rock ricochets bullets and kills instead of
    freezing). */
struct Stalactite
{
  /** YetiStalactite holds a reference to its Stalactite */
  static constexpr auto in_place_delete = true;

  std::string type;

  enum class State { HANGING, SHAKING, FALLING, SQUISHED };

  // state
  State state = State::HANGING;
  Timer timer = {};
  Vector shake_delta = {};
};

inline void read_component(ReaderMapping const& mapping, Stalactite& stalactite)
{
  mapping.read("type", stalactite.type);
}

/** Walks (with its Walker) until stomped; then lies flat, can be
    carried, and kicked to slide along killing badguys (mriceblock).
    List it before the walker: it decides when the Walker runs. */
struct IceBlock
{
  float kick_speed = 500.0f;
  int max_squishes = 10;
  float nokick_time = 0.1f;
  float flat_time = 4.0f;

  enum class State { NORMAL, FLAT, GRABBED, KICKED, WAKING };

  // state
  State state = State::NORMAL;
  Timer nokick_timer = {};
  Timer flat_timer = {};
  int squishcount = 0;
};

inline void read_component(ReaderMapping const& mapping, IceBlock& iceblock)
{
  mapping.read("kick-speed", iceblock.kick_speed);
  mapping.read("max-squishes", iceblock.max_squishes);
  mapping.read("nokick-time", iceblock.nokick_time);
  mapping.read("flat-time", iceblock.flat_time);
}

/** Jumps onto ledges and over gaps; walks at board_speed until it
    first lands, then at walk_speed (captainsnowball). List it before
    the walker. */
struct Boarder
{
  float walk_speed = 100.0f;
  float board_speed = 200.0f;
  float jump_speed = -400.0f;
};

inline void read_component(ReaderMapping const& mapping, Boarder& boarder)
{
  mapping.read("walk-speed", boarder.walk_speed);
  mapping.read("board-speed", boarder.board_speed);
  mapping.read("jump-speed", boarder.jump_speed);
}

/** Sleeps until a player comes within reach in front of it, then
    wakes up and walks (sspiky). List it before the walker. */
struct Sleeper
{
  float reach = 256.0f;

  enum class State { SLEEPING, WAKING, WALKING };

  // state
  State state = State::SLEEPING;
};

inline void read_component(ReaderMapping const& mapping, Sleeper& sleeper)
{
  mapping.read("reach", sleeper.reach);
}

/** Turns away from fire bullets it can see, and ricochets bullets that
    do not hit it in front (igel). List it before the walker. */
struct BulletShy
{
  float range_of_vision = 256.0f;
  /** seconds before turning around again */
  float turn_recover_time = 0.5f;

  // state
  Timer turn_recover_timer = {};
};

inline void read_component(ReaderMapping const& mapping, BulletShy& shy)
{
  mapping.read("range-of-vision", shy.range_of_vision);
  mapping.read("turn-recover-time", shy.turn_recover_time);
}

/** Explodes harmlessly (pushing things away) when touched, stomped,
    burnt or falling (short_fuse). */
struct Firecracker
{
};

inline void read_component(ReaderMapping const& /*mapping*/, Firecracker& /*firecracker*/)
{
}

/** Walks (with its Walker) until stomped; then hides in its shell, can
    be carried, and kicked to slide (upwards when stomped) killing
    badguys (snail). List it before the walker. */
struct Snail
{
  float kick_speed = 500.0f;
  float kick_speed_y = -500.0f;
  int max_squishes = 10;
  float flat_time = 4.0f;

  enum class State { NORMAL, FLAT, WAKING, KICKED_DELAY, KICKED, GRABBED };

  // state
  State state = State::NORMAL;
  Timer kicked_delay_timer = {};
  Timer flat_timer = {};
  int squishcount = 0;
};

inline void read_component(ReaderMapping const& mapping, Snail& snail)
{
  mapping.read("kick-speed", snail.kick_speed);
  mapping.read("kick-speed-y", snail.kick_speed_y);
  mapping.read("max-squishes", snail.max_squishes);
  mapping.read("flat-time", snail.flat_time);
}

/** Loses its head when stomped or burnt: the head becomes a new badguy
    (snowman). Bullets other than fire ricochet. */
struct Snowman
{
  std::string head = "snowball";
};

inline void read_component(ReaderMapping const& mapping, Snowman& snowman)
{
  mapping.read("head", snowman.head);
}

/** Turns into a stump badguy when stomped, sprouting two smaller
    badguys (mrtree). */
struct MrTree
{
  std::string stump = "stumpy";
  std::string sprout = "viciousivy";
};

inline void read_component(ReaderMapping const& mapping, MrTree& tree)
{
  mapping.read("stump", tree.stump);
  mapping.read("sprout", tree.sprout);
}

/** Is dizzy and invincible for a while when spawned by a MrTree, then
    walks (stumpy). List it before the walker. */
struct Stumpy
{
  float invincible_time = 1.0f;

  // state
  bool invincible = false;
  Timer invincible_timer = {};
};

inline void read_component(ReaderMapping const& mapping, Stumpy& stumpy)
{
  mapping.read("invincible-time", stumpy.invincible_time);
}

/** Jumps out of the water, waits below the surface, and gets beached
    (and burns) when it lands on solid ground for too long (fish). */
struct JumpingFish
{
  float jump_speed = -600.0f;
  float wait_time = 1.0f;
  float beach_time = 5.0f;

  // state
  Timer wait_timer = {};
  Timer beached_timer = {};
  /** y-coordinate to stop at, 0 until it first enters water */
  float stop_y = 0.0f;
};

inline void read_component(ReaderMapping const& mapping, JumpingFish& fish)
{
  mapping.read("jump-speed", fish.jump_speed);
  mapping.read("wait-time", fish.wait_time);
  mapping.read("beach-time", fish.beach_time);
}

/** Stands, charges and jumps towards its facing direction (skullyhop).
    Sprite actions: standing, charging, jumping. */
struct Hopper
{
  float jump_speed_x = 220.0f;
  float jump_speed_y = -450.0f;
  float recover_time = 0.5f;
  std::string sound = "sounds/hop.ogg";

  enum class State { STANDING, CHARGING, JUMPING };

  // state
  State state = State::JUMPING;
  Timer recover_timer = {};
};

inline void read_component(ReaderMapping const& mapping, Hopper& hopper)
{
  mapping.read("jump-speed-x", hopper.jump_speed_x);
  mapping.read("jump-speed-y", hopper.jump_speed_y);
  mapping.read("recover-time", hopper.recover_time);
  mapping.read("sound", hopper.sound);
}

/** Flies up and down without gravity, facing the nearest player
    (spidermite). */
struct Bobber
{
  float speed = 100.0f;
  /** seconds for one full up or down stroke */
  float fly_time = 1.2f;

  // state
  bool going_up = true;
  Timer timer = {};
};

inline void read_component(ReaderMapping const& mapping, Bobber& bobber)
{
  mapping.read("speed", bobber.speed);
  mapping.read("fly-time", bobber.fly_time);
}

/** How the badguy reacts to being stomped. Without this component the
    BadGuy default applies (not squishable). */
struct SquishReaction
{
  std::string action = "squished";
  /** suffix the action with -left/-right */
  bool directional = true;
  /** keep the sprite's bottom edge in place when switching action */
  bool anchor_bottom = false;
  /** before dying: drop (enable gravity, stop vertical movement) */
  bool drop = false;
  /** after dying: stop moving and fall with normal gravity */
  bool stop = false;
  std::string particles;
  int particle_count = 0;
};

inline void read_component(ReaderMapping const& mapping, SquishReaction& squish)
{
  mapping.read("action", squish.action);
  mapping.read("directional", squish.directional);
  mapping.read("anchor-bottom", squish.anchor_bottom);
  mapping.read("drop", squish.drop);
  mapping.read("stop", squish.stop);
  mapping.read("particles", squish.particles);
  mapping.read("particle-count", squish.particle_count);
}

#endif

/* EOF */
