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

#include "audio/sound_manager.hpp"
#include "badguy/archetype_badguy.hpp"
#include "ecs/object_behaviors.hpp"
#include "object/path.hpp"
#include "object/path_walker.hpp"
#include "object/bullet.hpp"
#include "supertux/constants.hpp"
#include "supertux/game_object_factory.hpp"
#include "supertux/game_object_manager.hpp"
#include "object/coin_explode.hpp"
#include "object/explosion.hpp"
#include "supertux/tile.hpp"
#include "util/log.hpp"
#include "video/drawing_context.hpp"
#include "math/random.hpp"
#include "math/util.hpp"
#include "object/sprite_particle.hpp"
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
  walker::initialize(self, ecs::get<Walker>(self.get_entity()));
}

void walker_collision_solid(ArchetypeBadguy& self, CollisionHit const& hit)
{
  walker::collision_solid(self, ecs::get<Walker>(self.get_entity()), hit);
}

HitResponse walker_collision_badguy(ArchetypeBadguy& self, BadGuy& other, CollisionHit const& hit)
{
  return walker::collision_badguy(self, ecs::get<Walker>(self.get_entity()), other, hit);
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

} // namespace

namespace walker {

void initialize(ArchetypeBadguy& self, Walker const& walker)
{
  if (self.m_frozen)
    return;
  self.m_sprite->set_action(self.m_dir == Direction::LEFT ? walker.left_action : walker.right_action);
  self.m_col.m_bbox.set_size(self.m_sprite->get_current_hitbox_width(), self.m_sprite->get_current_hitbox_height());
  self.m_physic.set_velocity_x(self.m_dir == Direction::LEFT ? -walker.speed : walker.speed);
  self.m_physic.set_acceleration_x(0.0);
}

void collision_solid(ArchetypeBadguy& self, Walker& walker, CollisionHit const& hit)
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
    turn_around(self, walker);
  }
}

HitResponse collision_badguy(ArchetypeBadguy& self, Walker& walker, BadGuy& badguy, CollisionHit const& hit)
{
  if (hit.top) {
    return FORCE_MOVE;
  }

  if (badguy.is_frozen())
    self.collision_solid(hit);

  if ((hit.left && (self.m_dir == Direction::LEFT)) || (hit.right && (self.m_dir == Direction::RIGHT))) {
    turn_around(self, walker);
  }

  return CONTINUE;
}

} // namespace walker

namespace {

// Floater ------------------------------------------------------------

bool floater_update(ArchetypeBadguy& self, float /*dt_sec*/)
{
  Floater const& floater = ecs::get<Floater>(self.get_entity());
  if (self.m_frozen || self.m_ignited)
    return true;

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
  return true;
}

// Patrol -------------------------------------------------------------

bool patrol_update(ArchetypeBadguy& self, float /*dt_sec*/)
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
  return true;
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

