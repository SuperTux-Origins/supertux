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

#include "ecs/player_systems.hpp"

#include <math.h>

#include "audio/sound_manager.hpp"
#include "control/controller.hpp"
#include "math/random.hpp"
#include "math/util.hpp"
#include "object/player.hpp"
#include "object/player_constants.hpp"
#include "object/sprite_particle.hpp"
#include "sprite/sprite.hpp"
#include "supertux/globals.hpp"
#include "supertux/sector.hpp"
#include "supertux/tile.hpp"
#include "util/fade_helper.hpp"

using namespace player_constants;

void
PlayerSystems::update_tag(Player& self, float dt_sec)
{
  if (self.is_dead() || Sector::get().get_object_count<Player>() == 1)
  {
    self.m_look.tag_timer.stop();
    self.m_tag_fade = nullptr;
    self.m_look.tag_alpha = 0.f;
    self.m_look.has_moved = true;
  }

  if (self.m_look.tag_timer.check())
  {
    self.m_look.tag_timer.stop();
    self.m_tag_fade = std::make_unique<FadeHelper>(1.f, 0.f, 1.f);
  }

  if (self.m_tag_fade)
  {
    self.m_look.tag_alpha = self.m_tag_fade->update(dt_sec);
    if (self.m_tag_fade->completed())
    {
      self.m_tag_fade = nullptr;
    }
  }
}

void
PlayerSystems::update_swimming(Player& self)
{
#ifdef SWIMMING
  if (!self.m_life.ghost_mode)
  {
    if (self.m_swim.no_water)
    {
      if (self.m_swim.swimming)
      {
        self.m_swim.water_jump = true;
        if (self.m_physic.get_velocity_y() > -350.f && self.m_controller->hold(Control::UP))
          self.m_physic.set_velocity_y(-350.f);
      }
      self.m_swim.swimming = false;
    }

    if ((self.on_ground() || self.m_climbing || self.m_jump.does_buttjump) && self.m_swim.water_jump)
    {
      if (self.is_big() && !self.m_move.stone && !self.adjust_height(BIG_TUX_HEIGHT))
      {
        //Force Tux's box up a little in order to not phase into floor
        self.adjust_height(BIG_TUX_HEIGHT, 10.f);
        self.do_duck();
      }
      else if (!self.is_big() || self.m_move.stone)
      {
        self.adjust_height(SMALL_TUX_HEIGHT);
      }
      self.m_dir = (self.m_physic.get_velocity_x() >= 0.f) ? Direction::RIGHT : Direction::LEFT;
      self.m_swim.water_jump = false;
      self.m_swim.boosting = false;
      self.m_powersprite->set_angle(0.f);
      self.m_lightsprite->set_angle(0.f);
    }
    self.m_swim.no_water = true;

    if ((self.m_swim.swimming || self.m_swim.water_jump) && self.is_big())
    {
      self.m_col.set_size(TUX_WIDTH, TUX_WIDTH);
      self.adjust_height(TUX_WIDTH);
    }

    Rectf swim_here_box = self.get_bbox();
    swim_here_box.set_bottom(self.m_col.m_bbox.get_bottom() - 16.f);
    bool can_swim_here = !Sector::get().is_free_of_tiles(swim_here_box, true, Tile::WATER);

    if (self.m_swim.swimming)
    {
      if (can_swim_here)
      {
        self.m_swim.no_water = false;
      }
      else
      {
        self.m_swim.swimming = false;
        self.m_swim.water_jump = true;
        if (self.m_physic.get_velocity_y() > -350.f && self.m_controller->hold(Control::UP))
          self.m_physic.set_velocity_y(-350.f);
      }
    }
    else
    {
      if (can_swim_here && !self.m_climbing)
      {
        self.m_swim.no_water = false;
        self.m_swim.water_jump = false;
        self.m_swim.swimming = true;
        self.m_swim.angle = math::angle(Vector(self.m_physic.get_velocity_x(), self.m_physic.get_velocity_y()));
        if (self.is_big())
          self.adjust_height(TUX_WIDTH);
        self.m_jump.wants_buttjump = self.m_jump.does_buttjump = self.m_jump.backflipping = false;
        self.m_dir = (self.m_physic.get_velocity_x() > 0) ? Direction::LEFT : Direction::RIGHT;
        SoundManager::current()->play("sounds/splash.wav", self.get_pos());
      }
    }
  }
#endif
}

