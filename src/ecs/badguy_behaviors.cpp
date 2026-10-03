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
#include "ecs/badguy_behaviors.hpp"

#include <assert.h>
#include <math.h>

#include "badguy/archetype_badguy.hpp"
#include "object/player.hpp"
#include "sprite/sprite.hpp"
#include "supertux/sector.hpp"

namespace {

char const* dir_suffix(Direction dir)
{
  return dir == Direction::LEFT ? "-left" : "-right";
}

} // namespace

// Walker -------------------------------------------------------------

namespace walker {

void
walk(ArchetypeBadguy& self, Walker& walker, float dest_x_velocity, float modifier)
{
  Physic& physic = self.m_physic;
  float current_x_velocity = physic.get_velocity_x();

  if (self.m_frozen)
    return;

  /* We're very close to our target speed. Just set it to avoid oscillation */
  if ((current_x_velocity > (dest_x_velocity - 5.0f)) &&
      (current_x_velocity < (dest_x_velocity + 5.0f)))
  {
    physic.set_velocity_x(dest_x_velocity);
    physic.set_acceleration_x(0.0);
  }
  /* Check if we're going too slow or even in the wrong direction */
  else if (((dest_x_velocity <= 0.0f) && (current_x_velocity > dest_x_velocity)) ||
           ((dest_x_velocity > 0.0f) && (current_x_velocity < dest_x_velocity)))
  {
    /* acceleration == walk-speed => it will take one second to get from zero
     * to full speed. */
    physic.set_acceleration_x(dest_x_velocity * modifier);
  }
  /* Check if we're going too fast */
  else if (((dest_x_velocity <= 0.0f) && (current_x_velocity < dest_x_velocity)) ||
           ((dest_x_velocity > 0.0f) && (current_x_velocity > dest_x_velocity)))
  {
    /* acceleration == walk-speed => it will take one second to get twice the
     * speed to normal speed. */
    physic.set_acceleration_x((-1.f) * dest_x_velocity);
  }
  else
  {
    /* The above should have covered all cases. */
    assert(false);
  }

  if (walker.max_drop_height > -1) {
    if (self.on_ground() && self.might_fall(walker.max_drop_height + 1))
    {
      turn_around(self, walker);
    }
  }

  if ((self.m_dir == Direction::LEFT) && (physic.get_velocity_x() > 0.0f)) {
    self.m_dir = Direction::RIGHT;
    self.set_action(walker.right_action, /* loops = */ -1);
  }
  else if ((self.m_dir == Direction::RIGHT) && (physic.get_velocity_x() < 0.0f)) {
    self.m_dir = Direction::LEFT;
    self.set_action(walker.left_action, /* loops = */ -1);
  }
}

void
turn_around(ArchetypeBadguy& self, Walker& walker)
{
  if (self.m_frozen)
    return;
  self.m_dir = self.m_dir == Direction::LEFT ? Direction::RIGHT : Direction::LEFT;
  if (self.get_state() == ArchetypeBadguy::STATE_INIT ||
      self.get_state() == ArchetypeBadguy::STATE_INACTIVE ||
      self.get_state() == ArchetypeBadguy::STATE_ACTIVE) {
    self.m_sprite->set_action(self.m_dir == Direction::LEFT ? walker.left_action : walker.right_action);
  }
  self.m_physic.set_velocity_x(-self.m_physic.get_velocity_x());
  self.m_physic.set_acceleration_x(-self.m_physic.get_acceleration_x());

  // if we get dizzy, we fall off the screen
  if (walker.turn_around_timer.started()) {
    if (walker.turn_around_counter++ > 10) self.kill_fall();
  } else {
    walker.turn_around_timer.start(1);
    walker.turn_around_counter = 0;
  }
}

} // namespace walker

