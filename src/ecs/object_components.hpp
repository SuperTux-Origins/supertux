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
#ifndef HEADER_SUPERTUX_ECS_OBJECT_COMPONENTS_HPP
#define HEADER_SUPERTUX_ECS_OBJECT_COMPONENTS_HPP

#include <memory>
#include <string>
#include <vector>

#include "collision/collision_hit.hpp"
#include "math/rectf.hpp"
#include "math/vector.hpp"
#include "video/color.hpp"
#include "supertux/timer.hpp"
#include "util/fade_helper.hpp"
#include "util/reader_mapping.hpp"
#include "supertux/player_status.hpp"
#include "video/surface_ptr.hpp"
#include "supertux/info_box_line.hpp"
#include "ecs/runtime_state.hpp"
#include "object/path_object.hpp"
#include "supertux/moving_object.hpp"

class Sprite;

/** Shakes, dissolves and falls when a player stands on it or an
    explosion hits it, then fades back in after a while (unstable_tile).
    Uses the sprite actions "shake", "dissolve" and "fall-down" if
    present. */
struct UnstableTile
{
  /** respawn fades in alpha, which must stay in place */
  static constexpr auto in_place_delete = true;

  float respawn_time = 5.f;
  float fade_out_time = 1.f;
  float fade_in_time = .5f;

  enum class State {
    NORMAL,   /**< default state */
    SHAKE,    /**< shaking, still solid */
    DISSOLVE, /**< dissolving, will turn non-solid after this */
    SLOWFALL, /**< slow fall phase (used when neither shaking nor dissolving exist */
    FALL      /**< falling down */
  };

  // state
  State state = State::NORMAL;
  float slowfall_timer = 0.f;
  Timer revive_timer = {};
  std::shared_ptr<FadeHelper> respawn = {};
  float alpha = 1.f;
  Vector original_pos = {};
};

inline void read_component(ReaderMapping const& mapping, UnstableTile& tile)
{
  mapping.read("respawn-time", tile.respawn_time);
  mapping.read("fade-out-time", tile.fade_out_time);
  mapping.read("fade-in-time", tile.fade_in_time);
}

/** Burns or melts away when hit by a fire bullet or explosion, and
    if linked spreads that to adjacent weak blocks (weak_block). */
struct WeakBlock
{
  bool linked = true;
  /** sprite used when not linked */
  std::string unlinked_sprite = "images/objects/weak_block/meltbox.sprite";

  enum class State {
    NORMAL, /**< default state */
    BURNING, /**< on fire, still solid */
    DISINTEGRATING /**< crumbling to dust, no longer solid */
  };

  // state
  State state = State::NORMAL;
  std::shared_ptr<Sprite> lightsprite = {};
};

inline void read_component(ReaderMapping const& mapping, WeakBlock& block)
{
  mapping.read("linked", block.linked);
  mapping.read("unlinked-sprite", block.unlinked_sprite);
}

/** Solid only while lit with its color, translucent otherwise
    (magicblock). Black blocks need any bright light. */
struct MagicBlock
{
  Color color = Color(0, 0, 0);
  float min_intensity = 0.8f;
  float alpha_solid = 0.7f;
  float alpha_nonsolid = 0.3f;
  float min_solid_time = 1.0f;
  /** seconds to wait for stable conditions until switching solidity */
  float switch_delay_time = 0.0f;

  // state
  bool is_solid = false;
  bool black = false;
  float trigger_red = 0.f;
  float trigger_green = 0.f;
  float trigger_blue = 0.f;
  float solid_time = 0.f;
  float switch_delay = 0.f;
  Rectf solid_box = {};
  Color light = Color(1.0f, 1.0f, 1.0f);
  Vector center = {};
};

inline void read_component(ReaderMapping const& mapping, MagicBlock& block)
{
  std::vector<float> color;
  if (mapping.read("color", color)) {
    block.color = Color(color);
  }
}

/** A checkpoint: touching it rings it and makes it the player's reset
    point (firefly). Torch sprites get a light, vbell/torch sprites
    their own sound. */
struct ResetPoint
{
  // state
  bool activated = false;
  /** position as in the level file, where Tux respawns */
  Vector initial_position = {};
  std::shared_ptr<Sprite> light = {};
};

inline void read_component(ReaderMapping const& /*mapping*/, ResetPoint& /*point*/)
{
}

/** A block that bounces when hit from below, knocking away badguys,
    coins and eggs on top; can break into pieces. The block type
    behavior (bonus-block, brick, ...) is listed before it and handles
    being hit. */
struct Block
{
  // state
  bool bouncing = false;
  bool breaking = false;
  float bounce_dir = 0.f;
  float bounce_offset = 0.f;
  float original_y = -1.f;
};