void
PlayerSystems::update_wall_cling(Player& self)
{
  Rectf wallclingleft = self.get_bbox();
  wallclingleft.set_left(wallclingleft.get_left() - 8.f);
  self.m_wall.on_left_wall = !Sector::get().is_free_of_statics(wallclingleft);

  Rectf wallclingright = self.get_bbox();
  wallclingright.set_right(wallclingright.get_right() + 8.f);
  self.m_wall.on_right_wall = !Sector::get().is_free_of_statics(wallclingright);

  self.m_wall.can_walljump = ((self.m_wall.on_right_wall || self.m_wall.on_left_wall) && !self.on_ground() && !self.m_swim.swimming && self.m_wall.in_walljump_tile && !self.m_move.stone);
  if (self.m_wall.can_walljump && (self.m_controller->hold(Control::LEFT) || self.m_controller->hold(Control::RIGHT)) && self.m_physic.get_velocity_y() >= 0.f && !self.m_controller->pressed(Control::JUMP))
  {
    self.m_physic.set_velocity_y(MAX_WALLCLING_YM);
    self.m_physic.set_acceleration_y(0);
    if (self.m_swim.water_jump)
    {
      self.adjust_height(self.is_big() ? BIG_TUX_HEIGHT : SMALL_TUX_HEIGHT);
      self.m_swim.water_jump = false;
      self.m_swim.boosting = false;
    }
    self.m_powersprite->set_angle(0.f);
    self.m_lightsprite->set_angle(0.f);
  }

  self.m_wall.in_walljump_tile = false;
}

void
PlayerSystems::update_rolling(Player& self, float dt_sec)
{
  if (self.m_move.stone)
  {
    float f = 1.f;

    if (!std::isnan(self.m_move.floor_normal.x))
      f = std::cos(self.m_move.floor_normal.x);

    self.m_sprite->set_angle(self.m_sprite->get_angle() + self.m_physic.get_movement(dt_sec).x * 3.141592653898f / 2.f / f);
  }
}

void
PlayerSystems::update_ground_movement(Player& self)
{
  // extend/shrink tux collision rectangle so that we fall through/walk over 1
  // tile holes
  if (fabsf(self.m_physic.get_velocity_x()) > MAX_WALK_XM) {
    self.m_col.set_width(RUNNING_TUX_WIDTH);
  }
  else {
    self.m_col.set_width(TUX_WIDTH);
  }

  // on downward slopes, adjust vertical velocity so tux walks smoothly down
  if (self.on_ground() && !self.m_swim.swimming && !self.m_life.dying) {
    if (self.m_move.floor_normal.y != 0) {
      if ((self.m_move.floor_normal.x * self.m_physic.get_velocity_x()) >= 0) {
        self.m_physic.set_velocity_y(250);
      }
    }
  }
}

void
PlayerSystems::update_backflip(Player& self, float dt_sec)
{
  if (self.m_jump.backflipping && !self.m_life.dying) {
    //prevent player from changing direction when backflipping
    self.m_dir = (self.m_jump.backflip_direction == 1) ? Direction::LEFT : Direction::RIGHT;
    if (self.m_jump.backflip_timer.started()) self.m_physic.set_velocity_x(100.0f * static_cast<float>(self.m_jump.backflip_direction));
    //rotate sprite during flip
    self.m_sprite->set_angle(self.m_sprite->get_angle() + (self.m_dir == Direction::LEFT ? 1 : -1) * dt_sec * (360.0f / 0.5f));
    if (self.m_player_status.has_hat_sprite(self.get_id()) && !self.m_swim.swimming && !self.m_swim.water_jump)
      self.m_powersprite->set_angle(self.m_sprite->get_angle());
    if (self.m_player_status.bonus[self.get_id()] == EARTH_BONUS)
      self.m_lightsprite->set_angle(self.m_sprite->get_angle());
  }
}

