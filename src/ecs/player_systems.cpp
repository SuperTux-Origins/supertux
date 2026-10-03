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
#include <sstream>

#include "audio/sound_manager.hpp"
#include "badguy/badguy.hpp"
#include "control/controller.hpp"
#include "control/input_manager.hpp"
#include "ecs/registry.hpp"
#include "math/random.hpp"
#include "math/util.hpp"
#include "object/bullet.hpp"
#include "object/camera.hpp"
#include "object/display_effect.hpp"
#include "object/falling_coin.hpp"
#include "object/music_object.hpp"
#include "object/particles.hpp"
#include "object/player.hpp"
#include "object/player_constants.hpp"
#include "object/sprite_particle.hpp"
#include "sprite/sprite.hpp"
#include "sprite/sprite_manager.hpp"
#include "supertux/game_session.hpp"
#include "supertux/gameconfig.hpp"
#include "supertux/globals.hpp"
#include "supertux/resources.hpp"
#include "supertux/sector.hpp"
#include "supertux/tile.hpp"
#include "trigger/climbable.hpp"
#include "util/fade_helper.hpp"
#include "video/surface.hpp"

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

void
PlayerSystems::handle_input_swimming(Player& self)
{
  float pointx = float(self.m_controller->hold(Control::RIGHT)) - float(self.m_controller->hold(Control::LEFT)),
        pointy = float(self.m_controller->hold(Control::DOWN)) - float(self.m_controller->hold(Control::UP));

  bool boost = self.m_controller->hold(Control::JUMP);

  PlayerSystems::swim(self, pointx,pointy,boost);
}

void
PlayerSystems::swim(Player& self, float pointx, float pointy, bool boost)
{
    if (self.m_swim.swimming)
      self.m_physic.set_gravity_modifier(.0f);

    // Angle
    bool is_ang_defined = (pointx != 0) || (pointy != 0);
    float pointed_angle = math::angle(Vector(pointx, pointy));
    float delta = 0;

    if(is_ang_defined)
    {
      delta = pointed_angle - self.m_swim.angle;

      if(std::abs(delta) > math::PI)
        delta += delta > 0 ? -math::TAU : math::TAU;

      float epsilon = (boost ? TURN_MAGNITUDE : TURN_MAGNITUDE_BOOST) * delta;
      self.m_swim.angle += epsilon;

      if (self.m_swim.angle > math::PI)
        self.m_swim.angle -= math::TAU;

      if (self.m_swim.angle <= -math::PI)
        self.m_swim.angle += math::TAU;
    }

    float vx = self.m_physic.get_velocity_x();
    float vy = self.m_physic.get_velocity_y();

    if (self.m_swim.swimming && !self.m_swim.water_jump)
    {

      if(is_ang_defined && std::abs(delta) < 0.01f)
        self.m_swim.angle = pointed_angle;

      self.m_swim.accel_modifier = is_ang_defined ? 600.f : 0.f;
      Vector swimming_direction = math::vec2_from_polar(self.m_swim.accel_modifier, pointed_angle);

      self.m_physic.set_acceleration_x((swimming_direction.x - 1.0f * vx) * 2.f);
      self.m_physic.set_acceleration_y((swimming_direction.y - 1.0f * vy) * 2.f);

      // Limit speed, if you go above this speed your acceleration is set to opposite (?)
      if (glm::length(self.m_physic.get_velocity()) > SWIM_SPEED)
      {
        self.m_physic.set_acceleration(-vx,-vy);   // Was too lazy to set it properly ~~zwatotem
      }

      // Natural friction
      if (!is_ang_defined)
      {
        self.m_physic.set_acceleration(-3.f*vx, -3.f*vy);
      }

      //not boosting? let's slow this penguin down!!!
      if (!boost && is_ang_defined && glm::length(self.m_physic.get_velocity()) > (SWIM_SPEED + 10.f))
      {
        self.m_physic.set_acceleration(-5.f*vx, -5.f*vy);
      }

      // Snapping to prevent unwanted floating
        if (!is_ang_defined && glm::length(Vector(vx,vy)) < 100.f)
      {
        vx = 0;
        vy = 0;
      }

      // Turbo, using pointsign
      float minboostspeed = 100.f;
      if (boost && glm::length(self.m_physic.get_velocity()) > minboostspeed)
      {
        if (glm::length(self.m_physic.get_velocity()) < SWIM_BOOST_SPEED)
        {
          self.m_swim.boosting = true;
          if (is_ang_defined)
          {
            vx += SWIM_TO_BOOST_ACCEL * pointx;
            vy += SWIM_TO_BOOST_ACCEL * pointy;
          }
        }
        else
        {
          //cap on boosting
          self.m_physic.set_acceleration(-vx, -vy);
        }
        self.m_physic.set_velocity(vx, vy);
      }
      else
      {
          if (glm::length(self.m_physic.get_velocity()) < (SWIM_SPEED + 10.f))
        {
          self.m_swim.boosting = false;
        }
      }
    }
    if (self.m_swim.water_jump && !self.m_swim.swimming)
    {
      self.m_swim.angle = math::angle(Vector(vx, vy));
    }

  // snap angle dir when water jumping to avoid crazy spinning graphics...
  if (self.m_swim.water_jump && !self.m_swim.swimming && std::abs(self.m_physic.get_velocity_x()) < 10.f)
  {
    self.m_sprite->set_angle(math::degrees(self.m_swim.angle));
    self.m_powersprite->set_angle(math::degrees(self.m_swim.angle));
    if (self.m_lightsprite)
    {
      self.m_lightsprite->set_angle(math::degrees(self.m_swim.angle));
    }
  }
  else
  {
    // otherwise angle the sprite normally
    float angle = (std::abs(self.m_swim.angle) <= math::PI_2) ?
                    math::degrees(self.m_swim.angle) :
                    math::degrees(math::PI + self.m_swim.angle);

    self.m_sprite->set_angle(angle);
    self.m_powersprite->set_angle(angle);
    if (self.m_lightsprite)
    {
      self.m_lightsprite->set_angle(angle);
    }

    //Force the speed to point in the direction Tux is going unless Tux is being pushed by something else
    if (self.m_swim.swimming && !self.m_swim.water_jump && boost && self.m_move.boost == 0.f && !self.m_move.velocity_override)
    {
      self.m_physic.set_velocity(math::at_angle(self.m_physic.get_velocity(), self.m_swim.angle));
    }
  }
}