inline void read_component(ReaderMapping const& /*mapping*/, Block& /*block*/)
{
}

/** Releases its contents when hit (bonusblock). The level sets
    "contents" (or the old numeric "data"), "count" and "script". */
struct BonusBlock
{
  enum class Content {
    COIN, FIREGROW, ICEGROW, AIRGROW, EARTHGROW, STAR, ONEUP, CUSTOM,
    SCRIPT, LIGHT, LIGHT_ON, TRAMPOLINE, RAIN, EXPLODE
  };

  Content contents = Content::COIN;
  int hit_counter = 1;
  std::string script;

  // state
  RuntimeState<std::unique_ptr<MovingObject>> object = {};
  SurfacePtr lightsprite = {};
};

inline void read_component(ReaderMapping const& mapping, BonusBlock& block)
{
  mapping.read("count", block.hit_counter);
  mapping.read("script", block.script);
}

/** Breaks when hit by a big player (else bounces), or gives coins if
    not breakable (brick). Heavy bricks only break under heavy impact. */
struct Brick
{
  bool breakable = true;
  bool heavy = false;

  // state
  int coin_counter = 0;
};

inline void read_component(ReaderMapping const& mapping, Brick& brick)
{
  mapping.read("breakable", brick.breakable);
  mapping.read("heavy", brick.heavy);
}

/** Invisible and passable until hit from below (invisible_block). */
struct InvisibleBlock
{
  // state
  bool visible = false;
};

inline void read_component(ReaderMapping const& /*mapping*/, InvisibleBlock& /*block*/)
{
}

/** Shows a message box when hit (infoblock). */
struct InfoBlock
{
  std::string message;
  Color frontcolor = Color(0.6f, 0.7f, 0.8f, 0.5f);
  Color backcolor = Color(0.f, 0.f, 0.f, 0.f);
  float roundness = 0.f;
  bool fadetransition = true;

  // state
  float shown_pct = 0.f; /**< Value in the range of 0..1, depending on how much of the infobox is currently shown */
  float dest_pct = 0.f; /**< With each call to update(), shown_pct will slowly transition to this value */
  RuntimeState<std::vector<std::unique_ptr<InfoBoxLine>>> lines = {}; /**< lines of text (or images) to display */
  float lines_height = 0.f;
  float initial_y = 0.f;
};

inline void read_component(ReaderMapping const& mapping, InfoBlock& block)
{
  mapping.read("message", block.message);
  std::vector<float> color;
  if (mapping.read("frontcolor", color)) {
    block.frontcolor = Color(color);
  }
  if (mapping.read("backcolor", color)) {
    block.backcolor = Color(color);
  }
  mapping.read("roundness", block.roundness);
  mapping.read("fadetransition", block.fadetransition);
}

/** Follows a path given in the level object: (path ...) inline or
    (path-ref "NAME"). running is the default when the level object
    does not set "running". */
struct PathFollower
{
  bool running = true;

  // state
  RuntimeState<std::unique_ptr<PathObject>> path = {};
};

inline void read_component(ReaderMapping const& /*mapping*/, PathFollower& /*follower*/)
{
}

/** Moves along its path (PathFollower), or automatically between the
    nodes nearest and farthest from the player if unnamed and not
    running (platform). Scriptable as a Platform. */
struct Platform
{
  // state
  int starting_node = 0;
  bool automatic = false;
  /** a Player touched the platform during the last round of collisions */
  bool player_contact = false;
  /** ... during the round before */
  bool last_player_contact = false;
  Vector speed = {};
};

inline void read_component(ReaderMapping const& /*mapping*/, Platform& /*platform*/)
{
}

/** Kills players and badguys touching it (hurting_platform). */
struct Hurting
{
};

inline void read_component(ReaderMapping const& /*mapping*/, Hurting& /*hurting*/)
{
}

/** Collected when the player touches it (coin). May follow a path
    (PathFollower); the level can set "collect-script" and
    "starting-node". */
struct Coin
{
  std::string collect_script;
  int starting_node = 0;
};

inline void read_component(ReaderMapping const& mapping, Coin& coin)
{
  mapping.read("collect-script", coin.collect_script);
  mapping.read("starting-node", coin.starting_node);
}

/** A coin that falls and bounces with gravity, e.g. from a coin rain
    (heavycoin). */
struct HeavyCoin
{
  // state
  CollisionHit last_hit = {};
};

inline void read_component(ReaderMapping const& /*mapping*/, HeavyCoin& /*coin*/)
{
}

/** A carryable object with gravity and ground friction that hurts
    what it falls on (rock). Base of trampolines and lanterns. Levels
    may set on-grab-script and on-ungrab-script. */
struct Rock
{
  std::string on_grab_script;
  std::string on_ungrab_script;