void
PlayerSystems::update_landing(Player& self)
{
  if (self.on_ground()) {
    self.m_jump.coyote_timer.start(COYOTE_TIME);
  }

  // set fall mode...
  if (self.on_ground()) {
    self.m_jump.fall_mode = PlayerJump::ON_GROUND;
    self.m_jump.last_ground_y = self.get_pos().y;
  }
  else {
    if (self.get_pos().y > self.m_jump.last_ground_y)
      self.m_jump.fall_mode = PlayerJump::FALLING;
    else if (self.m_jump.fall_mode == PlayerJump::ON_GROUND)
      self.m_jump.fall_mode = PlayerJump::JUMPING;
  }

  // check if we landed
  if (self.on_ground()) {
    self.m_jump.jumping = false;
    if (self.m_jump.backflipping && (self.m_jump.backflip_timer.get_timegone() > 0.15f)) {
      self.m_jump.backflipping = false;
      self.m_jump.backflip_direction = 0;
      self.m_physic.set_velocity_x(0);
      if (!self.m_move.stone) {
        self.m_sprite->set_angle(0.0f);
        self.m_powersprite->set_angle(0.0f);
        self.m_lightsprite->set_angle(0.0f);
      }

      // if controls are currently deactivated, we take care of standing up ourselves
      if (self.m_deactivated)
        self.do_standup(false);
    }
  }
}

void
PlayerSystems::update_boost(Player& self, float dt_sec)
{
  if (self.m_move.boost != 0.f)
  {
    bool sign = std::signbit(self.m_move.boost);
    self.m_move.boost = (sign ? -1.f : +1.f) * (std::abs(self.m_move.boost) - dt_sec * BOOST_DECREASE_RATE);
    if (std::signbit(self.m_move.boost) != sign)
      self.m_move.boost = 0.f;
  }
}

void
PlayerSystems::spawn_invincible_sparkles(Player& self)
{
  if (self.m_life.invincible_timer.started())
  {
    if (graphicsRandom.rand(0, 2) == 0)
    {
      float px = graphicsRandom.randf(self.m_col.m_bbox.get_left() + 0, self.m_col.m_bbox.get_right() - 0);
      float py = graphicsRandom.randf(self.m_col.m_bbox.get_top() + 0, self.m_col.m_bbox.get_bottom() - 0);
      Vector ppos = Vector(px, py);
      Vector pspeed = Vector(0, 0);
      Vector paccel = Vector(0, 0);
      Sector::get().add<SpriteParticle>(
        "images/particles/sparkle.sprite",
        // draw bright sparkle when there is lots of time left,
        // dark sparkle when invincibility is about to end
        (self.m_life.invincible_timer.get_timeleft() > TUX_INVINCIBLE_TIME_WARNING) ?
        // make every other a longer sparkle to make trail a bit fuzzy
        (size_t(g_game_time * 20) % 2) ? "small" : "medium"
        :
        "dark", ppos, ANCHOR_MIDDLE, pspeed, paccel, LAYER_OBJECTS + 1 + 5);
    }
  }
}

void
PlayerSystems::update_climb_animation(Player& self)
{
  if (self.m_climbing) {
    if ((self.m_physic.get_velocity_x() == 0) && (self.m_physic.get_velocity_y() == 0))
    {
      self.m_sprite->stop_animation();
      self.m_powersprite->stop_animation();
    }
    else
    {
      self.m_sprite->set_animation_loops(-1);
      self.m_powersprite->set_animation_loops(-1);
    }
  }
}

/* EOF */