namespace {

void walker_initialize(ArchetypeBadguy& self)
{
  Walker const& walker = ecs::get<Walker>(self.get_entity());
  if (self.m_frozen)
    return;
  self.m_sprite->set_action(self.m_dir == Direction::LEFT ? walker.left_action : walker.right_action);
  self.m_col.m_bbox.set_size(self.m_sprite->get_current_hitbox_width(), self.m_sprite->get_current_hitbox_height());
  self.m_physic.set_velocity_x(self.m_dir == Direction::LEFT ? -walker.speed : walker.speed);
  self.m_physic.set_acceleration_x(0.0);
}

void walker_after_move(ArchetypeBadguy& self, float /*dt_sec*/)
{
  Walker& walker = ecs::get<Walker>(self.get_entity());
  float const target = walker.target_velocity.value_or(self.m_dir == Direction::LEFT ? -walker.speed : walker.speed);
  float const acceleration = walker.acceleration;
  walker.target_velocity.reset();
  walker.acceleration = 1.0f;

  walker::walk(self, walker, target, acceleration);
}

void walker_collision_solid(ArchetypeBadguy& self, CollisionHit const& hit)
{
  self.update_on_ground_flag(hit);

  if (self.m_frozen)
  {
    self.default_collision_solid(hit);
    return;
  }

  if (hit.top) {
    if (self.m_physic.get_velocity_y() < 0) self.m_physic.set_velocity_y(0);
  }
  if (hit.bottom) {
    if (self.m_physic.get_velocity_y() > 0) self.m_physic.set_velocity_y(0);
  }

  if ((hit.left && (self.m_dir == Direction::LEFT)) || (hit.right && (self.m_dir == Direction::RIGHT))) {
    walker::turn_around(self, ecs::get<Walker>(self.get_entity()));
  }
}

HitResponse walker_collision_badguy(ArchetypeBadguy& self, BadGuy& badguy, CollisionHit const& hit)
{
  if (hit.top) {
    return FORCE_MOVE;
  }

  if (badguy.is_frozen())
    self.collision_solid(hit);

  if ((hit.left && (self.m_dir == Direction::LEFT)) || (hit.right && (self.m_dir == Direction::RIGHT))) {
    walker::turn_around(self, ecs::get<Walker>(self.get_entity()));
  }

  return CONTINUE;
}

// Floater ------------------------------------------------------------

void floater_update(ArchetypeBadguy& self, float /*dt_sec*/)
{
  Floater const& floater = ecs::get<Floater>(self.get_entity());
  if (self.m_frozen || self.m_ignited)
    return;

  Rectf floatbox = self.get_bbox();
  floatbox.set_bottom(self.get_bbox().get_bottom() + 8.f);
  bool const float_here = Sector::get().is_free_of_statics(floatbox);
  if (!float_here) {
    self.m_sprite->set_action(self.m_dir == Direction::LEFT ? "left" : "right");
  } else {
    self.m_sprite->set_action(self.m_dir == Direction::LEFT ? "float-left" : "float-right");
    if (self.m_physic.get_velocity_y() >= floater.max_fall_speed) {
      self.m_physic.set_velocity_y(floater.max_fall_speed);
    }
  }
}

// Patrol -------------------------------------------------------------

void patrol_update(ArchetypeBadguy& self, float /*dt_sec*/)
{
  Patrol const& patrol = ecs::get<Patrol>(self.get_entity());
  Walker& walker = ecs::get<Walker>(self.get_entity());

  // Turn around before leaving the patrol area
  float target = self.m_dir == Direction::LEFT ? -walker.speed : walker.speed;
  if (self.m_dir != Direction::LEFT && self.get_pos().x > (self.m_start_position.x + patrol.radius - 20.f))
    target = -walker.speed;
  if (self.m_dir != Direction::RIGHT && self.get_pos().x < (self.m_start_position.x - patrol.radius + 20.f))
    target = walker.speed;

  if (!patrol.slowdown_action.empty()) {
    bool const slow = std::abs(self.m_physic.get_velocity_x()) < walker.speed;
    std::string const action = slow ? patrol.slowdown_action + dir_suffix(self.m_dir)
                                    : (self.m_dir == Direction::LEFT ? "left" : "right");
    self.set_action(action, /* loops = */ -1);
  }

  walker.target_velocity = target;
  walker.acceleration = patrol.acceleration;
}

// SquishReaction -----------------------------------------------------

bool squish_collision_squished(ArchetypeBadguy& self, GameObject& object)
{
  SquishReaction const& squish = ecs::get<SquishReaction>(self.get_entity());
  if (self.m_frozen)
    return self.default_collision_squished(object);

  if (squish.anchor_bottom) {
    // MovingSprite::set_action() also adapts the hitbox to the new action
    self.set_action(squish.action + dir_suffix(self.m_dir), /* loops = */ -1, ANCHOR_BOTTOM);
  } else if (squish.directional) {
    self.m_sprite->set_action(squish.action, self.m_dir);
  } else {
    self.m_sprite->set_action(squish.action);
  }

  if (!squish.particles.empty()) {
    self.spawn_explosion_sprites(squish.particle_count, squish.particles);
  }

  self.kill_squished(object);

  if (squish.stop) {
    self.m_physic.set_gravity_modifier(1.f);
    self.m_physic.set_velocity_x(0.0);
    self.m_physic.set_acceleration_x(0.0);
  }
  return true;
}

// Jumper -------------------------------------------------------------

HitResponse jumper_hit(ArchetypeBadguy& self, CollisionHit const& chit)
{
  Jumper& jumper = ecs::get<Jumper>(self.get_entity());
  if (chit.bottom) {
    if (!jumper.ground_pos_set)
    {
      jumper.ground_pos = self.get_pos();
      jumper.ground_pos_set = true;
    }

    self.m_physic.set_velocity_y((self.m_frozen || self.get_state() != ArchetypeBadguy::STATE_ACTIVE) ? 0 : jumper.jump_speed);
    self.update_on_ground_flag(chit);
  } else if (chit.top) {
    self.m_physic.set_velocity_y(0);
  }

  return CONTINUE;
}

void jumper_collision_solid(ArchetypeBadguy& self, CollisionHit const& chit)
{
  jumper_hit(self, chit);

  if (self.m_frozen)
    self.default_collision_solid(chit);
}

HitResponse jumper_collision_badguy(ArchetypeBadguy& self, BadGuy& /*other*/, CollisionHit const& chit)
{
  return jumper_hit(self, chit);
}

void jumper_after_move(ArchetypeBadguy& self, float /*dt_sec*/)
{
  Jumper const& jumper = ecs::get<Jumper>(self.get_entity());
  if (self.m_frozen)
    return;

  if (auto player = self.get_nearest_player())
  {
    self.m_dir = (player->get_pos().x > self.get_pos().x) ? Direction::RIGHT : Direction::LEFT;
  }

  if (!jumper.ground_pos_set)
  {
    self.m_sprite->set_action("editor", self.m_dir);
    return;
  }

  if (self.get_pos().y < (jumper.ground_pos.y - jumper.mid_tolerance))
    self.m_sprite->set_action(self.m_dir == Direction::LEFT ? "left-up" : "right-up");
  else if (self.get_pos().y >= (jumper.ground_pos.y - jumper.mid_tolerance) &&
           self.get_pos().y < (jumper.ground_pos.y - jumper.low_tolerance))
    self.m_sprite->set_action(self.m_dir == Direction::LEFT ? "left-middle" : "right-middle");
  else
    self.m_sprite->set_action(self.m_dir == Direction::LEFT ? "left-down" : "right-down");
}

void jumper_after_freeze(ArchetypeBadguy& self)
{
  self.m_physic.set_velocity_y(std::max(0.0f, self.m_physic.get_velocity_y()));
}

// Bouncer ------------------------------------------------------------

void bouncer_initialize(ArchetypeBadguy& self)
{
  Bouncer const& bouncer = ecs::get<Bouncer>(self.get_entity());
  self.m_physic.set_velocity_x(self.m_dir == Direction::LEFT ? -bouncer.speed : bouncer.speed);
  self.m_sprite->set_action(self.m_dir);
}

void bouncer_after_move(ArchetypeBadguy& self, float /*dt_sec*/)
{
  if ((self.m_sprite->get_action() == "left-up" || self.m_sprite->get_action() == "right-up") &&
      self.m_sprite->animation_done())
  {
    self.m_sprite->set_action(self.m_dir);
  }

  Rectf lookbelow = self.get_bbox();
  lookbelow.set_bottom(lookbelow.get_bottom() + 48);
  lookbelow.set_top(lookbelow.get_top() + 31);
  bool const ground_below = !Sector::get().is_free_of_statics(lookbelow);
  if (ground_below && (self.m_physic.get_velocity_y() >= 64.0f))
  {
    self.m_sprite->set_action(self.m_dir == Direction::LEFT ? "left-down" : "right-down");
  }
  if (!ground_below && (self.m_sprite->get_action() == "left-down" || self.m_sprite->get_action() == "right-down"))
  {
    self.m_sprite->set_action(self.m_dir);
  }
}

void bouncer_collision_solid(ArchetypeBadguy& self, CollisionHit const& hit)
{
  Bouncer const& bouncer = ecs::get<Bouncer>(self.get_entity());
  if (self.m_sprite->get_action() == "squished")
  {
    return;
  }

  if (hit.bottom) {
    if (self.get_state() == ArchetypeBadguy::STATE_ACTIVE) {
      float bounce_speed = -self.m_physic.get_velocity_y() * bouncer.bounce_factor;
      self.m_physic.set_velocity_y(std::min(bouncer.jump_speed, bounce_speed));
      self.m_sprite->set_action(self.m_dir == Direction::LEFT ? "left-up" : "right-up", /* loops = */ 1);
    } else {
      self.m_physic.set_velocity_y(0);
    }
  } else if (hit.top) {
    self.m_physic.set_velocity_y(0);
  }

  // left or right collision
  // The direction must correspond, else we got fake bounces on slopes.
  if ((hit.left && self.m_dir == Direction::LEFT) || (hit.right && self.m_dir == Direction::RIGHT)) {
    self.m_dir = self.m_dir == Direction::LEFT ? Direction::RIGHT : Direction::LEFT;
    self.m_sprite->set_action(self.m_dir);
    self.m_physic.set_velocity_x(-self.m_physic.get_velocity_x());
  }
}

HitResponse bouncer_collision_badguy(ArchetypeBadguy& self, BadGuy& /*other*/, CollisionHit const& hit)
{
  self.collision_solid(hit);
  return CONTINUE;
}

} // namespace