  // state
  bool on_ground = false;
  Vector last_movement = {};
};

inline void read_component(ReaderMapping const& mapping, Rock& rock)
{
  mapping.read("on-grab-script", rock.on_grab_script);
  mapping.read("on-ungrab-script", rock.on_ungrab_script);
}

/** Bounces players and walking badguys landing on it while it is on
    the ground (trampoline). Non-portable ones use a fixed sprite. List
    it before the rock. */
struct Trampoline
{
  bool portable = true;
  std::string fixed_sprite = "images/objects/trampoline/trampoline_fix.sprite";
};

inline void read_component(ReaderMapping const& mapping, Trampoline& trampoline)
{
  mapping.read("portable", trampoline.portable);
  mapping.read("fixed-sprite", trampoline.fixed_sprite);
}

/** A trampoline that breaks after counter bounces or when let go
    (rustytrampoline). List it before the rock. */
struct RustyTrampoline
{
  bool portable = true;
  int counter = 3;
};

inline void read_component(ReaderMapping const& mapping, RustyTrampoline& trampoline)
{
  mapping.read("portable", trampoline.portable);
  mapping.read("counter", trampoline.counter);
}

/** Draws a small light of the given color on the light map. */
struct Glow
{
  Color color = Color(0.0f, 0.0f, 0.0f);
  std::string sprite = "images/objects/lightmap_light/lightmap_light-small.sprite";

  // state
  std::shared_ptr<Sprite> light = {};
};

inline void read_component(ReaderMapping const& mapping, Glow& glow)
{
  std::vector<float> color;
  if (mapping.read("color", color)) {
    glow.color = Color(color);
  }
  mapping.read("sprite", glow.sprite);
}

/** A generic powerup placed in a level (powerup): its effect follows
    from its sprite (egg, flowers, star, 1up, potions) unless the level
    gives a script. */
struct PowerUp
{
  std::string script;
  bool no_physics = false;

  // state
  std::shared_ptr<Sprite> light = {};
};

inline void read_component(ReaderMapping const& mapping, PowerUp& powerup)
{
  mapping.read("script", powerup.script);
  mapping.read("disable-physics", powerup.no_physics);
}

/** The egg from a bonus block: rolls along and makes Tux grow. */
struct GrowUp
{
  // state
  std::shared_ptr<Sprite> shade = {};
  std::shared_ptr<Sprite> light = {};
};

inline void read_component(ReaderMapping const& /*mapping*/, GrowUp& /*growup*/)
{
}

/** A flower rising from a bonus block, giving its bonus (fire, ice,
    air, earth). */
struct FlowerBonus
{
  BonusType bonus = FIRE_BONUS;
};

inline void read_component(ReaderMapping const& mapping, FlowerBonus& flower)
{
  std::string bonus;
  if (mapping.read("bonus", bonus)) {
    if (bonus == "fire") flower.bonus = FIRE_BONUS;
    else if (bonus == "ice") flower.bonus = ICE_BONUS;
    else if (bonus == "air") flower.bonus = AIR_BONUS;
    else if (bonus == "earth") flower.bonus = EARTH_BONUS;
  }
}

/** Bounces away and makes Tux invincible (star from a bonus block). */
struct Star
{
};

inline void read_component(ReaderMapping const& /*mapping*/, Star& /*star*/)
{
}

/** Jumps out and gives 100 coins (1up from a bonus block). */
struct OneUp
{
};

inline void read_component(ReaderMapping const& /*mapping*/, OneUp& /*oneup*/)
{
}

/** Drops (or slides sideways) onto players below it, then recovers
    (icecrusher). Its sprite decides ice/rock sounds, eyes and roots;
    big sprites make a large crusher. */
struct Crusher
{
  enum class State { IDLE, CRUSHING, RECOVERING };
  enum class Direction { DOWN, LEFT, RIGHT };
  enum class Size { NORMAL, LARGE };

  bool sideways = false;

  // state
  State state = State::IDLE;
  Size size = Size::NORMAL;
  Vector start_position = {};
  float cooldown_timer = 0.f;
  Direction side_dir = Direction::DOWN;
  std::shared_ptr<Sprite> lefteye = {};
  std::shared_ptr<Sprite> righteye = {};
  std::shared_ptr<Sprite> whites = {};
};

inline void read_component(ReaderMapping const& mapping, Crusher& crusher)
{
  mapping.read("sideways", crusher.sideways);
}

/** A root growing out of the ground where a root crusher hit. */
struct CrusherRoot
{
  // state
  Vector original_pos = {};
  Crusher::Direction direction = Crusher::Direction::DOWN;
  float delay_remaining = 0.f;
};

inline void read_component(ReaderMapping const& /*mapping*/, CrusherRoot& /*root*/)
{
}

#endif

/* EOF */