void
PlayerSystems::apply_friction(Player& self)
{
  bool is_on_ground = self.on_ground();
  float velx = self.m_physic.get_velocity_x();
  if (is_on_ground && (fabsf(velx) < (self.m_move.stone ? 5.f : WALK_SPEED))) {
    self.m_physic.set_velocity_x(0);
    self.m_physic.set_acceleration_x(0);
    return;
  }
  float friction = WALK_ACCELERATION_X;
  if (self.m_move.on_ice && is_on_ground)
    friction *= ICE_FRICTION_MULTIPLIER;
  else
    friction *= NORMAL_FRICTION_MULTIPLIER;

  if (velx < 0) {
    self.m_physic.set_acceleration_x(friction);
  } else if (velx > 0) {
    self.m_physic.set_acceleration_x(-friction);
  } // no friction for physic.get_velocity_x() == 0
}

void
PlayerSystems::handle_horizontal_input(Player& self)
{
  float vx = self.m_physic.get_velocity_x();
  float vy = self.m_physic.get_velocity_y();
  float ax = 0;
  float ay = self.m_physic.get_acceleration_y();

  float dirsign = 0;
  if (!self.m_move.duck || self.m_physic.get_velocity_y() != 0) {
    if (self.m_controller->hold(Control::LEFT) && !self.m_controller->hold(Control::RIGHT)) {
      self.m_old_dir = self.m_dir;
      if (!self.m_swim.water_jump) self.m_dir = Direction::LEFT;
      dirsign = -1;
    } else if (!self.m_controller->hold(Control::LEFT)
              && self.m_controller->hold(Control::RIGHT)) {
      self.m_old_dir = self.m_dir;
      if (!self.m_swim.water_jump) self.m_dir = Direction::RIGHT;
      dirsign = 1;
    }
  }

  // do not run if we're holding something which slows us down
  if ( self.m_grabbed_object && self.m_grabbed_object->is_hampering() ) {
    ax = dirsign * WALK_ACCELERATION_X;
    // limit speed
    if (vx >= MAX_WALK_XM && dirsign > 0) {
      ax = std::min(ax, -OVERSPEED_DECELERATION);
    } else if (vx <= -MAX_WALK_XM && dirsign < 0) {
      ax = std::max(ax, OVERSPEED_DECELERATION);
    }
  } else {
    if ( vx * dirsign < MAX_WALK_XM ) {
      ax = dirsign * WALK_ACCELERATION_X;
    } else {
      ax = dirsign * RUN_ACCELERATION_X;
    }
    // limit speed
    if (vx >= MAX_RUN_XM + BONUS_RUN_XM *((self.m_player_status.bonus[self.get_id()] == AIR_BONUS) ? 1 : 0)) {
      ax = std::min(ax, -OVERSPEED_DECELERATION);
    } else if (vx <= -MAX_RUN_XM - BONUS_RUN_XM *((self.m_player_status.bonus[self.get_id()] == AIR_BONUS) ? 1 : 0)) {
      ax = std::max(ax, OVERSPEED_DECELERATION);
    }
  }

  // we can reach WALK_SPEED without any acceleration
  if (dirsign != 0 && fabsf(vx) < WALK_SPEED) {
    vx = dirsign * WALK_SPEED;
  }

  //Check speedlimit.
  if ( self.m_move.speedlimit > 0 &&  vx * dirsign >= self.m_move.speedlimit ) {
    vx = dirsign * self.m_move.speedlimit;
    ax = 0;
  }

  // changing directions?
  if ((vx < 0 && dirsign >0) || (vx>0 && dirsign<0)) {
    if (self.on_ground()) {
      // let's skid!
      if (fabsf(vx)>SKID_XM && !self.m_move.skidding_timer.started()) {
        self.m_move.skidding_timer.start(SKID_TIME);
        SoundManager::current()->play("sounds/skid.wav", self.get_pos());
        // dust some particles
        Sector::get().add<Particles>(
            Vector(self.m_dir == Direction::LEFT ? self.m_col.m_bbox.get_right() : self.m_col.m_bbox.get_left(), self.m_col.m_bbox.get_bottom()),
            self.m_dir == Direction::LEFT ? 50 : -70, self.m_dir == Direction::LEFT ? 70 : -50, 260.0f, 280.0f,
            Vector(0, 300), 3, Color(.4f, .4f, .4f), 3, .8f, LAYER_OBJECTS+1);

        ax *= 2.5f;
      } else {
        ax *= 2;
      }
    }
    else {
      // give Tux tighter air control
      ax *= 2.f;
    }
  }

  if (self.m_move.on_ice && self.on_ground()) {
    ax *= ICE_ACCELERATION_MULTIPLIER;
  }

  self.m_physic.set_velocity(vx, vy);
  self.m_physic.set_acceleration(ax, ay);

  // we get slower when not pressing any keys
  if (dirsign == 0) {
    PlayerSystems::apply_friction(self);
  }

}

