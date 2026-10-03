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

#include "math/rectf.hpp"
#include "math/vector.hpp"
#include "video/color.hpp"
#include "supertux/timer.hpp"
#include "util/fade_helper.hpp"
#include "util/reader_mapping.hpp"

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

#endif

/* EOF */