  if (squish.drop) {
    self.m_physic.enable_gravity(true);
    self.m_physic.set_acceleration_y(0);
    self.m_physic.set_velocity_y(0);
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

void jumper_freeze(ArchetypeBadguy& self)
{
  self.default_freeze();
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

// Circler ------------------------------------------------------------

Vector circler_pos(ArchetypeBadguy const& self, Circler const& circler)
{
  return Vector(self.m_start_position.x + cosf(circler.angle) * circler.radius,
                self.m_start_position.y + sinf(circler.angle) * circler.radius);
}

void circler_construct(ArchetypeBadguy& self)
{
  self.m_col.m_bbox.set_pos(circler_pos(self, ecs::get<Circler>(self.get_entity())));
}

void circler_move(ArchetypeBadguy& self, float dt_sec)
{
  Circler& circler = ecs::get<Circler>(self.get_entity());
  circler.angle = fmodf(circler.angle + dt_sec * circler.speed, math::TAU);
  self.m_col.set_movement(circler_pos(self, circler) - self.get_pos());
  if (circler.spin != 0.0f) {
    self.m_sprite->set_angle(math::degrees(circler.angle) * circler.spin);
  }
}

// Flyer --------------------------------------------------------------

void flyer_initialize(ArchetypeBadguy& self)
{
  self.m_sprite->set_action(self.m_dir);
}

void flyer_activate(ArchetypeBadguy& self)
{
  Flyer& flyer = ecs::get<Flyer>(self.get_entity());
  flyer.puff_timer.start(static_cast<float>(gameRandom.randf(flyer.puff_interval_min, flyer.puff_interval_max)));
}

void flyer_collision_solid(ArchetypeBadguy& self, CollisionHit const& hit)
{
  if (hit.top || hit.bottom) {
    self.m_physic.set_velocity_y(0);
  }
}

void flyer_move(ArchetypeBadguy& self, float dt_sec)
{
  Flyer& flyer = ecs::get<Flyer>(self.get_entity());
  flyer.elapsed = fmodf(flyer.elapsed + dt_sec, math::TAU / flyer.rate);

  float delta = flyer.elapsed * flyer.rate;

  // Put that function in a graphing calculator :
  // sin(x)^3 + sin(3(x - pi/3))/3
  float target_height = std::pow(std::sin(delta), 3.f) +
                        std::sin(3.f * ((delta - math::PI) / 3.f)) / 3.f;
  target_height = target_height * flyer.amplitude + self.m_start_position.y;
  self.m_physic.set_velocity_y(target_height - self.get_pos().y);
  self.m_col.set_movement(self.m_physic.get_movement(1.f));

  if (auto player = self.get_nearest_player()) {
    self.m_dir = (player->get_pos().x > self.get_pos().x) ? Direction::RIGHT : Direction::LEFT;
    self.m_sprite->set_action(self.m_dir);
  }

  // spawn smoke puffs
  if (flyer.puff_timer.check()) {
    Vector ppos = self.m_col.m_bbox.get_middle();
    Vector pspeed = Vector(gameRandom.randf(-10, 10), 150);
    Vector paccel = Vector(0,0);
    Sector::get().add<SpriteParticle>("images/particles/smoke.sprite",
                                      "default",
                                      ppos, ANCHOR_MIDDLE, pspeed, paccel,
                                      LAYER_OBJECTS-1);
    flyer.puff_timer.start(gameRandom.randf(flyer.puff_interval_min, flyer.puff_interval_max));
  }
}

// ElementalFade ------------------------------------------------------

void fade_out(ArchetypeBadguy& self)
{
  SoundManager::current()->play("sounds/sizzle.ogg", self.get_pos());
  self.m_sprite->set_action("fade", 1);
  Sector::get().add<SpriteParticle>("images/particles/smoke.sprite",
                                    "default",
                                    self.m_col.m_bbox.get_middle(), ANCHOR_MIDDLE,
                                    Vector(0, -150), Vector(0,0), LAYER_BACKGROUNDTILES+2);
  self.set_group(COLGROUP_DISABLED);

  // start dead-script
  self.run_dead_script();
}

void fade_freeze(ArchetypeBadguy& self)
{
  if (ecs::get<ElementalFade>(self.get_entity()).trigger == "freeze") {
    fade_out(self);
  } else {
    self.default_freeze();
  }
}

void fade_ignite(ArchetypeBadguy& self)
{
  if (ecs::get<ElementalFade>(self.get_entity()).trigger == "ignite") {
    fade_out(self);
  } else {
    self.default_ignite();
  }
}

void fade_after_move(ArchetypeBadguy& self, float /*dt_sec*/)
{
  if (self.m_sprite->get_action() == "fade" && self.m_sprite->animation_done()) {
    self.remove_me();
  }
}

// LoopingSound -------------------------------------------------------

void sound_construct(ArchetypeBadguy& self)
{
  SoundManager::current()->preload(ecs::get<LoopingSound>(self.get_entity()).sound);
}

void sound_activate(ArchetypeBadguy& self)
{
  LoopingSound& sound = ecs::get<LoopingSound>(self.get_entity());
  sound.source = SoundManager::current()->create_sound_source(sound.sound);
  sound.source->set_position(self.get_pos());
  sound.source->set_looping(true);
  sound.source->set_gain(sound.gain);
  sound.source->set_reference_distance(sound.reference_distance);
  sound.source->play();
}

void sound_deactivate(ArchetypeBadguy& self)
{
  ecs::get<LoopingSound>(self.get_entity()).source.reset();
}

void sound_after_move(ArchetypeBadguy& self, float /*dt_sec*/)
{
  LoopingSound& sound = ecs::get<LoopingSound>(self.get_entity());
  if (sound.source) {
    sound.source->set_position(self.get_pos());
  }
}

void sound_stop(ArchetypeBadguy& self)
{
  LoopingSound& sound = ecs::get<LoopingSound>(self.get_entity());
  if (sound.source) {
    sound.source->stop();
  }
}

void sound_play(ArchetypeBadguy& self)
{
  LoopingSound& sound = ecs::get<LoopingSound>(self.get_entity());
  if (sound.source) {
    sound.source->play();
  }
}

// BombCarrier --------------------------------------------------------

void explode_at(ArchetypeBadguy& self)
{
  Sector::get().add<Explosion>(self.m_col.m_bbox.get_middle(), EXPLOSION_STRENGTH_DEFAULT);
}

void carrier_construct(ArchetypeBadguy& /*self*/)
{
  // Prevent stutter when Tux jumps on it
  SoundManager::current()->preload("sounds/explosion.wav");
}

bool carrier_update(ArchetypeBadguy& self, float /*dt_sec*/)
{
  return !self.is_grabbed();
}

HitResponse carrier_collision(ArchetypeBadguy& self, GameObject& object, CollisionHit const& hit)
{
  if (self.is_grabbed())
    return FORCE_MOVE;
  return self.default_collision(object, hit);
}

HitResponse carrier_collision_player(ArchetypeBadguy& self, Player& player, CollisionHit const& hit)
{
  if (self.is_grabbed())
    return FORCE_MOVE;
  return self.default_collision_player(player, hit);
}

bool carrier_collision_squished(ArchetypeBadguy& self, GameObject& object)
{
  if (self.m_frozen)
    return self.default_collision_squished(object);

  auto player = dynamic_cast<Player*>(&object);
  if (player && player->is_invincible()) {
    player->bounce(self);
    self.kill_fall();
    return true;
  }

  if (self.is_valid()) {
    auto bomb = ArchetypeBadguy::create(ecs::get<BombCarrier>(self.get_entity()).bomb,
                                        self.get_pos(), self.m_dir, {}, self.m_sprite_name);
    // Do not trigger dispenser because we need to wait for
    // the bomb instance to explode.
    if (self.get_parent_dispenser() != nullptr)
    {
      bomb->set_parent_dispenser(self.get_parent_dispenser());
      self.set_parent_dispenser(nullptr);
    }
    Sector::get().add_object(std::move(bomb));
    self.remove_me();
  }
  self.kill_squished(object);
  return true;
}

void carrier_kill_fall(ArchetypeBadguy& self)
{
  if (self.is_valid()) {
    if (self.m_frozen)
      self.default_kill_fall();
    else
    {
      self.remove_me();
      explode_at(self);
      self.run_dead_script();
    }
  }
}

void carrier_ignite(ArchetypeBadguy& self)
{
  if (self.m_frozen)
    self.unfreeze();
  self.kill_fall();
}

bool carrier_is_portable(ArchetypeBadguy const& self)
{
  return self.m_frozen;
}

void carrier_grab(ArchetypeBadguy& self, MovingObject& object, Vector const& pos, Direction dir)
{
  self.Portable::grab(object, pos, dir);
  if (ecs::try_get<Owl>(object.get_entity()))
    self.m_sprite->set_action(dir);
  else
  {
    assert(self.m_frozen);
    self.m_sprite->set_action("iced", dir);
  }

  self.m_col.set_movement(pos - self.get_pos());
  self.m_dir = dir;
  self.set_colgroup_active(COLGROUP_DISABLED);
}

// Fuse ---------------------------------------------------------------

void fuse_explode(ArchetypeBadguy& self)
{
  ecs::get<Fuse>(self.get_entity()).ticking->stop();

  // Make the player let go before we explode, otherwise the player is holding
  // an invalid object.
  if (self.is_grabbed()) {
    auto player = dynamic_cast<Player*>(self.get_owner());
    if (player)
      player->stop_grabbing();
  }

  if (self.is_valid()) {
    self.remove_me();
    explode_at(self);
  }

  self.run_dead_script();
}

void fuse_construct(ArchetypeBadguy& self)
{
  Fuse& fuse = ecs::get<Fuse>(self.get_entity());
  fuse.ticking = SoundManager::current()->create_sound_source(fuse.sound);
  SoundManager::current()->preload("sounds/explosion.wav");
  self.set_action(self.m_dir == Direction::LEFT ? "ticking-left" : "ticking-right", 1);
  fuse.ticking->set_position(self.get_pos());
  fuse.ticking->set_looping(true);
  fuse.ticking->set_gain(1.0f);
  fuse.ticking->set_reference_distance(32);
  fuse.ticking->play();
}

void fuse_collision_solid(ArchetypeBadguy& self, CollisionHit const& hit)
{
  if (self.is_grabbed()) {
    return;
  }
  if (hit.top || hit.bottom)
    self.m_physic.set_velocity_y(0);
  if (hit.left || hit.right)
    self.m_physic.set_velocity_x(-self.m_physic.get_velocity_x() * 0.5f);
  if (hit.crush)
    self.m_physic.set_velocity(0, 0);

  self.update_on_ground_flag(hit);
}

HitResponse fuse_collision_player(ArchetypeBadguy& /*self*/, Player& /*player*/, CollisionHit const& /*hit*/)
{
  return ABORT_MOVE;
}

HitResponse fuse_collision_badguy(ArchetypeBadguy& /*self*/, BadGuy& /*other*/, CollisionHit const& /*hit*/)
{
  return ABORT_MOVE;
}

void fuse_move(ArchetypeBadguy& self, float dt_sec)
{
  if (self.on_ground()) self.m_physic.set_velocity_x(0);

  ecs::get<Fuse>(self.get_entity()).ticking->set_position(self.get_pos());

  if (self.m_sprite->animation_done()) {
    fuse_explode(self);
  }
  else if (!self.is_grabbed()) {
    self.m_col.set_movement(self.m_physic.get_movement(dt_sec));
  }
}

bool fuse_is_portable(ArchetypeBadguy const& /*self*/)
{
  return true;
}

void fuse_grab(ArchetypeBadguy& self, MovingObject& object, Vector const& pos, Direction dir)
{
  self.Portable::grab(object, pos, dir);
  self.m_col.set_movement(pos - self.get_pos());
  self.m_dir = dir;

  // We actually face the opposite direction of Tux here to make the fuse more
  // visible instead of hiding it behind Tux
  self.m_sprite->set_action_continued(self.m_dir == Direction::LEFT ? "ticking-right" : "ticking-left");
  self.set_colgroup_active(COLGROUP_DISABLED);
}

/** Velocity of an object thrown by the player (or let go while swimming) */
void throw_by(ArchetypeBadguy& self, Player* player, Direction dir)
{
  Physic& physic = self.m_physic;

  //handle swimming
  if (player && (player->is_swimming() || player->is_water_jumping()))
  {
    float swimangle = player->get_swimming_angle();
    physic.set_velocity(Vector(std::cos(swimangle) * 40.f, std::sin(swimangle) * 40.f) +
                        player->get_physic().get_velocity());
  }
  //handle non-swimming
  else
  {
    if (player)
    {
      //handle x-movement
      if (fabsf(player->get_physic().get_velocity_x()) < 1.0f)
        physic.set_velocity_x(0.f);
      else if ((player->m_dir == Direction::LEFT && player->get_physic().get_velocity_x() <= -1.0f)
               || (player->m_dir == Direction::RIGHT && player->get_physic().get_velocity_x() >= 1.0f))
        physic.set_velocity_x(player->get_physic().get_velocity_x()
                              + (player->m_dir == Direction::LEFT ? -10.f : 10.f));
      else
        physic.set_velocity_x(player->get_physic().get_velocity_x()
                              + (player->m_dir == Direction::LEFT ? -330.f : 330.f));
      //handle y-movement
      physic.set_velocity_y(dir == Direction::UP ? -500.f :
                            dir == Direction::DOWN ? 500.f :
                            player->get_physic().get_velocity_x() != 0.f ? -200.f : 0.f);
    }
  }
}

void fuse_ungrab(ArchetypeBadguy& self, MovingObject& object, Direction dir)
{
  throw_by(self, dynamic_cast<Player*>(&object), dir);
  self.set_colgroup_active(COLGROUP_MOVING);
  self.Portable::ungrab(object, dir);
}

void fuse_stop_sound(ArchetypeBadguy& self)
{
  if (auto& ticking = ecs::get<Fuse>(self.get_entity()).ticking) {
    ticking->stop();
  }
}

void fuse_play_sound(ArchetypeBadguy& self)
{
  if (auto& ticking = ecs::get<Fuse>(self.get_entity()).ticking) {
    ticking->play();
  }
}

// Stalactite ---------------------------------------------------------

constexpr int SHAKE_RANGE_X = 40;
constexpr float SHAKE_RANGE_Y = 400;

bool is_rock(Stalactite const& stalactite)
{
  return stalactite.type == "rock";
}

void stalactite_construct(ArchetypeBadguy& self)
{
  Stalactite const& stalactite = ecs::get<Stalactite>(self.get_entity());
  if (stalactite.type.empty()) {
    log_warning("No stalactite type set, setting to ice.");
  } else if (stalactite.type != "ice" && stalactite.type != "rock") {
    log_warning("Unknown type of stalactite:{}, setting to ice.", stalactite.type);
  }

  self.m_countMe = false;
  self.set_colgroup_active(COLGROUP_TOUCHABLE);
  SoundManager::current()->preload("sounds/cracking.wav");
  SoundManager::current()->preload("sounds/sizzle.ogg");
  SoundManager::current()->preload("sounds/icecrash.ogg");
}

void stalactite_move(ArchetypeBadguy& self, float dt_sec)
{
  Stalactite& stalactite = ecs::get<Stalactite>(self.get_entity());
  Rectf const& bbox = self.m_col.m_bbox;

  if (stalactite.state == Stalactite::State::HANGING) {
    auto player = self.get_nearest_player();
    if (player && !player->get_ghost_mode()) {
      if (player->get_bbox().get_right() > bbox.get_left() - SHAKE_RANGE_X
         && player->get_bbox().get_left() < bbox.get_right() + SHAKE_RANGE_X
         && player->get_bbox().get_bottom() > bbox.get_top()
         && player->get_bbox().get_top() < bbox.get_bottom() + SHAKE_RANGE_Y
         && Sector::get().can_see_player(bbox.get_middle())) {
        stalactite.timer.start(stalactite::SHAKE_TIME);
        stalactite.state = Stalactite::State::SHAKING;
        SoundManager::current()->play("sounds/cracking.wav", self.get_pos());
      }
    }
  } else if (stalactite.state == Stalactite::State::SHAKING) {
    stalactite.shake_delta = Vector(static_cast<float>(graphicsRandom.rand(-3, 3)), 0.0f);
    if (stalactite.timer.check()) {
      stalactite.state = Stalactite::State::FALLING;
      self.m_physic.enable_gravity(true);
      self.set_colgroup_active(COLGROUP_MOVING);
    }
  } else if (stalactite.state == Stalactite::State::FALLING) {
    self.m_col.set_movement(self.m_physic.get_movement(dt_sec));
  }
}

void stalactite_squish(ArchetypeBadguy& self, Stalactite& stalactite)
{
  stalactite.state = Stalactite::State::SQUISHED;
  self.m_physic.enable_gravity(true);
  self.m_physic.set_velocity_x(0);
  self.m_physic.set_velocity_y(0);
  self.set_state(ArchetypeBadguy::STATE_SQUISHED);
  self.m_sprite->set_action("squished");
  SoundManager::current()->play("sounds/icecrash.ogg", self.get_pos());
  self.set_group(COLGROUP_MOVING_ONLY_STATIC);
  self.run_dead_script();
}

void stalactite_collision_solid(ArchetypeBadguy& self, CollisionHit const& hit)
{
  Stalactite& stalactite = ecs::get<Stalactite>(self.get_entity());
  if (stalactite.state == Stalactite::State::FALLING) {
    if (hit.bottom) stalactite_squish(self, stalactite);
  }
  if (stalactite.state == Stalactite::State::SQUISHED) {
    self.m_physic.set_velocity_y(0);
  }
}

HitResponse stalactite_collision_player(ArchetypeBadguy& self, Player& player, CollisionHit const& /*hit*/)
{
  if (ecs::get<Stalactite>(self.get_entity()).state != Stalactite::State::SQUISHED) {
    player.kill(false);
  }
  return FORCE_MOVE;
}

HitResponse stalactite_collision_badguy(ArchetypeBadguy& self, BadGuy& other, CollisionHit const& hit)
{
  Stalactite const& stalactite = ecs::get<Stalactite>(self.get_entity());
  if (stalactite.state == Stalactite::State::SQUISHED) return FORCE_MOVE;

  // ignore other Stalactites
  if (ecs::try_get<Stalactite>(other.get_entity())) return FORCE_MOVE;

  if (stalactite.state != Stalactite::State::FALLING) return self.default_collision_badguy(other, hit);

  if (other.is_freezable() && !is_rock(stalactite)) {
    other.freeze();
  } else {
    other.kill_fall();
  }

  return FORCE_MOVE;
}

HitResponse stalactite_collision_bullet(ArchetypeBadguy& self, Bullet& bullet, CollisionHit const& hit)
{
  Stalactite& stalactite = ecs::get<Stalactite>(self.get_entity());
  if (is_rock(stalactite))
  {
    bullet.ricochet(self, hit);
  }
  else if (stalactite.state == Stalactite::State::HANGING)
  {
    stalactite.timer.start(stalactite::SHAKE_TIME);
    stalactite.state = Stalactite::State::SHAKING;
    bullet.remove_me();
    if (bullet.get_type() == FIRE_BONUS)
      SoundManager::current()->play("sounds/sizzle.ogg", self.get_pos());
    SoundManager::current()->play("sounds/cracking.wav", self.get_pos());
  }

  return FORCE_MOVE;
}

void stalactite_kill_fall(ArchetypeBadguy& /*self*/)
{
}

void stalactite_draw(ArchetypeBadguy& self, DrawingContext& context)
{
  if (self.get_state() == ArchetypeBadguy::STATE_INIT || self.get_state() == ArchetypeBadguy::STATE_INACTIVE)
    return;

  Stalactite const& stalactite = ecs::get<Stalactite>(self.get_entity());
  if (stalactite.state == Stalactite::State::SQUISHED) {
    self.m_sprite->draw(context.color(), self.get_pos(), LAYER_OBJECTS);
  } else if (stalactite.state == Stalactite::State::SHAKING) {
    self.m_sprite->draw(context.color(), self.get_pos() + stalactite.shake_delta, self.m_layer, self.m_flip);
  } else {
    self.m_sprite->draw(context.color(), self.get_pos(), self.m_layer, self.m_flip);
  }
}

void stalactite_deactivate(ArchetypeBadguy& self)
{
  if (ecs::get<Stalactite>(self.get_entity()).state != Stalactite::State::HANGING)
    self.remove_me();
}

// IceBlock -----------------------------------------------------------

void iceblock_set_state(ArchetypeBadguy& self, IceBlock& iceblock, IceBlock::State state)
{
  if (iceblock.state == state)
    return;

  switch (state) {
    case IceBlock::State::NORMAL:
      self.set_action(self.m_dir == Direction::LEFT ? "left" : "right", /* loops = */ -1);
      walker::initialize(self, ecs::get<Walker>(self.get_entity()));
      break;
    case IceBlock::State::FLAT:
      self.set_action(self.m_dir == Direction::LEFT ? "flat-left" : "flat-right", /* loops = */ -1);
      iceblock.flat_timer.start(iceblock.flat_time);
      break;
    case IceBlock::State::KICKED:
      SoundManager::current()->play("sounds/kick.wav", self.get_pos());
      self.m_physic.set_velocity_x(self.m_dir == Direction::LEFT ? -iceblock.kick_speed : iceblock.kick_speed);
      self.set_action(self.m_dir == Direction::LEFT ? "flat-left" : "flat-right", /* loops = */ -1);
      // we should slide above 1 block holes now...
      self.m_col.m_bbox.set_size(34, 31.8f);
      break;
    case IceBlock::State::GRABBED:
      iceblock.flat_timer.stop();
      break;
    case IceBlock::State::WAKING:
      self.m_sprite->set_action(self.m_dir == Direction::LEFT ? "waking-left" : "waking-right",
                                /* loops = */ 1);
      break;
  }
  iceblock.state = state;
}

void iceblock_construct(ArchetypeBadguy& /*self*/)
{
  SoundManager::current()->preload("sounds/iceblock_bump.wav");
  SoundManager::current()->preload("sounds/stomp.wav");
  SoundManager::current()->preload("sounds/kick.wav");
}

void iceblock_initialize(ArchetypeBadguy& self)
{
  walker::initialize(self, ecs::get<Walker>(self.get_entity()));
  iceblock_set_state(self, ecs::get<IceBlock>(self.get_entity()), IceBlock::State::NORMAL);
}

bool iceblock_update(ArchetypeBadguy& self, float dt_sec)
{
  IceBlock& iceblock = ecs::get<IceBlock>(self.get_entity());
  if (iceblock.state == IceBlock::State::GRABBED || self.is_grabbed())
    return false;

  if (iceblock.state == IceBlock::State::FLAT && iceblock.flat_timer.check()) {
    iceblock_set_state(self, iceblock, IceBlock::State::WAKING);
  }

  if (iceblock.state == IceBlock::State::WAKING && self.m_sprite->animation_done()) {
    iceblock_set_state(self, iceblock, IceBlock::State::NORMAL);
  }

  if (iceblock.state == IceBlock::State::NORMAL)
  {
    // move and walk
    return true;
  }

  self.default_move(dt_sec);
  return false;
}

bool iceblock_can_break(ArchetypeBadguy const& self)
{
  IceBlock const& iceblock = ecs::get<IceBlock>(self.get_entity());
  return iceblock.state == IceBlock::State::KICKED || iceblock.state == IceBlock::State::FLAT;
}

void iceblock_collision_solid(ArchetypeBadguy& self, CollisionHit const& hit)
{
  IceBlock& iceblock = ecs::get<IceBlock>(self.get_entity());
  Walker& walker = ecs::get<Walker>(self.get_entity());

  self.update_on_ground_flag(hit);

  if (hit.top || hit.bottom) { // floor or roof
    self.m_physic.set_velocity_y(0);
  }

  // hit left or right
  switch (iceblock.state) {
    case IceBlock::State::NORMAL:
      walker::collision_solid(self, walker, hit);
      break;
    case IceBlock::State::KICKED: {
      if ((hit.right && self.m_dir == Direction::RIGHT) || (hit.left && self.m_dir == Direction::LEFT)) {
        self.m_dir = (self.m_dir == Direction::LEFT) ? Direction::RIGHT : Direction::LEFT;
        SoundManager::current()->play("sounds/iceblock_bump.wav", self.get_pos());
        self.m_physic.set_velocity_x(-self.m_physic.get_velocity_x() * .975f);
      }
      self.set_action(self.m_dir == Direction::LEFT ? "flat-left" : "flat-right", /* loops = */ -1);
      if (fabsf(self.m_physic.get_velocity_x()) < walker.speed * 1.5f)
        iceblock_set_state(self, iceblock, IceBlock::State::NORMAL);
      break;
    }
    case IceBlock::State::FLAT:
    case IceBlock::State::WAKING:
      self.m_physic.set_velocity_x(0);
      break;
    case IceBlock::State::GRABBED:
      break;
  }
}

HitResponse iceblock_collision(ArchetypeBadguy& self, GameObject& object, CollisionHit const& hit)
{
  if (ecs::get<IceBlock>(self.get_entity()).state == IceBlock::State::GRABBED)
    return FORCE_MOVE;

  return self.default_collision(object, hit);
}

HitResponse iceblock_collision_player(ArchetypeBadguy& self, Player& player, CollisionHit const& hit)
{
  IceBlock& iceblock = ecs::get<IceBlock>(self.get_entity());

  // handle kicks from left or right side
  if ((iceblock.state == IceBlock::State::WAKING || iceblock.state == IceBlock::State::FLAT) &&
      self.get_state() == ArchetypeBadguy::STATE_ACTIVE) {
    if (hit.left) {
      self.m_dir = Direction::RIGHT;
      player.kick();
      iceblock_set_state(self, iceblock, IceBlock::State::KICKED);
      return FORCE_MOVE;
    }
    else if (hit.right) {
      self.m_dir = Direction::LEFT;
      player.kick();
      iceblock_set_state(self, iceblock, IceBlock::State::KICKED);
      return FORCE_MOVE;
    }
  }

  return self.default_collision_player(player, hit);
}

HitResponse iceblock_collision_badguy(ArchetypeBadguy& self, BadGuy& badguy, CollisionHit const& hit)
{
  switch (ecs::get<IceBlock>(self.get_entity()).state) {
    case IceBlock::State::NORMAL:
      return walker::collision_badguy(self, ecs::get<Walker>(self.get_entity()), badguy, hit);
    case IceBlock::State::FLAT:
    case IceBlock::State::WAKING:
      return FORCE_MOVE;
    case IceBlock::State::KICKED:
      badguy.kill_fall();
      return FORCE_MOVE;
    default:
      assert(false);
  }
  return ABORT_MOVE;
}

bool iceblock_collision_squished(ArchetypeBadguy& self, GameObject& object)
{
  IceBlock& iceblock = ecs::get<IceBlock>(self.get_entity());

  Player* player = dynamic_cast<Player*>(&object);
  if (player && (player->m_does_buttjump || player->is_invincible())) {
    player->bounce(self);
    self.kill_fall();
    return true;
  }

  switch (iceblock.state)
  {
    case IceBlock::State::KICKED:
      {
        auto badguy = dynamic_cast<BadGuy*>(&object);
        if (badguy) {
          badguy->kill_fall();
          break;
        }
      }
      [[fallthrough]];

    case IceBlock::State::NORMAL:
      {
        iceblock.squishcount++;
        if (iceblock.squishcount >= iceblock.max_squishes) {
          self.kill_fall();
          return true;
        }
      }

      SoundManager::current()->play("sounds/stomp.wav", self.get_pos());
      self.m_physic.set_velocity_x(0);
      self.m_physic.set_velocity_y(0);
      iceblock_set_state(self, iceblock, IceBlock::State::FLAT);
      iceblock.nokick_timer.start(iceblock.nokick_time);
      break;

    case IceBlock::State::FLAT:
    case IceBlock::State::WAKING:
      {
        auto movingobject = dynamic_cast<MovingObject*>(&object);
        if (movingobject && (movingobject->get_pos().x < self.get_pos().x)) {
          self.m_dir = Direction::RIGHT;
        } else {
          self.m_dir = Direction::LEFT;
        }
      }
      if (iceblock.nokick_timer.check()) iceblock_set_state(self, iceblock, IceBlock::State::KICKED);
      break;

    case IceBlock::State::GRABBED:
      assert(false);
      break;
  }

  if (player) player->bounce(self);
  return true;
}

void iceblock_grab(ArchetypeBadguy& self, MovingObject& object, Vector const& pos, Direction dir)
{
  self.Portable::grab(object, pos, dir);
  self.m_col.set_movement(pos - self.get_pos());
  self.m_dir = dir;
  self.set_action(dir == Direction::LEFT ? "flat-left" : "flat-right", /* loops = */ -1);
  iceblock_set_state(self, ecs::get<IceBlock>(self.get_entity()), IceBlock::State::GRABBED);
  self.set_colgroup_active(COLGROUP_DISABLED);
}

void iceblock_ungrab(ArchetypeBadguy& self, MovingObject& object, Direction dir)
{
  IceBlock& iceblock = ecs::get<IceBlock>(self.get_entity());

  auto player = dynamic_cast<Player*> (&object);
  if (player && (player->is_swimming() || player->is_water_jumping()))
  {
    //move icecube a little bit away as to not insta-kill Tux
    float swimangle = player->get_swimming_angle();
    self.m_col.m_bbox.move(Vector(std::cos(swimangle) * 48.f, std::sin(swimangle) * 48.f));
  }

  if (dir == Direction::UP) {
    self.m_physic.set_velocity_y(-iceblock.kick_speed);
    iceblock_set_state(self, iceblock, IceBlock::State::FLAT);
  }
  else if (dir == Direction::DOWN) {
    Vector mov(0, 32);
    if (Sector::get().is_free_of_statics(self.get_bbox().moved(mov), &self)) {
      // There is free space, so throw it down
      SoundManager::current()->play("sounds/kick.wav", self.get_pos());
      self.m_physic.set_velocity_y(iceblock.kick_speed);
    }
    iceblock_set_state(self, iceblock, IceBlock::State::FLAT);
  }
  else {
    self.m_dir = dir;
    iceblock_set_state(self, iceblock, IceBlock::State::KICKED);
  }

  self.set_colgroup_active(COLGROUP_MOVING);
  self.Portable::ungrab(object, dir);
}

bool iceblock_is_portable(ArchetypeBadguy const& self)
{
  return self.m_frozen || ecs::get<IceBlock>(self.get_entity()).state == IceBlock::State::FLAT;
}

void iceblock_ignite(ArchetypeBadguy& self)
{
  iceblock_set_state(self, ecs::get<IceBlock>(self.get_entity()), IceBlock::State::NORMAL);
  self.default_ignite();
}

// Boarder ------------------------------------------------------------

bool boarder_might_climb(ArchetypeBadguy const& self, int width, int height)
{
  // make sure we check for at least a 1-pixel climb
  assert(height > 0);

  Rectf const& bbox = self.m_col.m_bbox;
  float x1;
  float x2;
  float y1a = bbox.get_top() + 1;
  float y2a = bbox.get_bottom() - 1;
  float y1b = bbox.get_top() + 1 - static_cast<float>(height);
  float y2b = bbox.get_bottom() - 1 - static_cast<float>(height);
  if (self.m_dir == Direction::LEFT) {
    x1 = bbox.get_left() - static_cast<float>(width);
    x2 = bbox.get_left() - 1;
  } else {
    x1 = bbox.get_right() + 1;
    x2 = bbox.get_right() + static_cast<float>(width);
  }
  return ((!Sector::get().is_free_of_statics(Rectf(x1, y1a, x2, y2a))) &&
          (Sector::get().is_free_of_statics(Rectf(x1, y1b, x2, y2b))));
}

void boarder_construct(ArchetypeBadguy& self)
{
  self.m_physic.set_velocity_y(ecs::get<Boarder>(self.get_entity()).jump_speed);
}

bool boarder_update(ArchetypeBadguy& self, float /*dt_sec*/)
{
  Boarder const& boarder = ecs::get<Boarder>(self.get_entity());
  Walker& walker = ecs::get<Walker>(self.get_entity());

  if (self.on_ground() && boarder_might_climb(self, 8, 64)) {
    self.m_physic.set_velocity_y(boarder.jump_speed);
  } else if (self.on_ground() && self.might_fall(16)) {
    self.m_physic.set_velocity_y(boarder.jump_speed);
    walker.speed = boarder.board_speed;
    self.m_physic.set_velocity_x(self.m_dir == Direction::LEFT ? -walker.speed : walker.speed);
  }
  return true;
}

void boarder_collision_solid(ArchetypeBadguy& self, CollisionHit const& hit)
{
  Boarder const& boarder = ecs::get<Boarder>(self.get_entity());
  Walker& walker = ecs::get<Walker>(self.get_entity());

  if (self.is_active() && (walker.speed == boarder.board_speed)) {
    walker.speed = boarder.walk_speed;
    self.m_physic.set_velocity_x(self.m_dir == Direction::LEFT ? -walker.speed : walker.speed);
  }
  walker::collision_solid(self, walker, hit);
}

// Sleeper ------------------------------------------------------------

void sleeper_initialize(ArchetypeBadguy& self)
{
  ecs::get<Sleeper>(self.get_entity()).state = Sleeper::State::SLEEPING;
  self.m_physic.set_velocity_x(0);
  self.m_sprite->set_action("sleeping", self.m_dir);
}

bool sleeper_update(ArchetypeBadguy& self, float dt_sec)
{
  Sleeper& sleeper = ecs::get<Sleeper>(self.get_entity());

  if (sleeper.state == Sleeper::State::WALKING) {
    return true;
  }

  if (sleeper.state == Sleeper::State::SLEEPING) {
    if (Player* player = self.get_nearest_player()) {
      Rectf const& bbox = self.m_col.m_bbox;
      Rectf pb = player->get_bbox();

      bool inReach_left = (pb.get_right() >= bbox.get_right() - ((self.m_dir == Direction::LEFT) ? sleeper.reach : 0));
      bool inReach_right = (pb.get_left() <= bbox.get_left() + ((self.m_dir == Direction::RIGHT) ? sleeper.reach : 0));
      bool inReach_top = (pb.get_bottom() >= bbox.get_top());
      bool inReach_bottom = (pb.get_top() <= bbox.get_bottom());

      if (inReach_left && inReach_right && inReach_top && inReach_bottom) {
        // wake up
        self.m_sprite->set_action("waking", self.m_dir, 1);
        sleeper.state = Sleeper::State::WAKING;
      }
    }

    self.default_move(dt_sec);
  }

  if (sleeper.state == Sleeper::State::WAKING) {
    if (self.m_sprite->animation_done()) {
      // start walking
      sleeper.state = Sleeper::State::WALKING;
      walker::initialize(self, ecs::get<Walker>(self.get_entity()));
    }

    self.default_move(dt_sec);
  }

  return false;
}

void sleeper_collision_solid(ArchetypeBadguy& self, CollisionHit const& hit)
{
  if (ecs::get<Sleeper>(self.get_entity()).state != Sleeper::State::WALKING) {
    self.default_collision_solid(hit);
    return;
  }
  walker::collision_solid(self, ecs::get<Walker>(self.get_entity()), hit);
}

HitResponse sleeper_collision_badguy(ArchetypeBadguy& self, BadGuy& badguy, CollisionHit const& hit)
{
  if (ecs::get<Sleeper>(self.get_entity()).state != Sleeper::State::WALKING) {
    return self.default_collision_badguy(badguy, hit);
  }
  return walker::collision_badguy(self, ecs::get<Walker>(self.get_entity()), badguy, hit);
}

void sleeper_freeze(ArchetypeBadguy& self)
{
  self.default_freeze();
  // if we get hit while sleeping, wake up :)
  ecs::get<Sleeper>(self.get_entity()).state = Sleeper::State::WALKING;
}

bool sleeper_is_flammable(ArchetypeBadguy const& self)
{
  return ecs::get<Sleeper>(self.get_entity()).state != Sleeper::State::SLEEPING;
}

// BulletShy ----------------------------------------------------------

bool shy_can_see(ArchetypeBadguy const& self, BulletShy const& shy, MovingObject const& o)
{
  Rectf const& bbox = self.m_col.m_bbox;
  Rectf ob = o.get_bbox();

  bool inReach_left = ((ob.get_right() < bbox.get_left()) &&
                       (ob.get_right() >= bbox.get_left() - ((self.m_dir == Direction::LEFT) ? shy.range_of_vision : 0)));
  bool inReach_right = ((ob.get_left() > bbox.get_right()) &&
                        (ob.get_left() <= bbox.get_right() + ((self.m_dir == Direction::RIGHT) ? shy.range_of_vision : 0)));
  bool inReach_top = (ob.get_bottom() >= bbox.get_top());
  bool inReach_bottom = (ob.get_top() <= bbox.get_bottom());

  return ((inReach_left || inReach_right) && inReach_top && inReach_bottom);
}

bool shy_update(ArchetypeBadguy& self, float dt_sec)
{
  BulletShy& shy = ecs::get<BulletShy>(self.get_entity());

  bool wants_to_flee = false;

  // check if we see a fire bullet
  for (auto const& bullet : Sector::get().get_objects_by_type<Bullet>()) {
    if (bullet.get_type() != FIRE_BONUS) continue;
    if (shy_can_see(self, shy, bullet)) wants_to_flee = true;
  }

  // if we flee, handle this ourselves
  if (wants_to_flee && (!shy.turn_recover_timer.started())) {
    walker::turn_around(self, ecs::get<Walker>(self.get_entity()));
    shy.turn_recover_timer.start(shy.turn_recover_time);
    self.default_move(dt_sec);
    return false;
  }

  // else adhere to default behaviour
  return true;
}

HitResponse shy_collision_bullet(ArchetypeBadguy& self, Bullet& bullet, CollisionHit const& hit)
{
  // default reaction if hit on front side or for freeze and unfreeze
  if (((self.m_dir == Direction::LEFT) && hit.left) || ((self.m_dir == Direction::RIGHT) && hit.right) ||
      (bullet.get_type() == ICE_BONUS) || ((bullet.get_type() == FIRE_BONUS) && (self.m_frozen))) {
    return self.default_collision_bullet(bullet, hit);
  }

  // else make bullet ricochet and ignore the hit
  bullet.ricochet(self, hit);
  return FORCE_MOVE;
}

// Firecracker --------------------------------------------------------

void firecracker_explode(ArchetypeBadguy& self)
{
  if (!self.is_valid())
    return;

  if (self.m_frozen)
    self.default_kill_fall();
  else
  {
    auto& explosion = Sector::get().add<Explosion>(self.get_bbox().get_middle(),
                                                   EXPLOSION_STRENGTH_NEAR, 8);
    explosion.hurts(false);
    self.run_dead_script();
    self.remove_me();
  }
}

void firecracker_construct(ArchetypeBadguy& /*self*/)
{
  SoundManager::current()->preload("sounds/firecracker.ogg");
}

bool firecracker_collision_squished(ArchetypeBadguy& self, GameObject& object)
{
  if (self.m_frozen)
    return self.default_collision_squished(object);

  if (!self.is_valid())
    return true;

  if (auto player = dynamic_cast<Player*>(&object))
    player->bounce(self);

  firecracker_explode(self);
  return true;
}

HitResponse firecracker_collision_player(ArchetypeBadguy& self, Player& player, CollisionHit const& /*hit*/)
{
  if (!self.m_frozen)
  {
    player.bounce(self);
    firecracker_explode(self);
  }
  return FORCE_MOVE;
}

void firecracker_freeze(ArchetypeBadguy& self)
{
  self.m_col.m_bbox.move(Vector(0.f, -100.f));
  self.default_freeze();
}

void firecracker_ignite(ArchetypeBadguy& self)
{
  if (self.m_frozen)
    self.unfreeze();
  self.kill_fall();
}

// Snail --------------------------------------------------------------

void snail_be_normal(ArchetypeBadguy& self, Snail& snail)
{
  if (snail.state == Snail::State::NORMAL) return;

  snail.state = Snail::State::NORMAL;
  walker::initialize(self, ecs::get<Walker>(self.get_entity()));
}

void snail_be_flat(ArchetypeBadguy& self, Snail& snail)
{
  snail.state = Snail::State::FLAT;
  self.m_sprite->set_action("flat", self.m_dir, /* loops = */ -1);

  self.m_physic.set_velocity_x(0);
  self.m_physic.set_velocity_y(0);

  snail.flat_timer.start(snail.flat_time);
}

void snail_be_grabbed(ArchetypeBadguy& self, Snail& snail)
{
  snail.state = Snail::State::GRABBED;
  self.m_sprite->set_action("flat", self.m_dir, /* loops = */ -1);
}

void snail_be_kicked(ArchetypeBadguy& self, Snail& snail, bool upwards)
{
  if (upwards)
    snail.state = Snail::State::KICKED_DELAY;
  else
    snail.state = Snail::State::KICKED;
  self.m_sprite->set_action("flat", self.m_dir, /* loops = */ -1);

  self.m_physic.set_velocity_x(self.m_dir == Direction::LEFT ? -snail.kick_speed : snail.kick_speed);
  self.m_physic.set_velocity_y(0);

  // start a timer to delay addition of upward movement until we are (hopefully) out from under the player
  if (upwards)
    snail.kicked_delay_timer.start(0.05f);
}

void snail_wake_up(ArchetypeBadguy& self, Snail& snail)
{
  snail.state = Snail::State::WAKING;
  self.m_sprite->set_action(self.m_dir == Direction::LEFT ? "waking-left" : "waking-right", /* loops = */ 1);
}

void snail_construct(ArchetypeBadguy& /*self*/)
{
  SoundManager::current()->preload("sounds/iceblock_bump.wav");
  SoundManager::current()->preload("sounds/stomp.wav");
  SoundManager::current()->preload("sounds/kick.wav");
}

void snail_initialize(ArchetypeBadguy& self)
{
  walker::initialize(self, ecs::get<Walker>(self.get_entity()));
  snail_be_normal(self, ecs::get<Snail>(self.get_entity()));
}

bool snail_can_break(ArchetypeBadguy const& self)
{
  return ecs::get<Snail>(self.get_entity()).state == Snail::State::KICKED;
}

bool snail_update(ArchetypeBadguy& self, float dt_sec)
{
  Snail& snail = ecs::get<Snail>(self.get_entity());

  if (snail.state == Snail::State::GRABBED || self.is_grabbed())
    return false;

  if (self.m_frozen)
  {
    self.default_move(dt_sec);
    return false;
  }

  switch (snail.state) {
    case Snail::State::NORMAL:
      // move and walk
      return true;

    case Snail::State::FLAT:
      if (snail.flat_timer.check())
        snail_wake_up(self, snail);
      break;

    case Snail::State::WAKING:
      if (self.m_sprite->animation_done())
        snail_be_normal(self, snail);
      break;

    case Snail::State::KICKED_DELAY:
      if (snail.kicked_delay_timer.check()) {
        self.m_physic.set_velocity_x(self.m_dir == Direction::LEFT ? -snail.kick_speed : snail.kick_speed);
        self.m_physic.set_velocity_y(snail.kick_speed_y);
        snail.state = Snail::State::KICKED;
      }
      break;

    case Snail::State::KICKED:
      self.m_physic.set_velocity_x(self.m_physic.get_velocity_x() * powf(0.99f, dt_sec/0.02f));
      if (fabsf(self.m_physic.get_velocity_x()) < ecs::get<Walker>(self.get_entity()).speed)
        snail_be_normal(self, snail);
      break;

    case Snail::State::GRABBED:
      break;
  }

  self.default_move(dt_sec);

  if (self.m_ignited)
    self.remove_me();

  return false;
}

void snail_collision_solid(ArchetypeBadguy& self, CollisionHit const& hit)
{
  Snail& snail = ecs::get<Snail>(self.get_entity());
  Walker& walker = ecs::get<Walker>(self.get_entity());

  if (self.m_frozen)
  {
    walker::collision_solid(self, walker, hit);
    return;
  }

  switch (snail.state)
  {
    case Snail::State::NORMAL:
      walker::collision_solid(self, walker, hit);
      return;
    case Snail::State::KICKED:
      if (hit.left || hit.right) {
        SoundManager::current()->play("sounds/iceblock_bump.wav", self.get_pos());

        if ((self.m_dir == Direction::LEFT && hit.left) || (self.m_dir == Direction::RIGHT && hit.right)) {
          self.m_dir = (self.m_dir == Direction::LEFT) ? Direction::RIGHT : Direction::LEFT;
          self.m_sprite->set_action("flat", self.m_dir, /* loops = */ -1);

          self.m_physic.set_velocity_x(-self.m_physic.get_velocity_x());
        }
      }
      [[fallthrough]];
    case Snail::State::FLAT:
    case Snail::State::KICKED_DELAY:
    case Snail::State::WAKING:
      if (hit.top || hit.bottom) {
        self.m_physic.set_velocity_y(0);
      }
      break;
    case Snail::State::GRABBED:
      break;
  }

  self.update_on_ground_flag(hit);
}

HitResponse snail_collision_badguy(ArchetypeBadguy& self, BadGuy& badguy, CollisionHit const& hit)
{
  Snail& snail = ecs::get<Snail>(self.get_entity());
  Walker& walker = ecs::get<Walker>(self.get_entity());

  if (self.m_frozen)
    return walker::collision_badguy(self, walker, badguy, hit);

  switch (snail.state) {
    case Snail::State::NORMAL:
      return walker::collision_badguy(self, walker, badguy, hit);
    case Snail::State::FLAT:
    case Snail::State::KICKED_DELAY:
    case Snail::State::WAKING:
      return FORCE_MOVE;
    case Snail::State::KICKED:
      badguy.kill_fall();
      return FORCE_MOVE;
    default:
      assert(false);
  }

  return ABORT_MOVE;
}

HitResponse snail_collision_player(ArchetypeBadguy& self, Player& player, CollisionHit const& hit)
{
  Snail& snail = ecs::get<Snail>(self.get_entity());

  if (self.m_frozen)
    return self.default_collision_player(player, hit);

  // handle kicks from left or right side
  if ((snail.state == Snail::State::WAKING || snail.state == Snail::State::FLAT) && (hit.left || hit.right)) {
    if (hit.left) {
      self.m_dir = Direction::RIGHT;
    } else if (hit.right) {
      self.m_dir = Direction::LEFT;
    }
    player.kick();
    snail_be_kicked(self, snail, false);
    return FORCE_MOVE;
  }

  return self.default_collision_player(player, hit);
}

bool snail_collision_squished(ArchetypeBadguy& self, GameObject& object)
{
  Snail& snail = ecs::get<Snail>(self.get_entity());

  if (self.m_frozen)
    return self.default_collision_squished(object);

  Player* player = dynamic_cast<Player*>(&object);
  if (player && (player->is_invincible() || player->m_does_buttjump)) {
    self.kill_fall();
    player->bounce(self);
    return true;
  }

  switch (snail.state) {
    case Snail::State::NORMAL:
      [[fallthrough]];
    case Snail::State::KICKED:
      snail.squishcount++;
      if (snail.squishcount >= snail.max_squishes) {
        self.kill_fall();
        return true;
      }
      SoundManager::current()->play("sounds/stomp.wav", self.get_pos());
      snail_be_flat(self, snail);
      break;

    case Snail::State::FLAT:
    case Snail::State::WAKING:
      SoundManager::current()->play("sounds/kick.wav", self.get_pos());
      {
        MovingObject* movingobject = dynamic_cast<MovingObject*>(&object);
        if (movingobject && (movingobject->get_pos().x < self.get_pos().x)) {
          self.m_dir = Direction::RIGHT;
        } else {
          self.m_dir = Direction::LEFT;
        }
      }
      snail_be_kicked(self, snail, true);
      break;

    case Snail::State::GRABBED:
    case Snail::State::KICKED_DELAY:
      break;
  }

  if (player) player->bounce(self);
  return true;
}

void snail_grab(ArchetypeBadguy& self, MovingObject& object, Vector const& pos, Direction dir)
{
  Snail& snail = ecs::get<Snail>(self.get_entity());

  self.Portable::grab(object, pos, dir);
  if (self.m_frozen)
    self.BadGuy::grab(object, pos, dir);
  self.m_col.set_movement(pos - self.get_pos());
  self.m_dir = dir;
  if (!self.m_frozen)
  {
    self.set_action(dir == Direction::LEFT ? "flat-left" : "flat-right", /* loops = */ -1);
    snail_be_grabbed(self, snail);
    snail.flat_timer.stop();
  }
  self.set_colgroup_active(COLGROUP_DISABLED);
}

void snail_ungrab(ArchetypeBadguy& self, MovingObject& object, Direction dir)
{
  Snail& snail = ecs::get<Snail>(self.get_entity());

  if (!self.m_frozen)
  {
    if (dir == Direction::UP) {
      snail_be_flat(self, snail);
    }
    else {
      self.m_dir = dir;
      snail_be_kicked(self, snail, ecs::try_get<Owl>(object.get_entity()) ? false : true);
    }
  }
  else
    self.BadGuy::ungrab(object, dir);

  self.set_colgroup_active(self.m_frozen ? COLGROUP_MOVING_STATIC : COLGROUP_MOVING);
  self.Portable::ungrab(object, dir);
}

bool snail_is_portable(ArchetypeBadguy const& self)
{
  return (ecs::get<Snail>(self.get_entity()).state == Snail::State::FLAT || self.m_frozen) && !self.m_ignited;
}

// Snowman ------------------------------------------------------------

void snowman_spawn_head(ArchetypeBadguy& self)
{
  // Hard-coded values from sprites
  Vector head_pos = self.get_pos() + Vector(5, 1);
  Sector::get().add_object(ArchetypeBadguy::create(ecs::get<Snowman>(self.get_entity()).head,
                                                   head_pos, self.m_dir, self.m_dead_script));
}

void snowman_construct(ArchetypeBadguy& /*self*/)
{
  SoundManager::current()->preload("sounds/pop.ogg");
}

HitResponse snowman_collision_bullet(ArchetypeBadguy& self, Bullet& bullet, CollisionHit const& hit)
{
  if (bullet.get_type() == FIRE_BONUS) {
    // fire bullets destroy snowman's body
    snowman_spawn_head(self);
    self.m_countMe = false;
    SoundManager::current()->play("sounds/pop.ogg", self.get_pos()); // this could be a different sound
    bullet.remove_me();
    self.ignite();
    return ABORT_MOVE;
  }
  else {
    // in all other cases, bullets ricochet
    bullet.ricochet(self, hit);
    return FORCE_MOVE;
  }
}

bool snowman_collision_squished(ArchetypeBadguy& self, GameObject& object)
{
  auto player = dynamic_cast<Player*>(&object);
  if (player && (player->m_does_buttjump || player->is_invincible())) {
    player->bounce(self);
    self.kill_fall();
    return true;
  }

  // bounce
  if (player)
    player->bounce(self);

  SoundManager::current()->play("sounds/pop.ogg", self.get_pos());

  // loose head: the remaining body falls off the screen
  Vector const head_pos = self.get_pos();
  self.set_action(self.m_dir == Direction::LEFT ? "headless-left" : "headless-right", /* loops = */ -1);
  self.set_pos(self.get_pos() + Vector(-4.0, 19.0)); /* difference in the sprite offsets */
  self.m_physic.set_velocity_y(0);
  self.m_physic.set_acceleration_y(0);
  self.m_physic.enable_gravity(true);
  self.set_state(ArchetypeBadguy::STATE_FALLING);
  self.m_countMe = false;

  /* Create a new badguy where the snowman's head was */
  Sector::get().add_object(ArchetypeBadguy::create(ecs::get<Snowman>(self.get_entity()).head,
                                                   head_pos + Vector(5, 1), self.m_dir, self.m_dead_script));
  return true;
}

// MrTree -------------------------------------------------------------

constexpr float SPROUT_WIDTH = 32;
constexpr float SPROUT_HEIGHT = 32;
constexpr float SPROUT_Y_OFFSET = 24;

void mrtree_construct(ArchetypeBadguy& /*self*/)
{
  SoundManager::current()->preload("sounds/mr_tree.ogg");
}

bool mrtree_collision_squished(ArchetypeBadguy& self, GameObject& object)
{
  MrTree const& tree = ecs::get<MrTree>(self.get_entity());

  if (self.m_frozen)
    return self.default_collision_squished(object);

  auto player = dynamic_cast<Player*>(&object);
  if (player && (player->m_does_buttjump || player->is_invincible())) {
    player->bounce(self);
    self.kill_fall();
    return true;
  }

  // replace with the stump
  Vector stump_pos = self.get_pos() + Vector(20, 25);
  auto stump = ArchetypeBadguy::create(tree.stump, stump_pos, self.m_dir);
  Rectf const stump_bbox = stump->get_bbox();
  Sector::get().add_object(std::move(stump));
  self.remove_me();

  // give Feedback
  SoundManager::current()->play("sounds/mr_tree.ogg", self.get_pos());
  if (player) player->bounce(self);

  // spawn some particles
  for (int px = static_cast<int>(stump_bbox.get_left()); px < static_cast<int>(stump_bbox.get_right()); px+=10) {
    Vector ppos = Vector(static_cast<float>(px),
                         static_cast<float>(stump_bbox.get_top()) - 5.0f);
    float angle = graphicsRandom.randf(-math::PI_2, math::PI_2);
    float velocity = graphicsRandom.randf(45, 90);
    float vx = sinf(angle)*velocity;
    float vy = -cosf(angle)*velocity;
    Vector pspeed = Vector(vx, vy);
    Vector paccel = Vector(0, Sector::get().get_gravity()*10);
    Sector::get().add<SpriteParticle>("images/particles/leaf.sprite",
                                      "default",
                                      ppos, ANCHOR_MIDDLE,
                                      pspeed, paccel,
                                      LAYER_OBJECTS-1);
  }

  if (!self.m_frozen) { //Frozen Mr.Trees don't spawn any sprouts.
    Vector sprout1_pos(stump_pos.x - SPROUT_WIDTH - 1, stump_pos.y - SPROUT_Y_OFFSET);
    Rectf sprout1_bbox(sprout1_pos.x, sprout1_pos.y, sprout1_pos.x + SPROUT_WIDTH, sprout1_pos.y + SPROUT_HEIGHT);
    if (Sector::get().is_free_of_movingstatics(sprout1_bbox, &self)) {
      auto sprout = ArchetypeBadguy::create(tree.sprout, sprout1_bbox.p1(), Direction::LEFT);
      sprout->m_countMe = false;
      Sector::get().add_object(std::move(sprout));
    }

    Vector sprout2_pos(stump_pos.x + self.m_sprite->get_current_hitbox_width() + 1, stump_pos.y - SPROUT_Y_OFFSET);
    Rectf sprout2_bbox(sprout2_pos.x, sprout2_pos.y, sprout2_pos.x + SPROUT_WIDTH, sprout2_pos.y + SPROUT_HEIGHT);
    if (Sector::get().is_free_of_movingstatics(sprout2_bbox, &self)) {
      auto sprout = ArchetypeBadguy::create(tree.sprout, sprout2_bbox.p1(), Direction::RIGHT);
      sprout->m_countMe = false;
      Sector::get().add_object(std::move(sprout));
    }
  }
  return true;
}

// Stumpy -------------------------------------------------------------

void stumpy_construct(ArchetypeBadguy& self)
{
  SoundManager::current()->preload("sounds/mr_treehit.ogg");

  // a stump left behind by a MrTree is dizzy at first
  if (self.is_spawned()) {
    Stumpy& stumpy = ecs::get<Stumpy>(self.get_entity());
    stumpy.invincible = true;
    stumpy.invincible_timer.start(stumpy.invincible_time);
  }
}

void stumpy_initialize(ArchetypeBadguy& self)
{
  if (ecs::get<Stumpy>(self.get_entity()).invincible) {
    self.m_sprite->set_action("dizzy", self.m_dir);
    self.m_col.m_bbox.set_size(self.m_sprite->get_current_hitbox_width(), self.m_sprite->get_current_hitbox_height());
    self.m_physic.set_velocity_x(0);
  } else {
    walker::initialize(self, ecs::get<Walker>(self.get_entity()));
  }
}

bool stumpy_update(ArchetypeBadguy& self, float dt_sec)
{
  Stumpy& stumpy = ecs::get<Stumpy>(self.get_entity());
  if (!stumpy.invincible) {
    return true;
  }

  if (stumpy.invincible_timer.check()) {
    stumpy.invincible = false;
    walker::initialize(self, ecs::get<Walker>(self.get_entity()));
  }
  self.default_move(dt_sec);
  return false;
}

bool stumpy_collision_squished(ArchetypeBadguy& self, GameObject& object)
{
  if (self.m_frozen)
    return self.default_collision_squished(object);

  // if we're still invincible, we ignore the hit
  if (ecs::get<Stumpy>(self.get_entity()).invincible) {
    SoundManager::current()->play("sounds/mr_treehit.ogg", self.get_pos());
    if (auto player = dynamic_cast<Player*>(&object)) player->bounce(self);
    return true;
  }

  // if we can die, we do
  self.m_sprite->set_action("squished", self.m_dir);
  self.m_col.set_size(self.m_sprite->get_current_hitbox_width(), self.m_sprite->get_current_hitbox_height());
  self.kill_squished(object);

  // spawn some particles
  for (int i = 0; i < 25; i++) {
    Vector ppos = self.m_col.m_bbox.get_middle();
    float angle = graphicsRandom.randf(-math::PI_2, math::PI_2);
    float velocity = graphicsRandom.randf(45, 90);
    float vx = sinf(angle)*velocity;
    float vy = -cosf(angle)*velocity;
    Vector pspeed = Vector(vx, vy);
    Vector paccel = Vector(0, Sector::get().get_gravity()*10);
    Sector::get().add<SpriteParticle>("images/particles/bark.sprite",
                                      "default",
                                      ppos, ANCHOR_MIDDLE,
                                      pspeed, paccel,
                                      LAYER_OBJECTS-1);
  }
  return true;
}

void stumpy_stop_on_hit(ArchetypeBadguy& self, CollisionHit const& hit)
{
  if (hit.top || hit.bottom) {
    self.m_physic.set_velocity_y(0);
  }
  if (hit.left || hit.right) {
    self.m_physic.set_velocity_x(0);
  }
}

void stumpy_collision_solid(ArchetypeBadguy& self, CollisionHit const& hit)
{
  self.update_on_ground_flag(hit);

  if (ecs::get<Stumpy>(self.get_entity()).invincible) {
    stumpy_stop_on_hit(self, hit);
  } else {
    walker::collision_solid(self, ecs::get<Walker>(self.get_entity()), hit);
  }
}

HitResponse stumpy_collision_badguy(ArchetypeBadguy& self, BadGuy& badguy, CollisionHit const& hit)
{
  if (ecs::get<Stumpy>(self.get_entity()).invincible) {
    stumpy_stop_on_hit(self, hit);
    return CONTINUE;
  }
  return walker::collision_badguy(self, ecs::get<Walker>(self.get_entity()), badguy, hit);
}

// JumpingFish --------------------------------------------------------

void fish_start_waiting(ArchetypeBadguy& self, JumpingFish& fish)
{
  fish.wait_timer.start(fish.wait_time);
  self.m_physic.enable_gravity(false);
  self.m_physic.set_velocity_y(0);
}

void fish_construct(ArchetypeBadguy& self)
{
  self.m_physic.enable_gravity(true);
}

void fish_hit(ArchetypeBadguy& self, CollisionHit const& hit)
{
  if (hit.top)
    self.m_physic.set_velocity_y(0);
}

void fish_collision_solid(ArchetypeBadguy& self, CollisionHit const& hit)
{
  JumpingFish& fish = ecs::get<JumpingFish>(self.get_entity());

  fish_hit(self, hit);
  if (!self.m_in_water && hit.bottom && !self.m_frozen)
  {
    self.m_physic.set_velocity_y(-300.f);
    if (!fish.beached_timer.started())
      fish.beached_timer.start(fish.beach_time);
  }

  if (self.m_frozen)
    self.default_collision_solid(hit);
}

HitResponse fish_collision_badguy(ArchetypeBadguy& self, BadGuy& /*other*/, CollisionHit const& hit)
{
  if (ecs::get<JumpingFish>(self.get_entity()).beached_timer.started())
    self.collision_solid(hit);

  fish_hit(self, hit);
  return CONTINUE;
}

void fish_collision_tile(ArchetypeBadguy& self, uint32_t tile_attributes)
{
  JumpingFish& fish = ecs::get<JumpingFish>(self.get_entity());

  if ((tile_attributes & Tile::WATER) && (self.m_physic.get_velocity_y() >= 0)) {
    if (fish.beached_timer.started())
      fish.beached_timer.stop();

    // initialize stop position if uninitialized
    if (fish.stop_y == 0) fish.stop_y = self.get_pos().y + self.m_col.m_bbox.get_height();

    // stop when we have reached the stop position
    if (self.get_pos().y >= fish.stop_y && self.m_physic.get_velocity_y() > 0.f) {
      if (!self.m_frozen)
        fish_start_waiting(self, fish);
      self.m_col.set_movement(Vector(0, 0));
    }
  }

  if ((!(tile_attributes & Tile::WATER) || self.m_frozen) && (tile_attributes & Tile::HURTS)) {
    self.kill_fall();
  }
}

void fish_after_move(ArchetypeBadguy& self, float /*dt_sec*/)
{
  JumpingFish& fish = ecs::get<JumpingFish>(self.get_entity());

  self.m_in_water = !Sector::get().is_free_of_tiles(self.get_bbox(), true, Tile::WATER);

  if (fish.beached_timer.check())
  {
    self.ignite();
    self.m_physic.reset();
    self.m_physic.enable_gravity(false);
    fish.beached_timer.stop();
  }

  // waited long enough?
  if (fish.wait_timer.check()) {
    self.m_physic.set_velocity_y(fish.jump_speed);
    self.m_physic.enable_gravity(true);
  }

  // set sprite
  if (!self.m_frozen && !self.is_ignited())
    self.m_sprite->set_action((self.m_physic.get_velocity_y() == 0.f && self.m_in_water) ? "wait" :
                              self.m_physic.get_velocity_y() < 0.f ? "normal" : "down");

  // we can't afford flying out of the tilemap, 'cause the engine would remove us.
  if ((self.get_pos().y - 31.8f) < 0) // too high, let us fall
  {
    self.m_physic.set_velocity_y(0);
    self.m_physic.enable_gravity(true);
  }

  if (self.m_ignited && self.m_in_water)
    self.remove_me();
}

void fish_freeze(ArchetypeBadguy& self)
{
  JumpingFish& fish = ecs::get<JumpingFish>(self.get_entity());

  self.default_freeze();
  self.m_physic.enable_gravity(true);
  self.m_sprite->set_action(self.m_physic.get_velocity_y() < 0 ? "iced" : "iced-down");
  self.m_sprite->set_color(Color(1.0f, 1.0f, 1.0f));
  fish.wait_timer.stop();
  if (fish.beached_timer.started())
    fish.beached_timer.stop();
}

void fish_unfreeze(ArchetypeBadguy& self, bool melt)
{
  self.m_dir = Direction::LEFT;
  self.default_unfreeze(melt);
}

void fish_kill_fall(ArchetypeBadguy& self)
{
  if (!self.is_ignited())
    self.m_sprite->set_action("normal");
  self.default_kill_fall();
}

// Hopper -------------------------------------------------------------

void hopper_set_state(ArchetypeBadguy& self, Hopper& hopper, Hopper::State state)
{
  if (state == Hopper::State::STANDING) {
    self.m_physic.set_velocity_x(0);
    self.m_physic.set_velocity_y(0);
    self.m_sprite->set_action("standing", self.m_dir);
    hopper.recover_timer.start(hopper.recover_time);
  } else if (state == Hopper::State::CHARGING) {
    self.m_sprite->set_action("charging", self.m_dir, 1);
  } else if (state == Hopper::State::JUMPING) {
    self.m_sprite->set_action("jumping", self.m_dir);
    self.m_physic.set_velocity_x(self.m_dir == Direction::LEFT ? -hopper.jump_speed_x : hopper.jump_speed_x);
    self.m_physic.set_velocity_y(hopper.jump_speed_y);
    SoundManager::current()->play(hopper.sound, self.get_pos());
  }

  hopper.state = state;
}

void hopper_construct(ArchetypeBadguy& self)
{
  SoundManager::current()->preload(ecs::get<Hopper>(self.get_entity()).sound);
}

void hopper_initialize(ArchetypeBadguy& self)
{
  // initial state is JUMPING, because we might start airborne
  ecs::get<Hopper>(self.get_entity()).state = Hopper::State::JUMPING;
  self.m_sprite->set_action("jumping", self.m_dir);
}

void hopper_collision_solid(ArchetypeBadguy& self, CollisionHit const& hit)
{
  Hopper& hopper = ecs::get<Hopper>(self.get_entity());

  if (self.m_frozen || self.get_state() == ArchetypeBadguy::STATE_BURNING)
  {
    self.default_collision_solid(hit);
    return;
  }

  // just default behaviour (i.e. stop at floor/walls) when squished
  if (self.get_state() == ArchetypeBadguy::STATE_SQUISHED) {
    self.default_collision_solid(hit);
  }

  // ignore collisions while standing still
  if (hopper.state != Hopper::State::JUMPING)
    return;

  // check if we hit the floor while falling
  if (hit.bottom && self.m_physic.get_velocity_y() > 0) {
    hopper_set_state(self, hopper, Hopper::State::STANDING);
  }
  // check if we hit the roof while climbing
  if (hit.top) {
    self.m_physic.set_velocity_y(0);
  }

  // check if we hit left or right while moving in either direction
  if (hit.left || hit.right) {
    self.m_dir = self.m_dir == Direction::LEFT ? Direction::RIGHT : Direction::LEFT;
    self.m_sprite->set_action("jumping", self.m_dir);
    self.m_physic.set_velocity_x(-0.25f*self.m_physic.get_velocity_x());
  }
}

HitResponse hopper_collision_badguy(ArchetypeBadguy& self, BadGuy& /*other*/, CollisionHit const& hit)
{
  // behaviour for badguy collisions is the same as for collisions with solids
  self.collision_solid(hit);
  return CONTINUE;
}

void hopper_after_move(ArchetypeBadguy& self, float /*dt_sec*/)
{
  Hopper& hopper = ecs::get<Hopper>(self.get_entity());

  // no change if frozen
  if (self.m_frozen)
    return;

  // charge when fully recovered
  if ((hopper.state == Hopper::State::STANDING) && (hopper.recover_timer.check())) {
    hopper_set_state(self, hopper, Hopper::State::CHARGING);
    return;
  }

  // jump as soon as charging animation completed
  if ((hopper.state == Hopper::State::CHARGING) && (self.m_sprite->animation_done())) {
    hopper_set_state(self, hopper, Hopper::State::JUMPING);
    return;
  }
}

void hopper_unfreeze(ArchetypeBadguy& self, bool melt)
{
  self.default_unfreeze(melt);
  hopper_initialize(self);
}

// Bobber -------------------------------------------------------------

void bobber_construct(ArchetypeBadguy& self)
{
  self.m_physic.enable_gravity(false);
}

void bobber_initialize(ArchetypeBadguy& self)
{
  Bobber& bobber = ecs::get<Bobber>(self.get_entity());
  self.m_sprite->set_action(self.m_dir);
  bobber.going_up = true;
  self.m_physic.set_velocity_y(-bobber.speed);
  bobber.timer.start(bobber.fly_time / 2);
}

void bobber_collision_solid(ArchetypeBadguy& self, CollisionHit const& hit)
{
  if (hit.top || hit.bottom) { // hit floor or roof?
    self.m_physic.set_velocity_y(0);
  }
  if (self.m_frozen)
    self.default_collision_solid(hit);
}

void bobber_move(ArchetypeBadguy& self, float dt_sec)
{
  Bobber& bobber = ecs::get<Bobber>(self.get_entity());

  if (self.m_frozen)
  {
    self.default_move(dt_sec);
    return;
  }

  if (bobber.timer.check()) {
    bobber.going_up = !bobber.going_up;
    self.m_physic.set_velocity_y(bobber.going_up ? -bobber.speed : bobber.speed);
    bobber.timer.start(bobber.fly_time);
  }
  self.m_col.set_movement(self.m_physic.get_movement(dt_sec));

  if (auto player = self.get_nearest_player()) {
    self.m_dir = (player->get_pos().x > self.get_pos().x) ? Direction::RIGHT : Direction::LEFT;
    self.m_sprite->set_action(self.m_dir);
  }
}

void bobber_freeze(ArchetypeBadguy& self)
{
  self.m_physic.enable_gravity(true);
  self.default_freeze();
}

void bobber_unfreeze(ArchetypeBadguy& self, bool melt)
{
  self.default_unfreeze(melt);
  self.m_physic.enable_gravity(false);
  bobber_initialize(self);
}

// DartShooter --------------------------------------------------------

constexpr float MUZZLE_Y = 25; /**< [px] muzzle y-offset from top */

void shooter_construct(ArchetypeBadguy& self)
{
  DartShooter& shooter = ecs::get<DartShooter>(self.get_entity());
  self.m_countMe = false;
  SoundManager::current()->preload("sounds/dartfire.wav");
  if (self.m_start_dir == Direction::AUTO) { log_warning("Setting a DartTrap's direction to AUTO is no good idea"); }
  if (shooter.initial_delay == 0) shooter.initial_delay = 0.1f;
}

void shooter_initialize(ArchetypeBadguy& self)
{
  self.m_sprite->set_action("idle", self.m_dir);
}

void shooter_activate(ArchetypeBadguy& self)
{
  DartShooter& shooter = ecs::get<DartShooter>(self.get_entity());
  shooter.fire_timer.start(shooter.initial_delay);
}

HitResponse shooter_collision_player(ArchetypeBadguy& /*self*/, Player& /*player*/, CollisionHit const& /*hit*/)
{
  return ABORT_MOVE;
}

void shooter_fire(ArchetypeBadguy& self, DartShooter& shooter)
{
  float px = self.get_pos().x;
  if (self.m_dir == Direction::RIGHT) px += 5;
  float py = self.get_pos().y;
  if (self.m_flip == NO_FLIP)
    py += MUZZLE_Y;
  else
    py += (self.m_col.m_bbox.get_height() - MUZZLE_Y - 7.0f);

  SoundManager::current()->play("sounds/dartfire.wav", self.get_pos());
  auto dart = ArchetypeBadguy::create(shooter.dart, Vector(px, py), self.m_dir);
  if (auto* projectile = ecs::try_get<Projectile>(dart->get_entity())) {
    projectile->parent = self.get_entity();
  }
  Sector::get().add_object(std::move(dart));
  shooter.loading = false;
  self.m_sprite->set_action("idle", self.m_dir);
}

/** stationary: replaces the physics movement */
void shooter_move(ArchetypeBadguy& self, float /*dt_sec*/)
{
  DartShooter& shooter = ecs::get<DartShooter>(self.get_entity());
  if (!shooter.enabled) {
    return;
  }

  if (!shooter.loading) {
    if ((shooter.ammo != 0) && (shooter.fire_timer.check())) {
      if (shooter.ammo > 0) shooter.ammo--;
      shooter.loading = true;
      self.m_sprite->set_action("loading", self.m_dir, 1);
      shooter.fire_timer.start(shooter.fire_delay);
    }
  } else if (self.m_sprite->animation_done()) {
    shooter_fire(self, shooter);
  }
}

// Projectile ---------------------------------------------------------

void projectile_construct(ArchetypeBadguy& self)
{
  self.m_countMe = false;
  SoundManager::current()->preload("sounds/darthit.wav");
  SoundManager::current()->preload("sounds/stomp.wav");
}

void projectile_initialize(ArchetypeBadguy& self)
{
  Projectile const& projectile = ecs::get<Projectile>(self.get_entity());
  if (projectile.velocity) {
    self.m_physic.set_velocity(*projectile.velocity);
  } else {
    self.m_physic.set_velocity_x(self.m_dir == Direction::LEFT ? -projectile.speed : projectile.speed);
  }

  std::string const& action = projectile.actions.size() == 1 ? projectile.actions[0] :
    projectile.actions[graphicsRandom.rand(static_cast<int>(projectile.actions.size()))];
  if (projectile.directional) {
    self.m_sprite->set_action(action, self.m_dir);
  } else {
    self.m_sprite->set_action(action);
  }
}

void projectile_deactivate(ArchetypeBadguy& self)
{
  self.remove_me();
}

void projectile_collision_solid(ArchetypeBadguy& self, CollisionHit const& /*hit*/)
{
  SoundManager::current()->play("sounds/darthit.wav", self.get_pos());
  self.remove_me();
}

HitResponse projectile_collision_badguy(ArchetypeBadguy& self, BadGuy& badguy, CollisionHit const& /*hit*/)
{
  // ignore collisions with parent
  if (badguy.get_entity() == ecs::get<Projectile>(self.get_entity()).parent) {
    return FORCE_MOVE;
  }
  SoundManager::current()->play("sounds/stomp.wav", self.get_pos());
  self.remove_me();
  badguy.kill_fall();
  return ABORT_MOVE;
}

HitResponse projectile_collision_player(ArchetypeBadguy& self, Player& player, CollisionHit const& hit)
{
  SoundManager::current()->play("sounds/stomp.wav", self.get_pos());
  self.remove_me();
  return self.default_collision_player(player, hit);
}

// Toad ---------------------------------------------------------------

void toad_set_state(ArchetypeBadguy& self, Toad& toad, Toad::State state)
{
  if (state == Toad::State::IDLE) {
    self.m_physic.set_velocity_x(0);
    self.m_physic.set_velocity_y(0);
    if (!self.m_frozen)
      self.m_sprite->set_action("idle", self.m_dir);

    toad.recover_timer.start(toad.recover_time);
  } else if (state == Toad::State::JUMPING) {
    self.m_sprite->set_action("jumping", self.m_dir);
    self.m_physic.set_velocity_x(self.m_dir == Direction::LEFT ? -toad.jump_speed_x : toad.jump_speed_x);
    self.m_physic.set_velocity_y(toad.jump_speed_y);
    SoundManager::current()->play(toad.sound, self.get_pos());
  } else if (state == Toad::State::FALLING) {
    Player* player = self.get_nearest_player();
    Rectf const& bbox = self.m_col.m_bbox;
    // face player
    if (player && (player->get_bbox().get_right() < bbox.get_left()) && (self.m_dir == Direction::RIGHT)) self.m_dir = Direction::LEFT;
    if (player && (player->get_bbox().get_left() > bbox.get_right()) && (self.m_dir == Direction::LEFT)) self.m_dir = Direction::RIGHT;
    self.m_sprite->set_action("idle", self.m_dir);
  }

  toad.state = state;
}

void toad_construct(ArchetypeBadguy& self)
{
  SoundManager::current()->preload(ecs::get<Toad>(self.get_entity()).sound);
}

void toad_initialize(ArchetypeBadguy& self)
{
  // initial state is JUMPING, because we might start airborne
  ecs::get<Toad>(self.get_entity()).state = Toad::State::JUMPING;
  self.m_sprite->set_action("jumping", self.m_dir);
}

void toad_collision_solid(ArchetypeBadguy& self, CollisionHit const& hit)
{
  Toad& toad = ecs::get<Toad>(self.get_entity());

  // default behavior when frozen
  if (self.m_frozen || self.get_state() == ArchetypeBadguy::STATE_BURNING)
  {
    self.default_collision_solid(hit);
    return;
  }

  // just default behaviour (i.e. stop at floor/walls) when squished
  if (self.get_state() == ArchetypeBadguy::STATE_SQUISHED) {
    self.default_collision_solid(hit);
    return;
  }

  // ignore collisions while standing still
  if (toad.state == Toad::State::IDLE) {
    return;
  }

  // check if we hit left or right while moving in either direction
  if (((self.m_physic.get_velocity_x() < 0) && hit.left) || ((self.m_physic.get_velocity_x() > 0) && hit.right)) {
    self.m_physic.set_velocity_x(-0.25f*self.m_physic.get_velocity_x());
  }

  // check if we hit the floor while falling
  if ((toad.state == Toad::State::FALLING) && hit.bottom) {
    toad_set_state(self, toad, Toad::State::IDLE);
    return;
  }

  // check if we hit the roof while climbing
  if ((toad.state == Toad::State::JUMPING) && hit.top) {
    self.m_physic.set_velocity_y(0);
  }
}

HitResponse toad_collision_badguy(ArchetypeBadguy& self, BadGuy& /*other*/, CollisionHit const& hit)
{
  // behaviour for badguy collisions is the same as for collisions with solids
  self.collision_solid(hit);
  return CONTINUE;
}

void toad_after_move(ArchetypeBadguy& self, float /*dt_sec*/)
{
  Toad& toad = ecs::get<Toad>(self.get_entity());

  // change sprite when we are falling and not frozen
  if ((toad.state == Toad::State::JUMPING) && (self.m_physic.get_velocity_y() > 0) && !self.m_frozen) {
    toad_set_state(self, toad, Toad::State::FALLING);
    return;
  }

  // jump when fully recovered and if not frozen
  if ((toad.state == Toad::State::IDLE) && (toad.recover_timer.check() && !self.m_frozen)) {
    toad_set_state(self, toad, Toad::State::JUMPING);
    return;
  }
}

void toad_unfreeze(ArchetypeBadguy& self, bool melt)
{
  self.default_unfreeze(melt);
  toad_initialize(self);
}

// Mole ---------------------------------------------------------------

void mole_set_state(ArchetypeBadguy& self, Mole& mole, Mole::State state)
{
  switch (state) {
    case Mole::State::PRE_THROWING:
      self.m_sprite->set_action("idle");
      self.set_colgroup_active(COLGROUP_DISABLED);
      mole.timer.start(mole.wait_time);
      break;
    case Mole::State::THROWING:
      self.m_sprite->set_action("idle");
      self.set_colgroup_active(COLGROUP_DISABLED);
      mole.timer.start(mole.throw_time);
      mole.throw_timer.start(mole.throw_interval);
      break;
    case Mole::State::POST_THROWING:
      self.m_sprite->set_action("idle");
      self.set_colgroup_active(COLGROUP_DISABLED);
      mole.timer.start(mole.wait_time);
      break;
    case Mole::State::PEEKING:
      self.m_sprite->set_action("peeking", 1);
      self.set_colgroup_active(COLGROUP_STATIC);
      break;
    case Mole::State::DEAD:
      self.m_sprite->set_action("squished");
      self.set_colgroup_active(COLGROUP_DISABLED);
      break;
    case Mole::State::BURNING:
      self.m_sprite->set_action("burning", 1);
      self.set_colgroup_active(COLGROUP_DISABLED);
      break;
  }

  mole.state = state;
}

void mole_throw_rock(ArchetypeBadguy& self, Mole& mole)
{
  float base_angle = (self.m_flip == NO_FLIP ? 90.0f : 270.0f);
  float angle = math::radians(gameRandom.randf(base_angle - 15.0f, base_angle + 15.0f));
  float vx = cosf(angle) * mole.throw_velocity;
  float vy = -sinf(angle) * mole.throw_velocity;

  SoundManager::current()->play("sounds/dartfire.wav", self.get_pos());
  auto rock = ArchetypeBadguy::create(mole.rock, self.m_col.m_bbox.get_middle(), Direction::LEFT);
  if (auto* projectile = ecs::try_get<Projectile>(rock->get_entity())) {
    projectile->velocity = Vector(vx, vy);
    projectile->parent = self.get_entity();
  }
  Sector::get().add_object(std::move(rock));
}

void mole_construct(ArchetypeBadguy& self)
{
  self.m_physic.enable_gravity(false);
  SoundManager::current()->preload("sounds/fall.wav");
  SoundManager::current()->preload("sounds/squish.wav");
  SoundManager::current()->preload("sounds/dartfire.wav");
}

void mole_activate(ArchetypeBadguy& self)
{
  Mole& mole = ecs::get<Mole>(self.get_entity());
  if (mole.state != Mole::State::DEAD) mole_set_state(self, mole, Mole::State::PRE_THROWING);
}

void mole_kill_fall(ArchetypeBadguy& self)
{
  mole_set_state(self, ecs::get<Mole>(self.get_entity()), Mole::State::DEAD);
  SoundManager::current()->play("sounds/fall.wav", self.get_pos());
  self.run_dead_script();
}

HitResponse mole_collision_badguy(ArchetypeBadguy& /*self*/, BadGuy& /*other*/, CollisionHit const& /*hit*/)
{
  return FORCE_MOVE;
}

bool mole_collision_squished(ArchetypeBadguy& self, GameObject& /*object*/)
{
  mole_set_state(self, ecs::get<Mole>(self.get_entity()), Mole::State::DEAD);
  SoundManager::current()->play("sounds/squish.wav", self.get_pos());
  self.run_dead_script();
  return true;
}

void mole_after_move(ArchetypeBadguy& self, float /*dt_sec*/)
{
  Mole& mole = ecs::get<Mole>(self.get_entity());

  switch (mole.state) {
    case Mole::State::PRE_THROWING:
      if (mole.timer.check()) {
        mole_set_state(self, mole, Mole::State::THROWING);
      }
      break;
    case Mole::State::THROWING:
      if (mole.throw_timer.check()) {
        mole_throw_rock(self, mole);
        mole.throw_timer.start(mole.throw_interval);
      }
      if (mole.timer.check()) {
        mole_set_state(self, mole, Mole::State::POST_THROWING);
      }
      break;
    case Mole::State::POST_THROWING:
      if (mole.timer.check()) {
        mole_set_state(self, mole, Mole::State::PEEKING);
      }
      break;
    case Mole::State::PEEKING:
      if (self.m_sprite->animation_done()) {
        mole_set_state(self, mole, Mole::State::PRE_THROWING);
      }
      break;
    case Mole::State::BURNING:
      if (self.m_sprite->animation_done()) {
        mole_set_state(self, mole, Mole::State::DEAD);
      }
      break;
    case Mole::State::DEAD:
      break;
  }
}

void mole_ignite(ArchetypeBadguy& self)
{
  mole_set_state(self, ecs::get<Mole>(self.get_entity()), Mole::State::BURNING);
  self.run_dead_script();
  SoundManager::current()->play("sounds/fire.ogg", self.get_pos());
}

// Diver --------------------------------------------------------------

void diver_construct(ArchetypeBadguy& self)
{
  Diver& diver = ecs::get<Diver>(self.get_entity());
  diver.speed = gameRandom.randf(diver.min_speed, diver.max_speed);
  self.m_physic.enable_gravity(false);
}

void diver_initialize(ArchetypeBadguy& self)
{
  float const speed = ecs::get<Diver>(self.get_entity()).speed;
  self.m_physic.set_velocity_x(self.m_dir == Direction::LEFT ? -speed : speed);
  self.m_sprite->set_action(self.m_dir);
}

void diver_bump_horizontal(ArchetypeBadguy& self, Diver& diver)
{
  self.m_dir = (self.m_dir == Direction::LEFT ? Direction::RIGHT : Direction::LEFT);
  self.m_sprite->set_action(self.m_dir);
  self.m_physic.set_velocity_x(self.m_dir == Direction::LEFT ? -diver.speed : diver.speed);
  if (diver.state == Diver::State::DIVING) {
    diver.state = Diver::State::FLYING;
    self.m_physic.set_velocity_y(0);
  }
}

void diver_bump_vertical(ArchetypeBadguy& self, Diver& diver)
{
  if (self.get_state() == ArchetypeBadguy::STATE_BURNING)
  {
    self.m_physic.set_velocity_y(0);
    self.m_physic.set_velocity_x(0);
    return;
  }

  if (diver.state == Diver::State::FLYING) {
    self.m_physic.set_velocity_y(0);
  } else if (diver.state == Diver::State::DIVING) {
    diver.state = Diver::State::CLIMBING;
    self.m_physic.set_velocity_y(-diver.speed);
    self.m_sprite->set_action(self.m_dir);
  } else if (diver.state == Diver::State::CLIMBING) {
    diver.state = Diver::State::FLYING;
    self.m_physic.set_velocity_y(0);
  }
}

void diver_collision_solid(ArchetypeBadguy& self, CollisionHit const& hit)
{
  Diver& diver = ecs::get<Diver>(self.get_entity());

  if (self.m_frozen)
    self.default_collision_solid(hit);
  else
  {
    if (self.m_sprite->get_action() == "squished-left" ||
        self.m_sprite->get_action() == "squished-right")
    {
      return;
    }

    if (hit.top || hit.bottom) {
      diver_bump_vertical(self, diver);
    }
    else if (hit.left || hit.right) {
      diver_bump_horizontal(self, diver);
    }
  }
}

/** linear prediction of player and badguy positions to decide if we should enter the DIVING state */
bool diver_should_dive(ArchetypeBadguy& self, Diver& diver)
{
  if (self.m_frozen)
    return false;

  auto const player = self.get_nearest_player();
  if (player && diver.last_player && (player == diver.last_player)) {

    // get positions, calculate movement
    Vector const& player_pos = player->get_pos();
    Vector const player_mov = (player_pos - diver.last_player_pos);
    Vector const self_pos = self.m_col.m_bbox.p1();
    Vector const self_mov = (self_pos - diver.last_self_pos);

    // new vertical speed to test with
    float vy = 2*fabsf(self_mov.x);

    // do not dive if we are not above the player
    float height = player_pos.y - self_pos.y;
    if (height <= 0) return false;

    // do not dive if we are too far above the player
    if (height > 512) return false;

    // do not dive if we would not descend faster than the player
    float relSpeed = vy - player_mov.y;
    if (relSpeed <= 0) return false;

    // guess number of frames to descend to same height as player
    float estFrames = height / relSpeed;

    // guess where the player would be at this time
    float estPx = (player_pos.x + (estFrames * player_mov.x));

    // guess where we would be at this time
    float estBx = (self_pos.x + (estFrames * self_mov.x));

    // near misses are OK, too
    if (fabsf(estPx - estBx) < 8) return true;
  }

  // update last player tracked, as well as our positions
  diver.last_player = player;
  if (player) {
    diver.last_player_pos = player->get_pos();
    diver.last_self_pos = self.m_col.m_bbox.p1();
  }

  return false;
}

bool diver_update(ArchetypeBadguy& self, float /*dt_sec*/)
{
  Diver& diver = ecs::get<Diver>(self.get_entity());

  if (diver.state == Diver::State::FLYING) {
    if (diver_should_dive(self, diver)) {
      diver.state = Diver::State::DIVING;
      self.m_physic.set_velocity_y(2*fabsf(self.m_physic.get_velocity_x()));
      self.m_sprite->set_action("diving", self.m_dir);
    }
  } else if (diver.state == Diver::State::CLIMBING) {
    // stop climbing when we're back at initial height
    if (self.get_pos().y <= self.m_start_position.y) {
      diver.state = Diver::State::FLYING;
      self.m_physic.set_velocity_y(0);
    }
  }
  return true;
}

void diver_freeze(ArchetypeBadguy& self)
{
  self.default_freeze();
  self.m_physic.enable_gravity(true);
}

void diver_unfreeze(ArchetypeBadguy& self, bool melt)
{
  self.default_unfreeze(melt);
  self.m_physic.enable_gravity(false);
  ecs::get<Diver>(self.get_entity()).state = Diver::State::FLYING;
  diver_initialize(self);
}

// Haywire ------------------------------------------------------------

std::shared_ptr<SoundSource> haywire_sound(ArchetypeBadguy& self, std::string const& name)
{
  std::shared_ptr<SoundSource> source = SoundManager::current()->create_sound_source(name);
  source->set_position(self.get_pos());
  source->set_looping(true);
  source->set_reference_distance(32);
  source->play();
  return source;
}

void haywire_start_exploding(ArchetypeBadguy& self, Haywire& haywire)
{
  Walker& walker = ecs::get<Walker>(self.get_entity());
  walker.speed = fabsf(haywire.exploding_speed);
  walker.max_drop_height = -1;
  haywire.time_until_explosion = haywire.explosion_time;
  haywire.exploding = true;

  haywire.ticking = haywire_sound(self, "sounds/fizz.wav");
  haywire.grunting = haywire_sound(self, "sounds/grunts.ogg");
}

void haywire_stop_exploding(ArchetypeBadguy& self, Haywire& haywire)
{
  Walker& walker = ecs::get<Walker>(self.get_entity());
  walker.left_action = "left";
  walker.right_action = "right";
  walker.speed = fabsf(haywire.normal_speed);
  walker.max_drop_height = haywire.normal_max_drop_height;
  haywire.time_until_explosion = 0.0f;
  haywire.exploding = false;

  if (haywire.ticking)
    haywire.ticking->stop();
  if (haywire.grunting)
    haywire.grunting->stop();
}

void haywire_construct(ArchetypeBadguy& /*self*/)
{
  //Prevent stutter when Tux jumps on it
  SoundManager::current()->preload("sounds/explosion.wav");
}

bool haywire_collision_squished(ArchetypeBadguy& self, GameObject& object)
{
  Haywire& haywire = ecs::get<Haywire>(self.get_entity());

  if (self.m_frozen)
    return self.default_collision_squished(object);

  auto player = dynamic_cast<Player*>(&object);
  if (player && player->is_invincible()) {
    player->bounce(self);
    self.kill_fall();
    return true;
  }

  if (haywire.stunned) {
    if (player)
      player->bounce(self);
    return true;
  }

  if (self.is_frozen()) {
    self.unfreeze();
  }

  if (!haywire.exploding) {
    haywire_start_exploding(self, haywire);
    haywire.stomped_timer.start(haywire.stomped_time);
  }

  haywire.time_stunned = haywire.stunned_time;
  haywire.stunned = true;
  self.m_physic.set_velocity_x(0.f);
  self.m_physic.set_acceleration_x(0.f);

  if (player)
    player->bounce(self);

  return true;
}

bool haywire_update(ArchetypeBadguy& self, float dt_sec)
{
  Haywire& haywire = ecs::get<Haywire>(self.get_entity());
  Walker& walker = ecs::get<Walker>(self.get_entity());

  auto* player = self.get_nearest_player();
  if (haywire.exploding) {
    haywire.ticking->set_position(self.get_pos());
    haywire.grunting->set_position(self.get_pos());
    if (dt_sec >= haywire.time_until_explosion) {
      self.kill_fall();
      return false;
    }
    else
      haywire.time_until_explosion -= dt_sec;
  }

  if (haywire.stunned) {
    if (haywire.time_stunned > dt_sec) {
      haywire.time_stunned -= dt_sec;
    }
    else { /* if (time_stunned <= dt_sec) */
      haywire.time_stunned = 0.f;
      haywire.stunned = false;
    }
  }

  if (!haywire.exploding) {
    // move and walk normally
    return true;
  }

  Rectf const& bbox = self.m_col.m_bbox;
  if (self.on_ground() && std::abs(self.m_physic.get_velocity_x()) > 40.f && player)
  {
    //jump over 1-tall roadblocks
    Rectf jump_box = self.get_bbox();
    jump_box.set_left(bbox.get_left() + (self.m_dir == Direction::LEFT ? -48.f : 38.f));
    jump_box.set_right(bbox.get_right() + (self.m_dir == Direction::RIGHT ? 48.f : -38.f));

    Rectf exception_box = self.get_bbox();
    exception_box.set_left(bbox.get_left() + (self.m_dir == Direction::LEFT ? -48.f : 38.f));
    exception_box.set_right(bbox.get_right() + (self.m_dir == Direction::RIGHT ? 48.f : -38.f));
    exception_box.set_top(bbox.get_top() - 32.f);
    exception_box.set_bottom(bbox.get_bottom() - 48.f);

    if (!Sector::get().is_free_of_statics(jump_box) && Sector::get().is_free_of_statics(exception_box))
    {
      self.m_physic.set_velocity_y(-325.f);
    }
    else
    {
      //jump over gaps if Tux isnt below
      Rectf gap_box = self.get_bbox();
      gap_box.set_left(bbox.get_left() + (self.m_dir == Direction::LEFT ? -38.f : 26.f));
      gap_box.set_right(bbox.get_right() + (self.m_dir == Direction::LEFT ? -26.f : 38.f));
      gap_box.set_top(bbox.get_top());
      gap_box.set_bottom(bbox.get_bottom() + 28.f);

      if (Sector::get().is_free_of_statics(gap_box)
          && (player->get_bbox().get_bottom() <= bbox.get_bottom()))
      {
        self.m_physic.set_velocity_y(-325.f);
      }
    }
  }

  if (haywire.stomped_timer.get_timeleft() < 0.05f) {
    self.set_action((self.m_dir == Direction::LEFT) ? "ticking-left" : "ticking-right", /* loops = */ -1);
    walker.left_action = "ticking-left";
    walker.right_action = "ticking-right";
  }
  else {
    self.set_action((self.m_dir == Direction::LEFT) ? "active-left" : "active-right", /* loops = */ 1);
    walker.left_action = "active-left";
    walker.right_action = "active-right";
  }

  float target_velocity = 0.f;
  if (!self.m_frozen)
  {
    if (haywire.stomped_timer.get_timeleft() >= 0.05f)
    {
      target_velocity = 0.f;
    }
    else if (player && haywire.time_stunned == 0.0f)
    {
      /* Player is on the right or left*/
      target_velocity = (player->get_pos().x > self.get_pos().x) ? walker.speed : (-1.f) * walker.speed;
    }
  }

  // move, then walk towards the target
  walker.target_velocity = target_velocity;
  walker.acceleration = 3.f;
  return true;
}

void haywire_stop_sounds(ArchetypeBadguy& self)
{
  Haywire& haywire = ecs::get<Haywire>(self.get_entity());
  if (haywire.ticking) {
    haywire.ticking->stop();
  }
  if (haywire.grunting) {
    haywire.grunting->stop();
  }
}

void haywire_play_sounds(ArchetypeBadguy& self)
{
  Haywire& haywire = ecs::get<Haywire>(self.get_entity());
  if (haywire.exploding) {
    if (haywire.ticking) {
      haywire.ticking->play();
    }
    if (haywire.grunting) {
      haywire.grunting->play();
    }
  }
}

void haywire_kill_fall(ArchetypeBadguy& self)
{
  Haywire& haywire = ecs::get<Haywire>(self.get_entity());
  if (haywire.exploding) {
    haywire.ticking->stop();
    haywire.grunting->stop();
  }
  if (self.is_valid()) {
    if (self.m_frozen)
      self.default_kill_fall();
    else
    {
      self.remove_me();
      explode_at(self);
      self.run_dead_script();
    }
  }
}

void haywire_ignite(ArchetypeBadguy& self)
{
  if (self.m_frozen)
    self.unfreeze();
  self.kill_fall();
}

void haywire_freeze(ArchetypeBadguy& self)
{
  Haywire& haywire = ecs::get<Haywire>(self.get_entity());
  self.default_freeze();
  if (haywire.exploding) {
    haywire_stop_exploding(self, haywire);
  }
}

HitResponse haywire_collision_badguy(ArchetypeBadguy& self, BadGuy& badguy, CollisionHit const& hit)
{
  if (ecs::get<Haywire>(self.get_entity()).exploding)
  {
    badguy.kill_fall();
    return FORCE_MOVE;
  }

  if (self.m_frozen)
    return FORCE_MOVE;
  else
  {
    walker::collision_badguy(self, ecs::get<Walker>(self.get_entity()), badguy, hit);
  }

  return ABORT_MOVE;
}

// GoldBomb -----------------------------------------------------------

void goldbomb_construct(ArchetypeBadguy& /*self*/)
{
  //Prevent stutter when Tux jumps on Gold Bomb
  SoundManager::current()->preload("sounds/explosion.wav");
}

void goldbomb_collision_solid(ArchetypeBadguy& self, CollisionHit const& hit)
{
  if (ecs::get<GoldBomb>(self.get_entity()).ticking) {
    if (hit.bottom) {
      self.m_physic.set_velocity_y(0);
      self.m_physic.set_velocity_x(0);
    } else if (hit.left || hit.right)
      self.m_physic.set_velocity_x(-self.m_physic.get_velocity_x());
    else if (hit.top)
      self.m_physic.set_velocity_y(0);
    self.update_on_ground_flag(hit);
    return;
  }
  walker::collision_solid(self, ecs::get<Walker>(self.get_entity()), hit);
}

HitResponse goldbomb_collision(ArchetypeBadguy& self, GameObject& object, CollisionHit const& hit)
{
  if (ecs::get<GoldBomb>(self.get_entity()).ticking) {
    if (dynamic_cast<Player*>(&object)) {
      return ABORT_MOVE;
    }
    if (dynamic_cast<BadGuy*>(&object)) {
      return ABORT_MOVE;
    }
  }
  if (self.is_grabbed())
    return FORCE_MOVE;

  return self.default_collision(object, hit);
}

HitResponse goldbomb_collision_player(ArchetypeBadguy& self, Player& player, CollisionHit const& hit)
{
  if (ecs::get<GoldBomb>(self.get_entity()).ticking)
    return FORCE_MOVE;
  if (self.is_grabbed())
    return FORCE_MOVE;
  return self.default_collision_player(player, hit);
}

HitResponse goldbomb_collision_badguy(ArchetypeBadguy& self, BadGuy& badguy, CollisionHit const& hit)
{
  if (ecs::get<GoldBomb>(self.get_entity()).ticking)
    return FORCE_MOVE;
  return walker::collision_badguy(self, ecs::get<Walker>(self.get_entity()), badguy, hit);
}

bool goldbomb_collision_squished(ArchetypeBadguy& self, GameObject& object)
{
  GoldBomb& goldbomb = ecs::get<GoldBomb>(self.get_entity());

  if (self.m_frozen)
    return self.default_collision_squished(object);

  Player* player = dynamic_cast<Player*>(&object);
  if (player && player->is_invincible()) {
    player->bounce(self);
    self.kill_fall();
    return true;
  }

  if (self.is_valid() && !goldbomb.ticking) {
    goldbomb.ticking = true;
    self.m_frozen = false;
    self.set_action(self.m_dir == Direction::LEFT ? "ticking-left" : "ticking-right", 1);
    self.m_physic.set_velocity_x(0);

    if (player)
      player->bounce(self);

    SoundManager::current()->play("sounds/squish.wav", self.get_pos());
    goldbomb.ticking_sound = SoundManager::current()->create_sound_source("sounds/fizz.wav");
    goldbomb.ticking_sound->set_position(self.get_pos());
    goldbomb.ticking_sound->set_looping(true);
    goldbomb.ticking_sound->set_gain(1.0f);
    goldbomb.ticking_sound->set_reference_distance(32);
    goldbomb.ticking_sound->play();
  }

  return true;
}

bool goldbomb_update(ArchetypeBadguy& self, float dt_sec)
{
  GoldBomb& goldbomb = ecs::get<GoldBomb>(self.get_entity());

  if (goldbomb.ticking) {
    if (self.on_ground()) self.m_physic.set_velocity_x(0);
    goldbomb.ticking_sound->set_position(self.get_pos());
    if (self.m_sprite->animation_done()) {
      self.kill_fall();
    }
    else if (!self.is_grabbed()) {
      self.m_col.set_movement(self.m_physic.get_movement(dt_sec));
    }
    return false;
  }

  // walk unless carried
  return !self.is_grabbed();
}

void goldbomb_kill_fall(ArchetypeBadguy& self)
{
  GoldBomb& goldbomb = ecs::get<GoldBomb>(self.get_entity());

  if (goldbomb.ticking)
    goldbomb.ticking_sound->stop();

  // Make the player let go before we explode, otherwise the player is holding
  // an invalid object.
  if (self.is_grabbed()) {
    Player* player = dynamic_cast<Player*>(self.get_owner());
    if (player)
      player->stop_grabbing();
  }

  if (self.is_valid()) {
    if (self.m_frozen)
      self.default_kill_fall();
    else
    {
      self.remove_me();
      explode_at(self);
      self.run_dead_script();
    }
    Sector::get().add<CoinExplode>(self.get_pos() + Vector(0, -40));
  }
}

void goldbomb_ignite(ArchetypeBadguy& self)
{
  if (self.m_frozen)
    self.unfreeze();
  self.kill_fall();
}

void goldbomb_grab(ArchetypeBadguy& self, MovingObject& object, Vector const& pos, Direction dir)
{
  self.Portable::grab(object, pos, dir);
  if (ecs::get<GoldBomb>(self.get_entity()).ticking) {
    // We actually face the opposite direction of Tux here to make the fuse more
    // visible instead of hiding it behind Tux
    self.m_sprite->set_action_continued(self.m_dir == Direction::LEFT ? "ticking-right" : "ticking-left");
    self.set_colgroup_active(COLGROUP_DISABLED);
  }
  else if (self.m_frozen) {
    self.m_sprite->set_action("iced", dir);
  }
  else if (ecs::try_get<Owl>(object.get_entity()))
    self.m_sprite->set_action(dir);
  self.m_col.set_movement(pos - self.get_pos());
  self.m_dir = dir;
  self.set_colgroup_active(COLGROUP_DISABLED);
}

void goldbomb_ungrab(ArchetypeBadguy& self, MovingObject& object, Direction dir)
{
  if (self.m_frozen)
    self.BadGuy::ungrab(object, dir);
  else
    throw_by(self, dynamic_cast<Player*>(&object), dir);

  self.set_colgroup_active(self.m_frozen ? COLGROUP_MOVING_STATIC : COLGROUP_MOVING);
  self.Portable::ungrab(object, dir);
}

void goldbomb_freeze(ArchetypeBadguy& self)
{
  if (!ecs::get<GoldBomb>(self.get_entity()).ticking) {
    self.default_freeze();
  }
}

bool goldbomb_is_portable(ArchetypeBadguy const& self)
{
  return (self.m_frozen || ecs::get<GoldBomb>(self.get_entity()).ticking);
}

void goldbomb_stop_sound(ArchetypeBadguy& self)
{
  if (auto& sound = ecs::get<GoldBomb>(self.get_entity()).ticking_sound) {
    sound->stop();
  }
}

void goldbomb_play_sound(ArchetypeBadguy& self)
{
  GoldBomb& goldbomb = ecs::get<GoldBomb>(self.get_entity());
  if (goldbomb.ticking && goldbomb.ticking_sound) {
    goldbomb.ticking_sound->play();
  }
}

// LiveFire -----------------------------------------------------------

void livefire_construct(ArchetypeBadguy& self)
{
  LiveFire& livefire = ecs::get<LiveFire>(self.get_entity());
  if (livefire.variant == "sleeping") {
    livefire.state = LiveFire::State::SLEEPING;
  } else if (livefire.variant == "dormant") {
    livefire.state = LiveFire::State::DORMANT;
  } else {
    livefire.state = LiveFire::State::WALKING;
  }
}

void livefire_initialize(ArchetypeBadguy& self)
{
  if (ecs::get<LiveFire>(self.get_entity()).variant == "walking") {
    walker::initialize(self, ecs::get<Walker>(self.get_entity()));
  } else {
    self.m_physic.set_velocity_x(0);
    self.m_sprite->set_action("sleeping", self.m_dir);
  }
}

void livefire_collision_solid(ArchetypeBadguy& self, CollisionHit const& hit)
{
  if (ecs::get<LiveFire>(self.get_entity()).state != LiveFire::State::WALKING) {
    self.default_collision_solid(hit);
    return;
  }
  walker::collision_solid(self, ecs::get<Walker>(self.get_entity()), hit);
}

HitResponse livefire_collision_badguy(ArchetypeBadguy& self, BadGuy& badguy, CollisionHit const& hit)
{
  if (ecs::get<LiveFire>(self.get_entity()).state != LiveFire::State::WALKING) {
    return self.default_collision_badguy(badguy, hit);
  }
  return walker::collision_badguy(self, ecs::get<Walker>(self.get_entity()), badguy, hit);
}

bool livefire_update(ArchetypeBadguy& self, float dt_sec)
{
  LiveFire& livefire = ecs::get<LiveFire>(self.get_entity());

  // Remove when extinguish animation is done
  if ((self.m_sprite->get_action() == "extinguish-left" || self.m_sprite->get_action() == "extinguish-right")
      && self.m_sprite->animation_done()) self.remove_me();

  if (livefire.state == LiveFire::State::WALKING) {
    return true;
  }

  if (livefire.state == LiveFire::State::SLEEPING && self.m_col.get_group() == COLGROUP_MOVING) {
    if (auto player = self.get_nearest_player()) {
      Rectf const& bbox = self.m_col.m_bbox;
      Rectf pb = player->get_bbox();

      bool inReach_left = (pb.get_right() >= bbox.get_right() - ((self.m_dir == Direction::LEFT) ? 256 : 0));
      bool inReach_right = (pb.get_left() <= bbox.get_left() + ((self.m_dir == Direction::RIGHT) ? 256 : 0));
      bool inReach_top = (pb.get_bottom() >= bbox.get_top());
      bool inReach_bottom = (pb.get_top() <= bbox.get_bottom());

      if (inReach_left && inReach_right && inReach_top && inReach_bottom) {
        // wake up
        self.m_sprite->set_action("waking", self.m_dir, 1);
        livefire.state = LiveFire::State::WAKING;
      }
    }
  }
  else if (livefire.state == LiveFire::State::WAKING) {
    if (self.m_sprite->animation_done()) {
      // start walking
      livefire.state = LiveFire::State::WALKING;
      walker::initialize(self, ecs::get<Walker>(self.get_entity()));
    }
  }

  self.default_move(dt_sec);
  return false;
}

void livefire_kill_fall(ArchetypeBadguy& self)
{
  LiveFire& livefire = ecs::get<LiveFire>(self.get_entity());

  SoundManager::current()->play(livefire.death_sound, self.get_pos());
  // throw a puff of smoke
  Vector ppos = self.m_col.m_bbox.get_middle();
  Vector pspeed = Vector(0, -150);
  Vector paccel = Vector(0,0);
  Sector::get().add<SpriteParticle>("images/particles/smoke.sprite",
                                    "default", ppos, ANCHOR_MIDDLE,
                                    pspeed, paccel,
                                    LAYER_BACKGROUNDTILES+2);
  // extinguish the flame
  self.m_sprite->set_action("extinguish", self.m_dir, 1);
  self.m_physic.set_velocity_y(0);
  self.m_physic.set_acceleration_y(0);
  self.m_physic.enable_gravity(false);
  self.m_lightsprite->set_blend(Blend::ADD);
  self.m_lightsprite->set_color(Color(1.0f, 0.9f, 0.8f));
  self.set_group(COLGROUP_DISABLED);
  livefire.state = LiveFire::State::DEAD;

  // start dead-script
  self.run_dead_script();
}

void livefire_freeze(ArchetypeBadguy& self)
{
  // attempting to freeze a flame causes it to go out
  ecs::get<LiveFire>(self.get_entity()).death_sound = "sounds/sizzle.ogg";
  self.kill_fall();
}

} // namespace

template<>
BadGuyBehavior const& behavior_of<LiveFire>()
{
  static BadGuyBehavior const behavior = {
    .construct = &livefire_construct,
    .initialize = &livefire_initialize,
    .update = &livefire_update,
    .collision_solid = &livefire_collision_solid,
    .collision_badguy = &livefire_collision_badguy,
    .freeze = &livefire_freeze,
    .kill_fall = &livefire_kill_fall,
  };
  return behavior;
}

template<>
BadGuyBehavior const& behavior_of<GoldBomb>()
{
  static BadGuyBehavior const behavior = {
    .construct = &goldbomb_construct,
    .update = &goldbomb_update,
    .collision = &goldbomb_collision,
    .collision_player = &goldbomb_collision_player,
    .collision_solid = &goldbomb_collision_solid,
    .collision_badguy = &goldbomb_collision_badguy,
    .collision_squished = &goldbomb_collision_squished,
    .freeze = &goldbomb_freeze,
    .ignite = &goldbomb_ignite,
    .kill_fall = &goldbomb_kill_fall,
    .is_portable = &goldbomb_is_portable,
    .grab = &goldbomb_grab,
    .ungrab = &goldbomb_ungrab,
    .stop_looping_sounds = &goldbomb_stop_sound,
    .play_looping_sounds = &goldbomb_play_sound,
  };
  return behavior;
}

template<>
BadGuyBehavior const& behavior_of<Haywire>()
{
  static BadGuyBehavior const behavior = {
    .construct = &haywire_construct,
    .deactivate = &haywire_stop_sounds,
    .update = &haywire_update,
    .collision_badguy = &haywire_collision_badguy,
    .collision_squished = &haywire_collision_squished,
    .freeze = &haywire_freeze,
    .ignite = &haywire_ignite,
    .kill_fall = &haywire_kill_fall,
    .stop_looping_sounds = &haywire_stop_sounds,
    .play_looping_sounds = &haywire_play_sounds,
  };
  return behavior;
}

template<>
BadGuyBehavior const& behavior_of<DartShooter>()
{
  static BadGuyBehavior const behavior = {
    .construct = &shooter_construct,
    .initialize = &shooter_initialize,
    .activate = &shooter_activate,
    .move = &shooter_move,
    .collision_player = &shooter_collision_player,
  };
  return behavior;
}

template<>
BadGuyBehavior const& behavior_of<Projectile>()
{
  static BadGuyBehavior const behavior = {
    .construct = &projectile_construct,
    .initialize = &projectile_initialize,
    .deactivate = &projectile_deactivate,
    .collision_player = &projectile_collision_player,
    .collision_solid = &projectile_collision_solid,
    .collision_badguy = &projectile_collision_badguy,
  };
  return behavior;
}

template<>
BadGuyBehavior const& behavior_of<Toad>()
{
  static BadGuyBehavior const behavior = {
    .construct = &toad_construct,
    .initialize = &toad_initialize,
    .after_move = &toad_after_move,
    .collision_solid = &toad_collision_solid,
    .collision_badguy = &toad_collision_badguy,
    .unfreeze = &toad_unfreeze,
  };
  return behavior;
}

template<>
BadGuyBehavior const& behavior_of<Mole>()
{
  static BadGuyBehavior const behavior = {
    .construct = &mole_construct,
    .activate = &mole_activate,
    .after_move = &mole_after_move,
    .collision_badguy = &mole_collision_badguy,
    .collision_squished = &mole_collision_squished,
    .ignite = &mole_ignite,
    .kill_fall = &mole_kill_fall,
  };
  return behavior;
}

template<>
BadGuyBehavior const& behavior_of<Diver>()
{
  static BadGuyBehavior const behavior = {
    .construct = &diver_construct,
    .initialize = &diver_initialize,
    .update = &diver_update,
    .collision_solid = &diver_collision_solid,
    .freeze = &diver_freeze,
    .unfreeze = &diver_unfreeze,
  };
  return behavior;
}

template<>
BadGuyBehavior const& behavior_of<JumpingFish>()
{
  static BadGuyBehavior const behavior = {
    .construct = &fish_construct,
    .after_move = &fish_after_move,
    .collision_solid = &fish_collision_solid,
    .collision_badguy = &fish_collision_badguy,
    .collision_tile = &fish_collision_tile,
    .freeze = &fish_freeze,
    .unfreeze = &fish_unfreeze,
    .kill_fall = &fish_kill_fall,
  };
  return behavior;
}

template<>
BadGuyBehavior const& behavior_of<Hopper>()
{
  static BadGuyBehavior const behavior = {
    .construct = &hopper_construct,
    .initialize = &hopper_initialize,
    .after_move = &hopper_after_move,
    .collision_solid = &hopper_collision_solid,
    .collision_badguy = &hopper_collision_badguy,
    .unfreeze = &hopper_unfreeze,
  };
  return behavior;
}

template<>
BadGuyBehavior const& behavior_of<Bobber>()
{
  static BadGuyBehavior const behavior = {
    .construct = &bobber_construct,
    .initialize = &bobber_initialize,
    .move = &bobber_move,
    .collision_solid = &bobber_collision_solid,
    .freeze = &bobber_freeze,
    .unfreeze = &bobber_unfreeze,
  };
  return behavior;
}

template<>
BadGuyBehavior const& behavior_of<Snowman>()
{
  static BadGuyBehavior const behavior = {
    .construct = &snowman_construct,
    .collision_squished = &snowman_collision_squished,
    .collision_bullet = &snowman_collision_bullet,
  };
  return behavior;
}

template<>
BadGuyBehavior const& behavior_of<MrTree>()
{
  static BadGuyBehavior const behavior = {
    .construct = &mrtree_construct,
    .collision_squished = &mrtree_collision_squished,
  };
  return behavior;
}

template<>
BadGuyBehavior const& behavior_of<Stumpy>()
{
  static BadGuyBehavior const behavior = {
    .construct = &stumpy_construct,
    .initialize = &stumpy_initialize,
    .update = &stumpy_update,
    .collision_solid = &stumpy_collision_solid,
    .collision_badguy = &stumpy_collision_badguy,
    .collision_squished = &stumpy_collision_squished,
  };
  return behavior;
}

template<>
BadGuyBehavior const& behavior_of<Firecracker>()
{
  static BadGuyBehavior const behavior = {
    .construct = &firecracker_construct,
    .collision_player = &firecracker_collision_player,
    .collision_squished = &firecracker_collision_squished,
    .freeze = &firecracker_freeze,
    .ignite = &firecracker_ignite,
    .kill_fall = &firecracker_explode,
  };
  return behavior;
}

template<>
BadGuyBehavior const& behavior_of<Snail>()
{
  static BadGuyBehavior const behavior = {
    .construct = &snail_construct,
    .initialize = &snail_initialize,
    .update = &snail_update,
    .collision_player = &snail_collision_player,
    .collision_solid = &snail_collision_solid,
    .collision_badguy = &snail_collision_badguy,
    .collision_squished = &snail_collision_squished,
    .is_portable = &snail_is_portable,
    .can_break = &snail_can_break,
    .grab = &snail_grab,
    .ungrab = &snail_ungrab,
  };
  return behavior;
}

template<>
BadGuyBehavior const& behavior_of<Boarder>()
{
  static BadGuyBehavior const behavior = {
    .construct = &boarder_construct,
    .update = &boarder_update,
    .collision_solid = &boarder_collision_solid,
  };
  return behavior;
}

template<>
BadGuyBehavior const& behavior_of<Sleeper>()
{
  static BadGuyBehavior const behavior = {
    .initialize = &sleeper_initialize,
    .update = &sleeper_update,
    .collision_solid = &sleeper_collision_solid,
    .collision_badguy = &sleeper_collision_badguy,
    .freeze = &sleeper_freeze,
    .is_flammable = &sleeper_is_flammable,
  };
  return behavior;
}

template<>
BadGuyBehavior const& behavior_of<BulletShy>()
{
  static BadGuyBehavior const behavior = {
    .update = &shy_update,
    .collision_bullet = &shy_collision_bullet,
  };
  return behavior;
}

template<>
BadGuyBehavior const& behavior_of<IceBlock>()
{
  static BadGuyBehavior const behavior = {
    .construct = &iceblock_construct,
    .initialize = &iceblock_initialize,
    .update = &iceblock_update,
    .collision = &iceblock_collision,
    .collision_player = &iceblock_collision_player,
    .collision_solid = &iceblock_collision_solid,
    .collision_badguy = &iceblock_collision_badguy,
    .collision_squished = &iceblock_collision_squished,
    .ignite = &iceblock_ignite,
    .is_portable = &iceblock_is_portable,
    .can_break = &iceblock_can_break,
    .grab = &iceblock_grab,
    .ungrab = &iceblock_ungrab,
  };
  return behavior;
}

template<>
BadGuyBehavior const& behavior_of<Stalactite>()
{
  static BadGuyBehavior const behavior = {
    .construct = &stalactite_construct,
    .deactivate = &stalactite_deactivate,
    .move = &stalactite_move,
    .collision_player = &stalactite_collision_player,
    .collision_solid = &stalactite_collision_solid,
    .collision_badguy = &stalactite_collision_badguy,
    .collision_bullet = &stalactite_collision_bullet,
    .kill_fall = &stalactite_kill_fall,
    .draw = &stalactite_draw,
  };
  return behavior;
}

template<>
BadGuyBehavior const& behavior_of<BombCarrier>()
{
  static BadGuyBehavior const behavior = {
    .construct = &carrier_construct,
    .update = &carrier_update,
    .collision = &carrier_collision,
    .collision_player = &carrier_collision_player,
    .collision_squished = &carrier_collision_squished,
    .ignite = &carrier_ignite,
    .kill_fall = &carrier_kill_fall,
    .is_portable = &carrier_is_portable,
    .grab = &carrier_grab,
  };
  return behavior;
}

template<>
BadGuyBehavior const& behavior_of<Fuse>()
{
  static BadGuyBehavior const behavior = {
    .construct = &fuse_construct,
    .move = &fuse_move,
    .collision_player = &fuse_collision_player,
    .collision_solid = &fuse_collision_solid,
    .collision_badguy = &fuse_collision_badguy,
    .ignite = &fuse_explode,
    .kill_fall = &fuse_explode,
    .is_portable = &fuse_is_portable,
    .grab = &fuse_grab,
    .ungrab = &fuse_ungrab,
    .stop_looping_sounds = &fuse_stop_sound,
    .play_looping_sounds = &fuse_play_sound,
  };
  return behavior;
}

template<>
BadGuyBehavior const& behavior_of<Circler>()
{
  static BadGuyBehavior const behavior = {
    .construct = &circler_construct,
    .move = &circler_move,
  };
  return behavior;
}

template<>
BadGuyBehavior const& behavior_of<Flyer>()
{
  static BadGuyBehavior const behavior = {
    .initialize = &flyer_initialize,
    .activate = &flyer_activate,
    .move = &flyer_move,
    .collision_solid = &flyer_collision_solid,
  };
  return behavior;
}

template<>
BadGuyBehavior const& behavior_of<ElementalFade>()
{
  static BadGuyBehavior const behavior = {
    .after_move = &fade_after_move,
    .freeze = &fade_freeze,
    .ignite = &fade_ignite,
  };
  return behavior;
}

template<>
BadGuyBehavior const& behavior_of<LoopingSound>()
{
  static BadGuyBehavior const behavior = {
    .construct = &sound_construct,
    .activate = &sound_activate,
    .deactivate = &sound_deactivate,
    .after_move = &sound_after_move,
    .stop_looping_sounds = &sound_stop,
    .play_looping_sounds = &sound_play,
  };
  return behavior;
}

template<>
BadGuyBehavior const& behavior_of<Jumper>()
{
  static BadGuyBehavior const behavior = {
    .after_move = &jumper_after_move,
    .collision_solid = &jumper_collision_solid,
    .collision_badguy = &jumper_collision_badguy,
    .freeze = &jumper_freeze,
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

namespace {

// Skydive ------------------------------------------------------------

void skydive_explode(ArchetypeBadguy& self)
{
  if (!self.is_valid())
    return;

  if (self.m_frozen)
    self.default_kill_fall();
  else
  {
    auto& explosion = Sector::get().add<Explosion>(
      get_anchor_pos(self.m_col.m_bbox, ANCHOR_BOTTOM), EXPLOSION_STRENGTH_NEAR);
    explosion.hurts(true);

    self.remove_me();
  }
}

void skydive_construct(ArchetypeBadguy& self)
{
  SoundManager::current()->preload("sounds/explosion.wav");
  self.set_action("normal", 1);
}

void skydive_collision_solid(ArchetypeBadguy& self, CollisionHit const& hit)
{
  if (self.m_frozen)
  {
    self.default_collision_solid(hit);
    return;
  }

  if (hit.bottom) {
    skydive_explode(self);
    return;
  }

  if (hit.left || hit.right)
    self.m_physic.set_velocity_x(0.0);

  skydive_explode(self);
}

HitResponse skydive_collision_badguy(ArchetypeBadguy& self, BadGuy& /*other*/, CollisionHit const& hit)
{
  if (hit.bottom) {
    skydive_explode(self);
    return ABORT_MOVE;
  }
  return FORCE_MOVE;
}

HitResponse skydive_collision_player(ArchetypeBadguy& self, Player& /*player*/, CollisionHit const& hit)
{
  if (hit.bottom) {
    skydive_explode(self);
    return ABORT_MOVE;
  }
  return FORCE_MOVE;
}

bool skydive_collision_squished(ArchetypeBadguy& self, GameObject& object)
{
  if (self.m_frozen)
    return self.default_collision_squished(object);

  if (auto player = dynamic_cast<Player *>(&object)) {
    player->bounce(self);
    return false;
  }

  skydive_explode(self);
  return false;
}

void skydive_collision_tile(ArchetypeBadguy& self, uint32_t tile_attributes)
{
  if (tile_attributes & Tile::HURTS)
  {
    skydive_explode(self);
  }
}

bool skydive_is_portable(ArchetypeBadguy const& /*self*/)
{
  return true;
}

void skydive_grab(ArchetypeBadguy& self, MovingObject& object, Vector const& pos, Direction dir)
{
  Vector movement = pos - self.get_pos();
  self.m_col.set_movement(movement);
  self.m_dir = dir;

  if (!self.m_frozen)
  {
    self.m_physic.set_velocity_x(movement.x * LOGICAL_FPS);
    self.m_physic.set_velocity_y(0.0);
    self.m_physic.set_acceleration_y(0.0);
  }
  self.m_physic.enable_gravity(false);
  self.set_group(COLGROUP_DISABLED);
  self.BadGuy::grab(object, pos, dir);
}

void skydive_ungrab(ArchetypeBadguy& self, MovingObject& object, Direction dir)
{
  if (auto player = dynamic_cast<Player*>(&object))
  {
    throw_by(self, player, dir);
  }
  else if (!self.m_frozen)
  {
    self.m_sprite->set_action("falling", 1);
    self.m_physic.set_velocity_y(0);
    self.m_physic.set_acceleration_y(0);
  }
  self.m_physic.enable_gravity(true);
  self.set_group(self.m_frozen ? COLGROUP_MOVING_STATIC : COLGROUP_MOVING);
  self.BadGuy::ungrab(object, dir);
}

// Owl ----------------------------------------------------------------

Portable* owl_carried(Owl const& owl)
{
  if (owl.carried == entt::null)
    return nullptr;
  return dynamic_cast<Portable*>(GameObjectManager::get_object_by_entity(owl.carried));
}

void owl_drop(ArchetypeBadguy& self, Owl& owl)
{
  if (Portable* carried = owl_carried(owl)) {
    carried->ungrab(self, self.m_dir);
  }
  owl.carried = entt::null;
}

void owl_construct(ArchetypeBadguy& self)
{
  self.set_action(self.m_dir == Direction::LEFT ? "left" : "right", /* loops = */ -1);
}

void owl_initialize(ArchetypeBadguy& self)
{
  Owl& owl = ecs::get<Owl>(self.get_entity());
  self.m_physic.set_velocity_x(self.m_dir == Direction::LEFT ? -owl.speed : owl.speed);
  self.m_physic.enable_gravity(false);
  self.m_sprite->set_action(self.m_dir);

  auto game_object = GameObjectFactory::instance().create(owl.carry, self.get_pos(), self.m_dir);
  if (game_object == nullptr)
  {
    log_fatal("Creating \"{}\" object failed.", owl.carry);
  }
  else if (dynamic_cast<Portable*>(game_object.get()) == nullptr)
  {
    log_warning("Object is not portable: {}", owl.carry);
  }
  else
  {
    owl.carried = game_object->get_entity();
    Sector::get().add_object(std::move(game_object));
  }
}

bool owl_is_above_player(ArchetypeBadguy const& self, Owl const& owl)
{
  auto player = Sector::get().get_nearest_player(self.m_col.m_bbox);
  if (!player)
    return false;

  // Let go of carried objects a short while *before* Tux is below us. This
  // makes it more likely that we'll hit him.
  float x_offset = (self.m_dir == Direction::LEFT) ? owl.activation_distance : -owl.activation_distance;

  Rectf const& bbox = self.m_col.m_bbox;
  Rectf const& player_bbox = player->get_bbox();

  return ((player_bbox.get_top() >= bbox.get_bottom()) /* player is below us */
          && ((player_bbox.get_right() + x_offset) > bbox.get_left())
          && ((player_bbox.get_left() + x_offset) < bbox.get_right()));
}

void owl_after_move(ArchetypeBadguy& self, float /*dt_sec*/)
{
  Owl& owl = ecs::get<Owl>(self.get_entity());

  if (self.m_frozen)
    return;

  Portable* carried = owl_carried(owl);
  if (carried == nullptr)
    return;

  if (!owl_is_above_player(self, owl)) {
    Vector obj_pos = get_anchor_pos(self.m_col.m_bbox, ANCHOR_BOTTOM);
    obj_pos.x -= 16.f; /* FIXME: Actually do use the half width of the carried object here. */
    obj_pos.y += 3.f; /* Move a little away from the hitbox (the body). Looks nicer. */

    //To drop enemie before leave the screen
    if (obj_pos.x <= 16 || obj_pos.x + 16 >= Sector::get().get_width()) {
      owl_drop(self, owl);
    }
    else
      carried->grab(self, obj_pos, self.m_dir);
  }
  else { /* if (is_above_player) */
    owl_drop(self, owl);
  }
}

bool owl_collision_squished(ArchetypeBadguy& self, GameObject& object)
{
  if (self.m_frozen)
    return self.default_collision_squished(object);

  if (auto player = Sector::get().get_nearest_player(self.m_col.m_bbox))
    player->bounce(self);

  owl_drop(self, ecs::get<Owl>(self.get_entity()));

  self.kill_fall();
  return true;
}

void owl_kill_fall(ArchetypeBadguy& self)
{
  if (!self.m_frozen)
  {
    SoundManager::current()->play("sounds/fall.wav", self.get_pos());
    self.m_physic.set_velocity_y(0);
    self.m_physic.set_acceleration_y(0);
    self.m_physic.enable_gravity(true);
    self.set_state(ArchetypeBadguy::STATE_FALLING);
  }
  else
    self.default_kill_fall();

  owl_drop(self, ecs::get<Owl>(self.get_entity()));

  // start dead-script
  self.run_dead_script();
}

void owl_freeze(ArchetypeBadguy& self)
{
  owl_drop(self, ecs::get<Owl>(self.get_entity()));
  self.m_physic.enable_gravity(true);
  self.default_freeze();
}

void owl_unfreeze(ArchetypeBadguy& self, bool melt)
{
  Owl const& owl = ecs::get<Owl>(self.get_entity());
  self.default_unfreeze(melt);
  self.m_physic.set_velocity_x(self.m_dir == Direction::LEFT ? -owl.speed : owl.speed);
  self.m_physic.enable_gravity(false);
  self.m_sprite->set_action(self.m_dir);
}

void owl_collision_solid(ArchetypeBadguy& self, CollisionHit const& hit)
{
  Owl const& owl = ecs::get<Owl>(self.get_entity());

  if (self.m_frozen)
  {
    self.default_collision_solid(hit);
    return;
  }

  if (hit.top || hit.bottom) {
    self.m_physic.set_velocity_y(0);
  } else if (hit.left || hit.right) {
    if (self.m_dir == Direction::LEFT) {
      self.set_action("right", /* loops = */ -1);
      self.m_dir = Direction::RIGHT;
      self.m_physic.set_velocity_x(owl.speed);
    }
    else {
      self.set_action("left", /* loops = */ -1);
      self.m_dir = Direction::LEFT;
      self.m_physic.set_velocity_x(-owl.speed);
    }
  }
}

void owl_ignite(ArchetypeBadguy& self)
{
  owl_drop(self, ecs::get<Owl>(self.get_entity()));
  self.default_ignite();
}

} // namespace

template<>
BadGuyBehavior const& behavior_of<Skydive>()
{
  static BadGuyBehavior const behavior = {
    .construct = &skydive_construct,
    .collision_player = &skydive_collision_player,
    .collision_solid = &skydive_collision_solid,
    .collision_badguy = &skydive_collision_badguy,
    .collision_squished = &skydive_collision_squished,
    .collision_tile = &skydive_collision_tile,
    .kill_fall = &skydive_explode,
    .is_portable = &skydive_is_portable,
    .grab = &skydive_grab,
    .ungrab = &skydive_ungrab,
  };
  return behavior;
}

template<>
BadGuyBehavior const& behavior_of<Owl>()
{
  static BadGuyBehavior const behavior = {
    .construct = &owl_construct,
    .initialize = &owl_initialize,
    .after_move = &owl_after_move,
    .collision_solid = &owl_collision_solid,
    .collision_squished = &owl_collision_squished,
    .freeze = &owl_freeze,
    .unfreeze = &owl_unfreeze,
    .ignite = &owl_ignite,
    .kill_fall = &owl_kill_fall,
  };
  return behavior;
}

namespace {

// PathFollower -------------------------------------------------------

void badguy_path_read(ArchetypeBadguy& self, ReaderMapping const& mapping)
{
  PathFollower& follower = ecs::get<PathFollower>(self.get_entity());
  follower.path.value = std::make_unique<PathObject>();
  follower.path.value->init_path(mapping, follower.running);
}

void badguy_path_move_to(ArchetypeBadguy& self, Vector const& pos)
{
  Vector shift = pos - self.m_col.m_bbox.p1();
  if (PathObject* path = path_follower::get(self); path && path->get_path()) {
    path->get_path()->move_by(shift);
  }
  self.set_pos(pos);
}

// Ghoul --------------------------------------------------------------

void ghoul_construct(ArchetypeBadguy& self)
{
  self.m_sprite->set_action(self.m_dir);
}

void ghoul_finish_construction(ArchetypeBadguy& self)
{
  PathObject* path = path_follower::get(self);
  if (path && path->get_walker() && path->get_walker()->is_running()) {
    ecs::get<Ghoul>(self.get_entity()).state = Ghoul::State::PATHMOVING_TRACK;
  }
}

void ghoul_deactivate(ArchetypeBadguy& self)
{
  Ghoul& ghoul = ecs::get<Ghoul>(self.get_entity());
  if (ghoul.state == Ghoul::State::TRACKING) {
    ghoul.state = Ghoul::State::IDLE;
  }
}

bool ghoul_collision_squished(ArchetypeBadguy& self, GameObject& /*object*/)
{
  if (auto player = Sector::get().get_nearest_player(self.m_col.m_bbox))
    player->bounce(self);
  self.m_sprite->set_action("squished", 1);
  self.kill_fall();
  return true;
}

void ghoul_move(ArchetypeBadguy& self, float dt_sec)
{
  Ghoul& ghoul = ecs::get<Ghoul>(self.get_entity());

  auto player = self.get_nearest_player();
  if (!player)
    return;

  Rectf const& bbox = self.m_col.m_bbox;
  Vector p1 = bbox.get_middle();
  Vector p2 = player->get_bbox().get_middle();
  Vector dist = (p2 - p1);

  Rectf const& player_bbox = player->get_bbox();

  if (player_bbox.get_right() < bbox.get_left()) {
    self.m_sprite->set_action("left", -1);
  }
  if (player_bbox.get_left() > bbox.get_right()) {
    self.m_sprite->set_action("right", -1);
  }

  switch (ghoul.state) {
    case Ghoul::State::STOPPED:
      break;

    case Ghoul::State::IDLE:
      if (glm::length(dist) <= ghoul.track_range) {
        ghoul.state = Ghoul::State::TRACKING;
      }
      break;

    case Ghoul::State::TRACKING:
      if (glm::length(dist) >= 1) {
        Vector dir_ = glm::normalize(dist);
        self.m_col.set_movement(dir_ * dt_sec * ghoul.flyspeed);
      } else {
        /* We somehow landed right on top of the player without colliding.
         * Sit tight and avoid a division by zero. */
      }
      break;

    case Ghoul::State::PATHMOVING:
    case Ghoul::State::PATHMOVING_TRACK: {
      PathObject* path = path_follower::get(self);
      if (path == nullptr || path->get_walker() == nullptr)
        return;
      path->get_walker()->update(dt_sec);
      self.m_col.set_movement(path->get_walker()->get_pos(bbox.get_size(), path->get_path_handle()) - self.get_pos());
      if (ghoul.state == Ghoul::State::PATHMOVING_TRACK && glm::length(dist) <= ghoul.track_range) {
        ghoul.state = Ghoul::State::TRACKING;
      }
      break;
    }
  }
}

} // namespace

template<>
BadGuyBehavior const& behavior_of<PathFollower>()
{
  static BadGuyBehavior const behavior = {
    .read = &badguy_path_read,
    .move_to = &badguy_path_move_to,
  };
  return behavior;
}

template<>
BadGuyBehavior const& behavior_of<Ghoul>()
{
  static BadGuyBehavior const behavior = {
    .construct = &ghoul_construct,
    .finish_construction = &ghoul_finish_construction,
    .deactivate = &ghoul_deactivate,
    .move = &ghoul_move,
    .collision_squished = &ghoul_collision_squished,
  };
  return behavior;
}

/* EOF */