void
PlayerSystems::early_jump_apex(Player& self)
{
  if (!self.m_jump.early_apex)
  {
    self.m_jump.early_apex = true;
    self.m_physic.set_gravity_modifier(JUMP_EARLY_APEX_FACTOR);
  }
}

void
PlayerSystems::do_jump_apex(Player& self)
{
  if (self.m_jump.early_apex)
  {
    self.m_jump.early_apex = false;
    self.m_physic.set_gravity_modifier(1.0f);
  }
}

void
PlayerSystems::handle_vertical_input(Player& self)
{
  // Press jump key
  if (self.m_controller->pressed(Control::JUMP)) self.m_jump.jump_button_timer.start(JUMP_GRACE_TIME);
  if (self.m_controller->hold(Control::JUMP) && self.m_jump.jump_button_timer.started() && (self.m_jump.can_jump || self.m_jump.coyote_timer.started())) {
    self.m_jump.jump_button_timer.stop();
    if (self.m_move.duck) {
      // when running, only jump a little bit; else do a backflip
      if ((self.m_physic.get_velocity_x() != 0) ||
          (self.m_controller->hold(Control::LEFT)) ||
          (self.m_controller->hold(Control::RIGHT)))
      {
        self.do_jump(-300);
      }
      else
      {
        self.do_backflip();
      }
    } else {
      // airflower allows for higher jumps-
      // jump a bit higher if we are running; else do a normal jump
      if (self.m_player_status.bonus[self.get_id()] == AIR_BONUS)
        self.do_jump((fabsf(self.m_physic.get_velocity_x()) > MAX_WALK_XM) ? -620.0f : -580.0f);
      else
        self.do_jump((fabsf(self.m_physic.get_velocity_x()) > MAX_WALK_XM) ? -580.0f : -520.0f);
    }
    //Stop the coyote timer only after calling do_jump, because do_jump also checks for the timer
    self.m_jump.coyote_timer.stop();
    // airflower glide only when holding jump key
  }
  else if (self.m_controller->hold(Control::JUMP) && self.m_player_status.bonus[self.get_id()] == AIR_BONUS && self.m_physic.get_velocity_y() > MAX_GLIDE_YM) {
    // glide stops if buttjump is initiated
    if (!self.m_controller->hold(Control::DOWN))
    {
      self.m_physic.set_velocity_y(MAX_GLIDE_YM);
      self.m_physic.set_acceleration_y(0);
    }
  }


  // Let go of jump key
  else if (!self.m_controller->hold(Control::JUMP)) {
    if (!self.m_jump.backflipping && self.m_jump.jumping && self.m_physic.get_velocity_y() < 0) {
      self.m_jump.jumping = false;
      PlayerSystems::early_jump_apex(self);
    }
  }

  if (self.m_jump.early_apex && self.m_physic.get_velocity_y() >= 0) {
    PlayerSystems::do_jump_apex(self);
  }

  /* In case the player has pressed Down while in a certain range of air,
     enable butt jump action */
  if (self.m_controller->hold(Control::DOWN) && !self.m_move.duck && self.is_big() && !self.on_ground()) {
    self.m_jump.wants_buttjump = true;
    if (self.m_jump.buttjump_timer.check())
    {
      self.m_jump.buttjump_timer.stop();
      self.m_jump.does_buttjump = true;
    }
    if (self.m_jump.does_buttjump) {
      self.m_physic.set_velocity_y(BUTTJUMP_SPEED);
    }
  }

  /* When Down is not held anymore, disable butt jump */
  if (!self.m_controller->hold(Control::DOWN)) {
    self.m_jump.wants_buttjump = false;
    self.m_jump.does_buttjump = false;
  }

  //The real walljumping magic
  if (self.m_controller->pressed(Control::JUMP) && self.m_wall.can_walljump && !self.m_jump.backflipping)
  {
    SoundManager::current()->play((self.is_big()) ? "sounds/bigjump.wav" : "sounds/jump.wav", self.get_pos());
    self.m_physic.set_velocity_x(self.m_player_status.bonus[self.get_id()] == AIR_BONUS ?
      self.m_wall.on_left_wall ? 480.f : -480.f : self.m_wall.on_left_wall ? 380.f : -380.f);
    self.do_jump(-520.f);
  }

 self.m_physic.set_acceleration_y(0);
}