template<>
BadGuyBehavior const& behavior_of<Jumper>()
{
  static BadGuyBehavior const behavior = {
    .after_move = &jumper_after_move,
    .collision_solid = &jumper_collision_solid,
    .collision_badguy = &jumper_collision_badguy,
    .after_freeze = &jumper_after_freeze,
  };
  return behavior;
}

template<>
BadGuyBehavior const& behavior_of<Bouncer>()
{
  static BadGuyBehavior const behavior = {
    .initialize = &bouncer_initialize,
    .after_move = &bouncer_after_move,
    .collision_solid = &bouncer_collision_solid,
    .collision_badguy = &bouncer_collision_badguy,
  };
  return behavior;
}

template<>
BadGuyBehavior const& behavior_of<Walker>()
{
  static BadGuyBehavior const behavior = {
    .initialize = &walker_initialize,
    .after_move = &walker_after_move,
    .collision_solid = &walker_collision_solid,
    .collision_badguy = &walker_collision_badguy,
  };
  return behavior;
}

template<>
BadGuyBehavior const& behavior_of<Floater>()
{
  static BadGuyBehavior const behavior = { .update = &floater_update };
  return behavior;
}

template<>
BadGuyBehavior const& behavior_of<Patrol>()
{
  static BadGuyBehavior const behavior = { .update = &patrol_update };
  return behavior;
}

template<>
BadGuyBehavior const& behavior_of<SquishReaction>()
{
  static BadGuyBehavior const behavior = { .collision_squished = &squish_collision_squished };
  return behavior;
}

/* EOF */