void
PlayerSystems::handle_input(Player& self)
{
  // Display the player's ID on top of them at the beginning of the level/sector
  // and persist the number until the player moves, because players will be
  // stacked upon spawning.
  // It is probably possible to displace the player without touching left or
  // right, but for simplicity, only those can make the player number vanish.
  if (!self.m_look.has_moved && (self.m_controller->hold(Control::LEFT) || self.m_controller->hold(Control::RIGHT)))
  {
    self.m_look.has_moved = true;
    self.m_look.tag_timer.start(1.f);
  }

  if (self.m_life.ghost_mode) {
    PlayerSystems::handle_input_ghost(self);
    return;
  }
  if (self.m_climbing) {
    PlayerSystems::handle_input_climbing(self);
    return;
  }
  if (self.m_move.stone) {
    PlayerSystems::handle_input_rolling(self);
    return;
  }
  if (self.m_swim.swimming) {
    PlayerSystems::handle_input_swimming(self);
  }
  else
  {
    if (self.m_swim.water_jump)
    {
      PlayerSystems::swim(self, 0,0,0);
    }
  }

  if (!self.m_swim.swimming)
  {
    if (!self.m_swim.water_jump && !self.m_jump.backflipping) self.m_sprite->set_angle(0);
    if (!self.m_jump.early_apex) {
      self.m_physic.set_gravity_modifier(1.0f);
    }
    else {
      self.m_physic.set_gravity_modifier(JUMP_EARLY_APEX_FACTOR);
    }
  }

  /* Peeking */
  if (!self.m_controller->hold( Control::PEEK_LEFT ) && !self.m_controller->hold( Control::PEEK_RIGHT))
    self.m_move.peeking_x = Direction::AUTO;
  if (!self.m_controller->hold( Control::PEEK_UP ) && !self.m_controller->hold( Control::PEEK_DOWN))
    self.m_move.peeking_y = Direction::AUTO;

  if (self.m_controller->pressed(Control::PEEK_LEFT))
    self.m_move.peeking_x = Direction::LEFT;
  else if (self.m_controller->pressed(Control::PEEK_RIGHT))
    self.m_move.peeking_x = Direction::RIGHT;

  if (self.m_controller->pressed(Control::PEEK_UP))
    self.m_move.peeking_y = Direction::UP;
  else if (self.m_controller->pressed(Control::PEEK_DOWN))
    self.m_move.peeking_y = Direction::DOWN;

  /* Handle horizontal movement: */
  if (!self.m_jump.backflipping && !self.m_move.stone && !self.m_swim.swimming) PlayerSystems::handle_horizontal_input(self);

  /* Jump/jumping? */
  if (self.on_ground())
    self.m_jump.can_jump = true;

  /* Handle vertical movement: */
  if (!self.m_move.stone && !self.m_swim.swimming) PlayerSystems::handle_vertical_input(self);

  /* grabbing */
  bool just_grabbed = PlayerSystems::try_grab(self);

  /* Shoot! */
  auto active_bullets = Sector::get().get_object_count<Bullet>([&self](Bullet const& b){ return &b.get_player() == &self; });
  if (self.m_controller->pressed(Control::ACTION) && (self.m_player_status.bonus[self.get_id()] == FIRE_BONUS || self.m_player_status.bonus[self.get_id()] == ICE_BONUS) && !just_grabbed) {
    if ((self.m_player_status.bonus[self.get_id()] == FIRE_BONUS &&
      active_bullets < self.m_player_status.max_fire_bullets[self.get_id()]) ||
      (self.m_player_status.bonus[self.get_id()] == ICE_BONUS &&
      active_bullets < self.m_player_status.max_ice_bullets[self.get_id()]))
    {
      Vector pos = self.get_pos() + Vector(self.m_col.m_bbox.get_width() / 2.f, self.m_col.m_bbox.get_height() / 2.f);
      Direction swim_dir;
      swim_dir = ((std::abs(self.m_swim.angle) <= math::PI_2)
        || (self.m_swim.water_jump && std::abs(self.m_physic.get_velocity_x()) < 10.f)) ? Direction::RIGHT : Direction::LEFT;
      if (self.m_swim.swimming || self.m_swim.water_jump)
      {
        self.m_dir = swim_dir;
      }
      Sector::get().add<Bullet>(pos, (self.m_swim.swimming || self.m_swim.water_jump) ?
        self.m_physic.get_velocity() + (Vector(std::cos(self.m_swim.angle), std::sin(self.m_swim.angle)) * 600.f) :
        Vector(((self.m_dir == Direction::RIGHT ? 600.f : -600.f) + self.m_physic.get_velocity_x()), 0.f),
        self.m_dir, self.m_player_status.bonus[self.get_id()], self);
      SoundManager::current()->play("sounds/shoot.wav", self.get_pos());
    }
  }

  /* Turn to Stone */
  if (self.m_controller->hold(Control::DOWN) && !self.m_jump.does_buttjump && self.m_jump.coyote_timer.started() && !self.m_swim.swimming && (std::abs(self.m_physic.get_velocity_x()) > 150.f) && self.m_player_status.bonus[self.get_id()] == EARTH_BONUS) {
    self.m_physic.set_gravity_modifier(1.0f); // Undo jump_early_apex
    self.adjust_height(TUX_WIDTH);
    self.m_move.stone = true;
    self.m_swim.swimming = false;
    self.m_move.duck = false;
  }

  if (self.m_move.stone)
    PlayerSystems::apply_friction(self);

  /* Duck or Standup! */
  if (self.m_controller->hold(Control::DOWN) && !self.m_move.stone && !self.m_swim.swimming) {
    self.do_duck();
  }
  else {
    self.do_standup(false);
  }

  /* Drop grabbed object when releasing the Action button on keyboard or gamepad, and on the second button press when using touchscreen */
  if ((self.m_controller->is_touchscreen() ? self.m_controller->pressed(Control::ACTION) : !self.m_controller->hold(Control::ACTION)) &&
      self.m_grabbed_object && !just_grabbed) {
    auto moving_object = dynamic_cast<MovingObject*> (self.m_grabbed_object);
    if (moving_object) {
      // move the grabbed object a bit away from tux
      Rectf grabbed_bbox = moving_object->get_bbox();
      Rectf dest_;
      if (self.m_swim.swimming || self.m_swim.water_jump)
      {
        dest_.set_bottom(self.m_col.m_bbox.get_bottom() + (std::sin(self.m_swim.angle) * 32.f));
        dest_.set_top(dest_.get_bottom() - grabbed_bbox.get_height());
        dest_.set_left(self.m_col.m_bbox.get_left() + (std::cos(self.m_swim.angle) * 32.f));
        dest_.set_right(dest_.get_left() + grabbed_bbox.get_width());
      }
      else
      {
        dest_.set_bottom(self.m_col.m_bbox.get_top() + self.m_col.m_bbox.get_height() * 0.66666f);
        dest_.set_top(dest_.get_bottom() - grabbed_bbox.get_height());

        if (self.m_dir == Direction::LEFT)
        {
          dest_.set_right(self.m_col.m_bbox.get_left() - 1);
          dest_.set_left(dest_.get_right() - grabbed_bbox.get_width());
        }
        else
        {
          dest_.set_left(self.m_col.m_bbox.get_right() + 1);
          dest_.set_right(dest_.get_left() + grabbed_bbox.get_width());
        }
      }

      if (Sector::get().is_free_of_tiles(dest_, true) &&
         Sector::get().is_free_of_statics(dest_, moving_object, true))
      {
        moving_object->set_pos(dest_.p1());
        if (self.m_controller->hold(Control::UP))
        {
          self.m_grabbed_object->ungrab(self, Direction::UP);
        }
        else if (self.m_controller->hold(Control::DOWN))
        {
          self.m_grabbed_object->ungrab(self, Direction::DOWN);
        }
        else if (self.m_swim.swimming || self.m_swim.water_jump)
        {
          self.m_grabbed_object->ungrab(self,
            std::abs(self.m_swim.angle) <= math::PI_2 ? Direction::RIGHT : Direction::LEFT);
        }
        else
        {
          self.m_grabbed_object->ungrab(self, self.m_dir);
        }
        moving_object->del_remove_listener(self.m_grabbed_object_remove_listener.get());
        self.m_grabbed_object = nullptr;
        self.m_released_object = true;
      }
    } else {
      log_debug("Non MovingObject grabbed?!?");
    }
  }

  if (!self.m_controller->hold(Control::ACTION) && self.m_released_object) {
    self.m_released_object = false;
  }

  /* stop backflipping at will */
  if ( self.m_jump.backflipping && ( !self.m_controller->hold(Control::JUMP) && !self.m_jump.backflip_timer.started()) ){
    self.stop_backflipping();
  }
}

void
PlayerSystems::handle_input_ghost(Player& self)
{
  float vx = 0;
  float vy = 0;
  if (self.m_controller->hold(Control::LEFT)) {
    self.m_dir = Direction::LEFT;
    vx -= MAX_RUN_XM * 2;
  }
  if (self.m_controller->hold(Control::RIGHT)) {
    self.m_dir = Direction::RIGHT;
    vx += MAX_RUN_XM * 2;
  }
  if (self.m_controller->hold(Control::UP)) {
    vy -= MAX_RUN_XM * 2;
  }
  if (self.m_controller->hold(Control::DOWN)) {
    vy += MAX_RUN_XM * 2;
  }
  if (self.m_controller->hold(Control::ACTION)) {
    self.set_ghost_mode(false);
  }
  self.m_physic.set_velocity(Vector(vx, vy) * (self.m_controller->hold(Control::JUMP) ? 2.5f : 1.f));
  self.m_physic.set_acceleration(0, 0);
}

void
PlayerSystems::handle_input_climbing(Player& self)
{
  if (!self.m_climbing) {
    log_warning("handle_input_climbing called with climbing set to 0. Input handling skipped");
    return;
  }

  float vx = 0;
  float vy = 0;
  if (self.m_controller->hold(Control::LEFT)) {
    self.m_dir = Direction::LEFT;
    vx -= MAX_CLIMB_XM;
  }
  if (self.m_controller->hold(Control::RIGHT)) {
    self.m_dir = Direction::RIGHT;
    vx += MAX_CLIMB_XM;
  }
  if (self.m_controller->hold(Control::UP) && self.m_col.m_bbox.get_top() > self.m_climbing->get_bbox().get_top()) {
    vy -= MAX_CLIMB_YM;
  }
  if (self.m_controller->hold(Control::DOWN)) {
    vy += MAX_CLIMB_YM;
  }
  if (self.m_controller->hold(Control::JUMP)) {
    if (self.m_jump.can_jump) {
      self.stop_climbing(*self.m_climbing);
      return;
    }
  } else {
    self.m_jump.can_jump = true;
  }
  if (self.m_controller->hold(Control::ACTION)) {
    self.stop_climbing(*self.m_climbing);
    return;
  }
  self.m_physic.set_velocity(vx, vy);
  self.m_physic.set_acceleration(0, 0);
}

void
PlayerSystems::handle_input_rolling(Player& self)
{
  // handle exiting
  if (self.m_move.stone)
  {
    if (!self.m_controller->hold(Control::DOWN)) {
      self.stop_rolling(false);
    }
    else if (self.m_player_status.bonus[self.get_id()] != EARTH_BONUS) {
      self.stop_rolling();
    }
  }

  // handle jumping
  if (self.m_controller->pressed(Control::JUMP)) self.m_jump.jump_button_timer.start(JUMP_GRACE_TIME);
  if (self.m_controller->hold(Control::JUMP) && self.m_jump.jump_button_timer.started() && (self.m_jump.can_jump || self.m_jump.coyote_timer.started()))
  {
    self.m_jump.jump_button_timer.stop();
    self.do_jump(-450.f);
    self.m_jump.coyote_timer.stop();
  }

  // Let go of jump key
  else if (!self.m_controller->hold(Control::JUMP)) {
    if (!self.m_jump.backflipping && self.m_jump.jumping && self.m_physic.get_velocity_y() < 0) {
      self.m_jump.jumping = false;
      PlayerSystems::early_jump_apex(self);
    }
  }

  if (self.m_jump.early_apex && self.m_physic.get_velocity_y() >= 0) {
    PlayerSystems::do_jump_apex(self);
  }

  // handle x-movement

  if (std::abs(self.m_physic.get_velocity_x()) > MAX_STONE_SPEED) {
    self.m_physic.set_acceleration_x(-self.m_physic.get_velocity_x());
  }
  else
  {
    // these variables are apparently used differently and must be initialized differently to avoid errors
    float ax;
    float sx = 0.f;

    // slope velocity
    if (self.m_move.floor_normal.y != 0)
    {
      if (self.m_move.floor_normal.x > 0.f) {
        sx = ((self.m_dir == Direction::LEFT ? STONE_UP_ACCELERATION : STONE_DOWN_ACCELERATION)*std::abs(self.m_move.floor_normal.x));
      }
      if (self.m_move.floor_normal.x < 0.f) {
        sx = ((self.m_dir == Direction::RIGHT ? -STONE_UP_ACCELERATION : -STONE_DOWN_ACCELERATION)*std::abs(self.m_move.floor_normal.x));
      }
    }
    else
    {
      sx = 0.f;
    }

    // key velocity
    if (self.m_controller->hold(Control::LEFT) && !self.m_controller->hold(Control::RIGHT))
    {
      ax = -STONE_KEY_ACCELERATION;
      self.m_dir = Direction::LEFT;
    }
    else if (self.m_controller->hold(Control::RIGHT) && !self.m_controller->hold(Control::LEFT))
    {
      ax = STONE_KEY_ACCELERATION;
      self.m_dir = Direction::RIGHT;
    }
    else {
      ax = 0.f;
    }

    if (self.m_controller->hold(Control::RIGHT) || self.m_controller->hold(Control::LEFT) || self.m_move.floor_normal.y != 0.f) {
      self.m_physic.set_acceleration_x(ax + sx);
    }
    else {
      PlayerSystems::apply_friction(self);
    }
  }
}

void
PlayerSystems::position_grabbed_object(Player& self)
{
  auto moving_object = dynamic_cast<MovingObject*>(self.m_grabbed_object);
  assert(moving_object);
  auto const& object_bbox = moving_object->get_bbox();
  if (!self.m_swim.swimming && !self.m_swim.water_jump)
  {
    // Position where we will hold the lower-inner corner
    Vector pos(self.m_col.m_bbox.get_left() + self.m_col.m_bbox.get_width() / 2,
      self.m_col.m_bbox.get_top() + self.m_col.m_bbox.get_height()*0.66666f);
    // Adjust to find the grabbed object's upper-left corner
    if (self.m_dir == Direction::LEFT)
      pos.x -= object_bbox.get_width();
    pos.y -= object_bbox.get_height();
    self.m_grabbed_object->grab(self, pos, self.m_dir);
  }
  else
  {
    Vector pos(self.m_col.m_bbox.get_left() + (std::cos(self.m_swim.angle) * 32.f),
               self.m_col.m_bbox.get_top() + (std::sin(self.m_swim.angle) * 32.f));
    self.m_grabbed_object->grab(self, pos, self.m_dir);
  }
}

bool
PlayerSystems::try_grab(Player& self)
{
  if (self.m_controller->hold(Control::ACTION) && !self.m_grabbed_object && !self.m_move.duck && !self.m_released_object)
  {

    Vector pos(0.0f, 0.0f);
    if (!self.m_swim.swimming && !self.m_swim.water_jump)
    {
      if (self.m_dir == Direction::LEFT)
      {
        pos = Vector(self.m_col.m_bbox.get_left() - 5, self.m_col.m_bbox.get_bottom() - 16);
      }
      else
      {
        pos = Vector(self.m_col.m_bbox.get_right() + 5, self.m_col.m_bbox.get_bottom() - 16);
      }
    }
    else
    {
      pos = Vector(self.m_col.m_bbox.get_left() + 16.f + (std::cos(self.m_swim.angle) * 48.f),
                   self.m_col.m_bbox.get_top() + 16.f + (std::sin(self.m_swim.angle) * 48.f));
    }

    for (auto& moving_object : Sector::get().get_objects_by_type<MovingObject>())
    {
      Portable* portable = dynamic_cast<Portable*>(&moving_object);
      if (portable && portable->is_portable())
      {
        // make sure the Portable isn't currently non-solid
        if (moving_object.get_group() == COLGROUP_DISABLED) continue;

        // check if we are within reach
        if (moving_object.get_bbox().contains(pos))
        {
          if (self.m_climbing)
            self.stop_climbing(*self.m_climbing);
          self.m_grabbed_object = portable;

          moving_object.add_remove_listener(self.m_grabbed_object_remove_listener.get());

          PlayerSystems::position_grabbed_object(self);
          return true;
        }
      }
    }
  }
  return false;
}

void
PlayerSystems::draw(Player& self, DrawingContext& context)
{
  if (!self.m_look.visible)
    return;

  if (self.is_dead() && self.m_target && Sector::get().get_object_count<Player>([&self](Player const& p){ return !p.is_dead() && !p.is_dying() && !p.is_winning() && &p != &self; }))
  {
    auto* target = Sector::get().get_object_by_uid<Player>(*self.m_target);
    if (target)
    {
      Vector pos(target->get_bbox().get_middle().x, target->get_bbox().get_top() - static_cast<float>(self.m_multiplayer_arrow->get_height()) * 1.5f);
      Vector pos_surf(pos - Vector(static_cast<float>(self.m_multiplayer_arrow->get_width()) / 2.f, 0.f));
      self.m_multiplayer_arrow->draw(context.color(), pos_surf, LAYER_LIGHTMAP + 1);
      context.color().draw_text(Resources::normal_font, std::to_string(self.get_id() + 1), pos,
                                FontAlignment::ALIGN_CENTER, LAYER_LIGHTMAP + 1);
    }
    return;
  }

  if (self.m_look.tag_alpha > 0.f)
  {
    context.color().draw_text(Resources::normal_font, std::to_string(self.get_id() + 1),
                              self.m_col.m_bbox.get_middle() - Vector(0.f, Resources::normal_font->get_height() / 2.f),
                              FontAlignment::ALIGN_CENTER, LAYER_LIGHTMAP + 1,
                              Color(1.f, 1.f, 1.f, self.m_look.tag_alpha));
  }

  // if Tux is above camera, draw little "air arrow" to show where he is x-wise
  if (self.m_col.m_bbox.get_bottom() - 16 < Sector::get().get_camera().get_translation().y) {
    float px = self.m_col.m_bbox.get_left() + (self.m_col.m_bbox.get_right() - self.m_col.m_bbox.get_left() - static_cast<float>(self.m_airarrow.get()->get_width())) / 2.0f;
    float py = Sector::get().get_camera().get_translation().y;
    py += std::min(((py - (self.m_col.m_bbox.get_bottom() + 16)) / 4), 16.0f);
    context.color().draw_surface(self.m_airarrow, Vector(px, py), LAYER_HUD - 1);
  }

  std::string sa_prefix = "";
  std::string sa_postfix = "";

  if (self.m_player_status.bonus[self.get_id()] == GROWUP_BONUS)
    sa_prefix = "big";
  else if (self.m_player_status.bonus[self.get_id()] == FIRE_BONUS)
    if (g_config->christmas_mode)
      sa_prefix = "santa";
    else
      sa_prefix = "fire";
  else if (self.m_player_status.bonus[self.get_id()] == ICE_BONUS)
    sa_prefix = "ice";
  else if (self.m_player_status.bonus[self.get_id()] == AIR_BONUS)
    sa_prefix = "air";
  else if (self.m_player_status.bonus[self.get_id()] == EARTH_BONUS)
    sa_prefix = "earth";
  else
    sa_prefix = "small";
  if (!self.m_swim.swimming && !self.m_swim.water_jump)
  {
    sa_postfix = (self.m_dir == Direction::RIGHT) ? "-right" : "-left";
  }
  else
  {
    sa_postfix = ((std::abs(self.m_swim.angle) <= math::PI_2)
      || (self.m_swim.water_jump && std::abs(self.m_physic.get_velocity_x()) < 10.f))
      ? "-right" : "-left";
  }

  /* Set Tux sprite action */
  if (self.m_life.dying) {
    self.m_sprite->set_action("gameover");
  }
  else if (self.m_look.growing)
  {
    self.m_sprite->set_action_continued(self.m_swim.swimming || self.m_swim.water_jump ?
      "swimgrow"+sa_postfix : "grow"+sa_postfix);
    // while growing, do not change action
    // self.do_duck() will take care of cancelling growing manually
    // update() will take care of cancelling when growing completed
  }
  else if (self.m_move.stone) {
    self.m_sprite->set_action("earth-stone");
  }
  else if (self.m_climbing) {
    self.m_sprite->set_action(sa_prefix+"-climbing"+sa_postfix);

    // Avoid flickering briefly after growing on ladder
    if ((self.m_physic.get_velocity_x()==0)&&(self.m_physic.get_velocity_y()==0))
      self.m_sprite->stop_animation();
  }
  else if (self.m_jump.backflipping) {
    self.m_sprite->set_action(sa_prefix+"-backflip"+sa_postfix);
  }
  else if (self.m_move.duck && self.is_big() && !self.m_swim.swimming) {
    self.m_sprite->set_action(sa_prefix+"-duck"+sa_postfix);
  }
  else if (self.m_move.skidding_timer.started() && !self.m_move.skidding_timer.check() && !self.m_swim.swimming) {
    self.m_sprite->set_action(sa_prefix+"-skid"+sa_postfix);
  }
  else if (self.m_move.kick_timer.started() && !self.m_move.kick_timer.check() && !self.m_swim.swimming && !self.m_swim.water_jump) {
    self.m_sprite->set_action(sa_prefix+"-kick"+sa_postfix);
  }
  else if ((self.m_jump.wants_buttjump || self.m_jump.does_buttjump) && self.is_big() && !self.m_swim.water_jump) {
    self.m_sprite->set_action(sa_prefix+"-buttjump"+sa_postfix, 1);
  }
  else if ((self.m_controller->hold(Control::LEFT) || self.m_controller->hold(Control::RIGHT)) && self.m_wall.can_walljump)
  {
    self.m_sprite->set_action(sa_prefix+"-walljump"+(self.m_wall.on_left_wall ? "-left" : "-right"), 1);
  }
  else if (!self.on_ground() || self.m_jump.fall_mode != PlayerJump::ON_GROUND)
  {
    if (self.m_physic.get_velocity_x() != 0 || self.m_jump.fall_mode != PlayerJump::ON_GROUND)
    {
      if (self.m_swim.swimming || self.m_swim.water_jump)
      {
        if (self.m_swim.water_jump && self.m_dir != self.m_old_dir)
          log_debug("Obracanko (:");
        if (glm::length(self.m_physic.get_velocity()) < 50.f)
          self.m_sprite->set_action(sa_prefix + "-floating" + sa_postfix);
        else if (self.m_swim.water_jump)
          self.m_sprite->set_action(sa_prefix + "-swimjump" + sa_postfix);
        else
          self.m_sprite->set_action(sa_prefix + "-swimming" + sa_postfix);
      }
      else
      {
        if (self.m_physic.get_velocity_y() > 0)
          self.m_sprite->set_action(sa_prefix + "-fall" + sa_postfix);
        else if (self.m_physic.get_velocity_y() <= 0)
          self.m_sprite->set_action(sa_prefix + "-jump" + sa_postfix);
      }
    }
  }
  else
  {
    if (fabsf(self.m_physic.get_velocity_x()) < 1.0f) {
      // Determine which idle stage we're at
      if (self.m_sprite->get_action().find("-stand-") == std::string::npos && self.m_sprite->get_action().find("-idle-") == std::string::npos) {
        self.m_look.idle_stage = 0;
        self.m_look.idle_timer.start(static_cast<float>(IDLE_TIME[self.m_look.idle_stage]) / 1000.0f);

        self.m_sprite->set_action_continued(sa_prefix+("-" + IDLE_STAGES[self.m_look.idle_stage])+sa_postfix);
      }
      else if (self.m_look.idle_timer.check() || (IDLE_TIME[self.m_look.idle_stage] == 0 && self.m_sprite->animation_done())) {
        self.m_look.idle_stage++;
        if (self.m_look.idle_stage >= IDLE_STAGE_COUNT)
          self.m_look.idle_stage = 1;

        self.m_look.idle_timer.start(static_cast<float>(IDLE_TIME[self.m_look.idle_stage]) / 1000.0f);

        if (IDLE_TIME[self.m_look.idle_stage] == 0)
          self.m_sprite->set_action(sa_prefix+("-" + IDLE_STAGES[self.m_look.idle_stage])+sa_postfix, 1);
        else
          self.m_sprite->set_action(sa_prefix+("-" + IDLE_STAGES[self.m_look.idle_stage])+sa_postfix);
      }
      else {
        self.m_sprite->set_action_continued(sa_prefix+("-" + IDLE_STAGES[self.m_look.idle_stage])+sa_postfix);
      }
    }
    else {
      if (fabsf(self.m_physic.get_velocity_x()) > MAX_WALK_XM && !self.is_big()) {
        self.m_sprite->set_action(sa_prefix+"-run"+sa_postfix);
      } else {
        self.m_sprite->set_action(sa_prefix+"-walk"+sa_postfix);
      }
    }
  }

  /* Set Tux powerup sprite action */
  if (self.m_player_status.has_hat_sprite(self.get_id()))
  {
    self.m_powersprite->set_action(self.m_sprite->get_action());
    if (self.m_powersprite->get_frames() == self.m_sprite->get_frames())
    {
      self.m_powersprite->set_frame(self.m_sprite->get_current_frame());
      self.m_powersprite->set_frame_progress(self.m_sprite->get_current_frame_progress());
    }
    if (self.m_player_status.bonus[self.get_id()] == EARTH_BONUS)
    {
      self.m_lightsprite->set_action(self.m_sprite->get_action());
      if (self.m_lightsprite->get_frames() == self.m_sprite->get_frames())
      {
        self.m_lightsprite->set_frame(self.m_sprite->get_current_frame());
        self.m_lightsprite->set_frame_progress(self.m_sprite->get_current_frame_progress());
      }
    }
  }

  /*
  // Tux is holding something
  if ((grabbed_object != 0 && physic.get_velocity_y() == 0) ||
  (shooting_timer.get_timeleft() > 0 && !shooting_timer.check())) {
  if (duck) {
  } else {
  }
  }
  */

  /* Draw Tux */
  if (self.m_life.safe_timer.started() && size_t(g_game_time * 40) % 2)
  {
  }  // don't draw Tux

  else if (self.m_player_status.bonus[self.get_id()] == EARTH_BONUS) {
    self.m_sprite->draw(context.color(), self.get_pos(), LAYER_OBJECTS + 1);
  }
  else {
    if (self.m_life.dying)
      self.m_sprite->draw(context.color(), self.get_pos(), Sector::get().get_foremost_layer());
    else
      self.m_sprite->draw(context.color(), self.get_pos(), LAYER_OBJECTS + 1);

    if (self.m_player_status.has_hat_sprite(self.get_id()))
      self.m_powersprite->draw(context.color(), self.get_pos(), LAYER_OBJECTS + 1);
  }

  //TODO: Replace recoloring with proper costumes
  Color power_color = (self.m_player_status.bonus[self.get_id()] == FIRE_BONUS ? Color(1.f, 0.7f, 0.5f) :
    self.m_player_status.bonus[self.get_id()] == ICE_BONUS ? Color(0.7f, 1.f, 1.f) :
    self.m_player_status.bonus[self.get_id()] == AIR_BONUS ? Color(0.7f, 1.f, 0.5f) :
    self.m_player_status.bonus[self.get_id()] == EARTH_BONUS ? Color(1.f, 0.9f, 0.6f) :
    Color(1.f, 1.f, 1.f));

  self.m_sprite->set_color(self.m_move.stone ? Color(1.f, 1.f, 1.f) : power_color);
}

/* EOF */
