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
#include "ecs/object_behaviors.hpp"

#include "audio/sound_manager.hpp"
#include "badguy/badguy.hpp"
#include "badguy/crusher.hpp"
#include "control/controller.hpp"
#include "ecs/badguy_components.hpp"
#include "object/lit_object.hpp"
#include "object/portable_object.hpp"
#include "object/pushbutton.hpp"
#include "scripting/rock.hpp"
#include "object/path.hpp"
#include "object/path_walker.hpp"
#include "scripting/platform.hpp"
#include "squirrel/squirrel_util.hpp"
#include "object/bouncy_coin.hpp"
#include "audio/sound_source.hpp"
#include "object/coin_explode.hpp"
#include "object/coin_rain.hpp"
#include "object/flower.hpp"
#include "object/growup.hpp"
#include "object/oneup.hpp"
#include "object/portable.hpp"
#include "object/powerup.hpp"
#include "object/specialriser.hpp"
#include "object/star.hpp"
#include "supertux/game_object_factory.hpp"
#include "supertux/level.hpp"
#include "util/reader_collection.hpp"
#include "util/reader_object.hpp"
#include "video/surface.hpp"
#include "math/random.hpp"
#include "object/archetype_object.hpp"
#include "object/bullet.hpp"
#include "object/explosion.hpp"
#include "math/util.hpp"
#include "object/camera.hpp"
#include "object/player.hpp"
#include "object/sprite_particle.hpp"
#include "supertux/game_session.hpp"
#include "supertux/globals.hpp"
#include "video/video_system.hpp"
#include "video/viewport.hpp"
#include "sprite/sprite.hpp"
#include "sprite/sprite_manager.hpp"
#include "supertux/constants.hpp"
#include "supertux/physic.hpp"
#include "supertux/sector.hpp"
#include "util/log.hpp"
#include "video/drawing_context.hpp"

namespace {

// UnstableTile -------------------------------------------------------

constexpr float DELAY_IF_TUX = 0.001f;

void unstable_fall_down(ArchetypeObject& self, UnstableTile& tile)
{
  if (tile.state == UnstableTile::State::FALL)
    return;

  Physic& physic = ecs::get<Physic>(self.get_entity());
  if (self.m_sprite->has_action("fall-down"))
  {
    tile.state = UnstableTile::State::FALL;
    self.set_action("fall-down", /* loops = */ 1);
    physic.set_gravity_modifier(.98f);
    physic.enable_gravity(true);
  }
  else
  {
    tile.state = UnstableTile::State::FALL;
  }
}

void unstable_slow_fall(ArchetypeObject& self, UnstableTile& tile)
{
  /* Only enter slow-fall if neither shake nor dissolve is available. */
  if (tile.state != UnstableTile::State::NORMAL)
  {
    unstable_fall_down(self, tile);
    return;
  }

  Physic& physic = ecs::get<Physic>(self.get_entity());
  if (self.m_sprite->has_action("fall-down"))
  {
    tile.state = UnstableTile::State::SLOWFALL;
    self.set_action("fall-down", /* loops = */ 1);
    physic.set_gravity_modifier(.10f);
    physic.enable_gravity(true);
    tile.original_pos = self.m_col.get_pos();
    tile.slowfall_timer = 0.5f; /* Fall slowly for half a second. */
  }
  else
  {
    tile.state = UnstableTile::State::FALL;
  }
}

void unstable_dissolve(ArchetypeObject& self, UnstableTile& tile)
{
  if ((tile.state != UnstableTile::State::NORMAL) && (tile.state != UnstableTile::State::SHAKE))
    return;

  if (self.m_sprite->has_action("dissolve"))
  {
    tile.state = UnstableTile::State::DISSOLVE;
    self.set_action("dissolve", /* loops = */ 1);
  }
  else
  {
    unstable_slow_fall(self, tile);
  }
}

void unstable_shake(ArchetypeObject& self, UnstableTile& tile)
{
  if (tile.state != UnstableTile::State::NORMAL)
    return;

  if (self.m_sprite->has_action("shake"))
  {
    tile.state = UnstableTile::State::SHAKE;
    self.set_action("shake", /* loops = */ 1);
  }
  else
  {
    unstable_dissolve(self, tile);
  }
}

void unstable_revive(ArchetypeObject& self, UnstableTile& tile)
{
  Physic& physic = ecs::get<Physic>(self.get_entity());
  tile.state = UnstableTile::State::NORMAL;
  self.set_group(COLGROUP_STATIC);
  physic.enable_gravity(false);
  physic.set_velocity(Vector(0.0f, 0.0f));
  self.m_col.set_pos(tile.original_pos);
  self.m_col.set_movement(Vector(0.0f, 0.0f));
  tile.revive_timer.stop();
  tile.respawn = std::make_shared<FadeHelper>(&tile.alpha, tile.fade_in_time, 1.f);
  self.m_sprite->set_action("normal");
}

void unstable_construct(ArchetypeObject& self)
{
  UnstableTile& tile = ecs::get<UnstableTile>(self.get_entity());
  Physic& physic = ecs::emplace<Physic>(self.get_entity());
  tile.original_pos = self.m_col.get_pos();
  self.m_sprite->set_action("normal");
  physic.set_gravity_modifier(.98f);
  physic.enable_gravity(false);
}

HitResponse unstable_collision(ArchetypeObject& self, GameObject& other, CollisionHit const& /*hit*/)
{
  UnstableTile& tile = ecs::get<UnstableTile>(self.get_entity());
  if (tile.state == UnstableTile::State::NORMAL)
  {
    Rectf const& bbox = self.m_col.m_bbox;
    Player* player = dynamic_cast<Player*>(&other);
    if (player != nullptr &&
        (player->get_bbox().get_bottom() < bbox.get_top() + SHIFT_DELTA ||
         player->get_bbox().get_top() < bbox.get_bottom() + SHIFT_DELTA))
    {
      unstable_shake(self, tile);
    }

    if (dynamic_cast<Explosion*>(&other))
    {
      unstable_shake(self, tile);
    }
  }
  return FORCE_MOVE;
}

void unstable_update(ArchetypeObject& self, float dt_sec)
{
  UnstableTile& tile = ecs::get<UnstableTile>(self.get_entity());
  Physic& physic = ecs::get<Physic>(self.get_entity());

  if (tile.respawn)
  {
    tile.respawn->update(dt_sec);
    if (tile.respawn->completed())
      tile.respawn.reset();
  }

  switch (tile.state)
  {
    case UnstableTile::State::NORMAL:
      break;

    case UnstableTile::State::SHAKE:
      if (self.m_sprite->animation_done())
        unstable_dissolve(self, tile);
      break;

    case UnstableTile::State::DISSOLVE:
      if (self.m_sprite->animation_done()) {
        /* dissolving is done. Set to non-solid. */
        self.set_group(COLGROUP_DISABLED);
        unstable_fall_down(self, tile);
      }
      break;

    case UnstableTile::State::SLOWFALL:
      if (tile.slowfall_timer >= dt_sec)
        tile.slowfall_timer -= dt_sec;
      else /* Switch to normal falling procedure */
        unstable_fall_down(self, tile);
      self.m_col.set_movement(physic.get_movement(dt_sec));
      break;

    case UnstableTile::State::FALL:
      tile.alpha = std::max(tile.alpha - dt_sec / tile.fade_out_time, 0.f);
      if (!tile.revive_timer.started())
      {
        if (tile.revive_timer.check())
        {
          if (Sector::current() && Sector::get().is_free_of_movingstatics(self.m_col.m_bbox.grown(-1.f)))
          {
            unstable_revive(self, tile);
          }
          else
          {
            tile.revive_timer.start(DELAY_IF_TUX);
          }
        }
        else
        {
          tile.revive_timer.start(tile.respawn_time);
        }
      }
      else if (tile.alpha > 0.f)
      {
        self.m_col.set_movement(physic.get_movement(dt_sec));
      }
      else
      {
        self.set_group(COLGROUP_DISABLED);
      }
      break;
  }
}

void unstable_draw(ArchetypeObject& self, DrawingContext& context)
{
  context.push_transform();
  context.transform().alpha *= ecs::get<UnstableTile>(self.get_entity()).alpha;
  self.default_draw(context);
  context.pop_transform();
}

// WeakBlock ----------------------------------------------------------

constexpr char const* STRAWBOX_SPRITE = "images/objects/weak_block/strawbox.sprite";
constexpr char const* MELTBOX_SPRITE = "images/objects/weak_block/meltbox.sprite";

std::shared_ptr<Sprite> weak_block_light(std::string const& sprite)
{
  std::shared_ptr<Sprite> light = SpriteManager::current()->create(sprite);
  light->set_blend(Blend::ADD);
  light->set_color(Color(0.3f, 0.2f, 0.1f));
  return light;
}

void weak_block_construct(ArchetypeObject& self)
{
  WeakBlock& block = ecs::get<WeakBlock>(self.get_entity());

  self.m_sprite->set_action("normal");

  // unlinked blocks melt instead of burning and do not spread
  if (!block.linked) {
    self.m_default_sprite_name = block.unlinked_sprite;
    self.m_sprite_name = self.m_default_sprite_name;
    self.m_sprite = SpriteManager::current()->create(self.m_sprite_name);
    self.m_sprite->set_action("normal");
  }

  block.lightsprite = weak_block_light("images/objects/lightmap_light/lightmap_light-small.sprite");

  if (self.m_sprite_name == STRAWBOX_SPRITE) {
    SoundManager::current()->preload("sounds/fire.ogg"); // TODO: use own sound?
  } else if (self.m_sprite_name == MELTBOX_SPRITE) {
    SoundManager::current()->preload("sounds/sizzle.ogg");
  }
}

HitResponse weak_block_collision_bullet(ArchetypeObject& self, Bullet& bullet, CollisionHit const& hit)
{
  switch (ecs::get<WeakBlock>(self.get_entity()).state) {
    case WeakBlock::State::NORMAL:
      //Ensure only fire destroys weakblock
      if (bullet.get_type() == FIRE_BONUS) {
        weak_block::start_burning(self);
        bullet.remove_me();
      }
      //Other bullets ricochet
      else {
        bullet.ricochet(self, hit);
      }
      break;
    case WeakBlock::State::BURNING:
    case WeakBlock::State::DISINTEGRATING:
      break;
  }

  return FORCE_MOVE;
}

HitResponse weak_block_collision(ArchetypeObject& self, GameObject& other, CollisionHit const& hit)
{
  switch (ecs::get<WeakBlock>(self.get_entity()).state) {
    case WeakBlock::State::NORMAL:
      if (auto bullet = dynamic_cast<Bullet*> (&other)) {
        return weak_block_collision_bullet(self, *bullet, hit);
      }
      break;

    case WeakBlock::State::BURNING:
      if (self.m_sprite_name != STRAWBOX_SPRITE)
        break;

      if (auto badguy = dynamic_cast<BadGuy*> (&other)) {
        badguy->ignite();
      }
      break;

    case WeakBlock::State::DISINTEGRATING:
      break;
  }

  return FORCE_MOVE;
}

/** pass hit to nearby weak blocks */
void weak_block_spread_hit(ArchetypeObject& self, WeakBlock const& block)
{
  //Destroy adjacent weakblocks if applicable
  if (block.linked) {
    for (auto& other : Sector::get().get_objects_by_type<ArchetypeObject>()) {
      auto* other_block = ecs::try_get<WeakBlock>(other.get_entity());
      if (other_block && &other != &self && other_block->state == WeakBlock::State::NORMAL)
      {
        const float dx = fabsf(other.get_pos().x - self.m_col.m_bbox.get_left());
        const float dy = fabsf(other.get_pos().y - self.m_col.m_bbox.get_top());
        if ((dx <= 32.5f) && (dy <= 32.5f)) {
          weak_block::start_burning(other);
        }
      }
    }
  }
}

void weak_block_update(ArchetypeObject& self, float /*dt_sec*/)
{
  WeakBlock& block = ecs::get<WeakBlock>(self.get_entity());

  switch (block.state) {
    case WeakBlock::State::NORMAL:
      break;

    case WeakBlock::State::BURNING:
      // cause burn light to flicker randomly
      if (block.linked) {
        if (gameRandom.rand(10) >= 7) {
          block.lightsprite->set_color(Color(0.2f + gameRandom.randf(20.0f) / 100.0f,
                                             0.1f + gameRandom.randf(20.0f)/100.0f,
                                             0.1f));
        } else
          block.lightsprite->set_color(Color(0.3f, 0.2f, 0.1f));
      }

      if (self.m_sprite->animation_done()) {
        block.state = WeakBlock::State::DISINTEGRATING;
        self.m_sprite->set_action("disintegrating", 1);
        weak_block_spread_hit(self, block);
        self.set_group(COLGROUP_DISABLED);
        block.lightsprite = weak_block_light("images/objects/lightmap_light/lightmap_light-tiny.sprite");
      }
      break;

    case WeakBlock::State::DISINTEGRATING:
      if (self.m_sprite->animation_done()) {
        self.remove_me();
        return;
      }
      break;
  }
}

void weak_block_draw(ArchetypeObject& self, DrawingContext& context)
{
  WeakBlock const& block = ecs::get<WeakBlock>(self.get_entity());

  //Draw the Sprite just in front of other objects
  self.m_sprite->draw(context.color(), self.get_pos(), LAYER_OBJECTS + 10, self.m_flip);

  if (block.linked && (block.state != WeakBlock::State::NORMAL))
  {
    block.lightsprite->draw(context.light(), self.m_col.m_bbox.get_middle(), 0);
  }
}

// MagicBlock ---------------------------------------------------------

void magicblock_construct(ArchetypeObject& self)
{
  MagicBlock& block = ecs::get<MagicBlock>(self.get_entity());

  // all alpha to make the sprite still visible
  block.color.alpha = block.alpha_solid;

  // set trigger
  if (block.color.red == 0 && block.color.green == 0 && block.color.blue == 0) { // is it black?
    block.black = true;
    block.trigger_red = block.min_intensity;
    block.trigger_green = block.min_intensity;
    block.trigger_blue = block.min_intensity;
  } else {
    block.black = false;
    block.trigger_red = block.color.red;
    block.trigger_green = block.color.green;
    block.trigger_blue = block.color.blue;
  }

  Rectf const& bbox = self.m_col.m_bbox;
  block.center = bbox.get_middle();
  block.solid_box = Rectf(bbox.get_left() + SHIFT_DELTA, bbox.get_top() + SHIFT_DELTA,
                          bbox.get_right() - SHIFT_DELTA, bbox.get_bottom() - SHIFT_DELTA);
}

void magicblock_update(ArchetypeObject& self, float dt_sec)
{
  MagicBlock& block = ecs::get<MagicBlock>(self.get_entity());

  // Check if center of this block is on screen.
  // Don't update if not, because there is no light off screen.
  float screen_left = Sector::get().get_camera().get_translation().x;
  float screen_top = Sector::get().get_camera().get_translation().y;
  float screen_right = screen_left + static_cast<float>(SCREEN_WIDTH);
  float screen_bottom = screen_top + static_cast<float>(SCREEN_HEIGHT);
  if ((block.center.x > screen_right) || (block.center.y > screen_bottom) ||
      (block.center.x < screen_left) || (block.center.y < screen_top)) {
    block.switch_delay = block.switch_delay_time;
    return;
  }

  bool lighting_ok;
  if (block.black) {
    lighting_ok = (block.light.red >= block.trigger_red ||
                   block.light.green >= block.trigger_green ||
                   block.light.blue >= block.trigger_blue);
  } else {
    lighting_ok = (block.light.red >= block.trigger_red &&
                   block.light.green >= block.trigger_green &&
                   block.light.blue >= block.trigger_blue);
  }

  // overrule lighting_ok if switch_delay has not yet passed
  if (lighting_ok == block.is_solid) {
    block.switch_delay = block.switch_delay_time;
  } else {
    if (block.switch_delay > 0) {
      lighting_ok = block.is_solid;
      block.switch_delay -= dt_sec;
    }
  }

  if (lighting_ok) {
    // lighting suggests going solid
    if (!block.is_solid) {
      if (Sector::get().is_free_of_movingstatics(block.solid_box, &self)) {
        block.is_solid = true;
        block.solid_time = 0;
        block.switch_delay = block.switch_delay_time;
      }
    }
  } else {
    // lighting suggests going nonsolid
    if (block.solid_time >= block.min_solid_time) {
      block.is_solid = false;
    }
  }

  // Update Sprite.
  if (block.is_solid) {
    block.solid_time += dt_sec;
    block.color.alpha = block.alpha_solid;
    self.m_sprite->set_action("solid");
    self.set_group(COLGROUP_STATIC);
  } else {
    block.color.alpha = block.alpha_nonsolid;
    self.m_sprite->set_action("normal");
    self.set_group(COLGROUP_DISABLED);
  }
}

void magicblock_draw(ArchetypeObject& self, DrawingContext& context)
{
  // Ask for update about lightmap at center of this block
  // context.light().get_pixel(m_center, m_light);
  log_warning("FIXME: get_pixel() no longer supported, implement this differently");

  self.default_draw(context);
  context.color().draw_filled_rect(self.m_col.m_bbox, ecs::get<MagicBlock>(self.get_entity()).color, self.m_layer);
}

bool magicblock_collides(ArchetypeObject const& self, GameObject& /*other*/, CollisionHit const& /*hit*/)
{
  return ecs::get<MagicBlock>(self.get_entity()).is_solid;
}

// ResetPoint ---------------------------------------------------------

/** Color and offset of the light of torch sprites */
const Color TORCH_LIGHT_COLOR = Color(0.87f, 0.64f, 0.12f);
const Vector TORCH_LIGHT_OFFSET = Vector(0, 12);

bool is_torch(ArchetypeObject const& self)
{
  return self.m_sprite_name.find("torch", 0) != std::string::npos;
}

void reset_point_reactivate(ArchetypeObject& self, ResetPoint const& point)
{
  if (!GameSession::current()) {
    return;
  }

  if (!GameSession::current()->get_reset_point_sectorname().empty() &&
      GameSession::current()->get_reset_point_pos() == point.initial_position) {
    // TODO: && GameSession::current()->get_reset_point_sectorname() ==  <sector this firefly is in>
    // GameSession::current()->get_current_sector()->get_name() is not yet initialized.
    // Worst case a resetpoint in a different sector at the same position as the real
    // resetpoint the player is spawning is set to ringing, too. Until we can check the sector, too, dont set
    // activated = true; here.
    self.m_sprite->set_action("ringing");
  }
}

void reset_point_construct(ArchetypeObject& self)
{
  ResetPoint& point = ecs::get<ResetPoint>(self.get_entity());
  point.initial_position = self.get_pos();

  // a level-specific sprite (bell variants, torch) brings its own light and sound
  if (self.m_sprite_name != self.m_default_sprite_name) {
    self.m_sprite = SpriteManager::current()->create(self.m_sprite_name);
    self.m_col.m_bbox.set_size(self.m_sprite->get_current_hitbox_width(), self.m_sprite->get_current_hitbox_height());

    if (is_torch(self)) {
      point.light = SpriteManager::current()->create("images/objects/lightmap_light/lightmap_light-small.sprite");
      point.light->set_blend(Blend::ADD);
      point.light->set_color(TORCH_LIGHT_COLOR);
    }

    reset_point_reactivate(self, point);

    if (self.m_sprite_name.find("vbell", 0) != std::string::npos) {
      SoundManager::current()->preload("sounds/savebell_low.wav");
    }
    else if (is_torch(self)) {
      SoundManager::current()->preload("sounds/fire.ogg");
    }
    else {
      SoundManager::current()->preload("sounds/savebell2.wav");
    }
  } else {
    reset_point_reactivate(self, point);
  }
}

void reset_point_draw(ArchetypeObject& self, DrawingContext& context)
{
  ResetPoint const& point = ecs::get<ResetPoint>(self.get_entity());
  self.default_draw(context);

  if (is_torch(self) && (point.activated || self.m_sprite->get_action() == "ringing")) {
    point.light->draw(context.light(), self.m_col.m_bbox.get_middle() +
                      (self.m_flip == NO_FLIP ? -TORCH_LIGHT_OFFSET : TORCH_LIGHT_OFFSET), 0);
  }
}

HitResponse reset_point_collision(ArchetypeObject& self, GameObject& other, CollisionHit const& /*hit*/)
{
  ResetPoint& point = ecs::get<ResetPoint>(self.get_entity());

  // If the bell is already activated, don't ring it again!
  if (point.activated || self.m_sprite->get_action() == "ringing")
    return ABORT_MOVE;

  if (dynamic_cast<Player*>(&other)) {
    point.activated = true;

    // spawn some particles
    for (int i = 0; i < 5; i++) {
      Vector ppos = self.m_col.m_bbox.get_middle();
      float angle = graphicsRandom.randf(-math::PI_2, math::PI_2);
      float velocity = graphicsRandom.randf(450.0f, 900.0f);
      float vx = sinf(angle)*velocity;
      float vy = -cosf(angle)*velocity;
      Vector pspeed = Vector(vx, vy);
      Vector paccel = Vector(0.0f, 1000.0f);
      Sector::get().add<SpriteParticle>("images/particles/reset.sprite", "default", ppos, ANCHOR_MIDDLE, pspeed, paccel, LAYER_OBJECTS-1);
    }

    if (self.m_sprite_name.find("vbell", 0) != std::string::npos) {
      SoundManager::current()->play("sounds/savebell_low.wav", self.get_pos());
    }
    else if (is_torch(self)) {
      SoundManager::current()->play("sounds/fire.ogg", self.get_pos());
    }
    else {
      SoundManager::current()->play("sounds/savebell2.wav", self.get_pos());
    }

    self.m_sprite->set_action("ringing");
    GameSession::current()->set_reset_point(Sector::get().get_name(), point.initial_position);
  }

  return ABORT_MOVE;
}

// Block --------------------------------------------------------------

std::unique_ptr<MovingObject> to_moving_object(std::unique_ptr<GameObject> object)
{
  if (dynamic_cast<MovingObject*>(object.get()) != nullptr) {
    return std::unique_ptr<MovingObject>(static_cast<MovingObject*>(object.release()));
  } else {
    return std::unique_ptr<MovingObject>();
  }
}

const float upgrade_sound_gain = 0.3f;

constexpr float BOUNCY_BRICK_MAX_OFFSET = 8;
constexpr float BOUNCY_BRICK_SPEED = 90;
constexpr float BUMP_ROTATION_ANGLE = 10;

void block_start_break(ArchetypeObject& self, GameObject* hitter)
{
  block::start_bounce(self, hitter);
  ecs::get<Block>(self.get_entity()).breaking = true;
}

void block_break_me(ArchetypeObject& self)
{
  const auto gravity = Sector::get().get_gravity() * 100;
  Vector pos = self.get_pos() + Vector(16.0f, 16.0f);

  for (char const* action : {"piece1", "piece2", "piece3", "piece4", "piece5", "piece6"})
  {
    Vector velocity(graphicsRandom.randf(-100, 100),
                    graphicsRandom.randf(-400, -300));
    Sector::get().add<SpriteParticle>(self.m_sprite->clone(), action,
                                      pos, ANCHOR_MIDDLE,
                                      velocity, Vector(0, gravity),
                                      LAYER_OBJECTS + 3);
  }

  self.remove_me();
}

void block_construct(ArchetypeObject& self)
{
  self.m_col.m_bbox.set_size(32, 32.1f);
  SoundManager::current()->preload("sounds/upgrade.wav");
  SoundManager::current()->preload("sounds/brick.wav");
}

HitResponse block_collision(ArchetypeObject& self, GameObject& other, CollisionHit const& /*hit*/)
{
  Block const& block = ecs::get<Block>(self.get_entity());
  Rectf const& bbox = self.m_col.m_bbox;

  auto player = dynamic_cast<Player*> (&other);
  if (player)
  {
    if (player->is_swimboosting())
    {
      self.hit(*player);
    }
    else if (!player->is_water_jumping() && !player->is_swimming())
    {
      bool x_coordinates_intersect =
        player->get_bbox().get_right() >= bbox.get_left() &&
        player->get_bbox().get_left() <= bbox.get_right();
      if (player->get_bbox().get_top() > bbox.get_bottom() - SHIFT_DELTA &&
          x_coordinates_intersect)
      {
        self.hit(*player);
      }
    }
  }

  // only interact with other objects if...
  //   1) we are bouncing
  //   2) the object is not portable (either never or not currently)
  //   3) the object is being hit from below (baguys don't get killed for activating boxes)
  auto badguy = dynamic_cast<BadGuy*> (&other);
  auto portable = dynamic_cast<Portable*> (&other);
  auto moving_object = dynamic_cast<MovingObject*> (&other);
  bool is_portable = ((portable != nullptr) && portable->is_portable());
  bool hit_mo_from_below = ((moving_object == nullptr) || (moving_object->get_bbox().get_bottom() < (bbox.get_top() + SHIFT_DELTA)));
  // portable badguys (e.g. bombs) still get hit
  if (block.bouncing && (!is_portable || badguy) && hit_mo_from_below) {

    // Badguys get killed
    if (badguy) {
      badguy->kill_fall();
    }

    // Coins get collected
    auto coin_object = dynamic_cast<ArchetypeObject*>(&other);
    if (coin_object && ecs::try_get<Coin>(coin_object->get_entity())) {
      coin::collect(*coin_object);
    }

    //Eggs get jumped
    if (auto growup = dynamic_cast<GrowUp*> (&other)) {
      growup->do_jump();
    }
  }

  return FORCE_MOVE;
}

void block_update(ArchetypeObject& self, float dt_sec)
{
  Block& block = ecs::get<Block>(self.get_entity());
  if (!block.bouncing)
    return;

  float offset = block.original_y - self.get_pos().y;
  if (offset > BOUNCY_BRICK_MAX_OFFSET) {
    block.bounce_dir = BOUNCY_BRICK_SPEED;
    self.m_col.set_movement(Vector(0, block.bounce_dir * dt_sec));
    if (block.breaking) {
      block_break_me(self);
    }
  } else if (offset < BOUNCY_BRICK_SPEED * dt_sec && block.bounce_dir > 0) {
    self.m_col.set_movement(Vector(0, offset));
    block.bounce_dir = 0;
    block.bouncing = false;
    self.m_sprite->set_angle(0);
  } else {
    self.m_col.set_movement(Vector(0, block.bounce_dir * dt_sec));
  }
}

void block_draw(ArchetypeObject& self, DrawingContext& context)
{
  self.m_sprite->draw(context.color(), self.get_pos(), LAYER_OBJECTS+1, self.m_flip);
}

// BonusBlock ---------------------------------------------------------

BonusBlock::Content bonus_content_by_data(int tile_data)
{
  // Warning: 'tile_data' can't be cast to 'Content', this manual
  // conversion is necessary
  switch (tile_data) {
    case 1: return BonusBlock::Content::COIN;
    case 2: return BonusBlock::Content::FIREGROW;
    case 3: return BonusBlock::Content::STAR;
    case 4: return BonusBlock::Content::ONEUP;
    case 5: return BonusBlock::Content::ICEGROW;
    case 6: return BonusBlock::Content::LIGHT;
    case 7: return BonusBlock::Content::TRAMPOLINE;
    case 8: return BonusBlock::Content::CUSTOM; // Trampoline
    case 9: return BonusBlock::Content::CUSTOM; // Rock
    case 10: return BonusBlock::Content::RAIN;
    case 11: return BonusBlock::Content::EXPLODE;
    case 12: return BonusBlock::Content::CUSTOM; // Red potion
    case 13: return BonusBlock::Content::AIRGROW;
    case 14: return BonusBlock::Content::EARTHGROW;
    case 15: return BonusBlock::Content::LIGHT_ON;
    default:
      log_warning("Invalid box contents");
      return BonusBlock::Content::COIN;
  }
}

BonusBlock::Content bonus_content_from_string(std::string const& contentstring)
{
  if (contentstring == "coin") {
    return BonusBlock::Content::COIN;
  } else if (contentstring == "firegrow") {
    return BonusBlock::Content::FIREGROW;
  } else if (contentstring == "icegrow") {
    return BonusBlock::Content::ICEGROW;
  } else if (contentstring == "airgrow") {
    return BonusBlock::Content::AIRGROW;
  } else if (contentstring == "earthgrow") {
    return BonusBlock::Content::EARTHGROW;
  } else if (contentstring == "star") {
    return BonusBlock::Content::STAR;
  } else if (contentstring == "1up") {
    return BonusBlock::Content::ONEUP;
  } else if (contentstring == "custom") {
    return BonusBlock::Content::CUSTOM;
  } else if (contentstring == "script") { // use when bonusblock is to contain ONLY a script
    return BonusBlock::Content::SCRIPT;
  } else if (contentstring == "light") {
    return BonusBlock::Content::LIGHT;
  } else if (contentstring == "light-on") {
    return BonusBlock::Content::LIGHT_ON;
  } else if (contentstring == "trampoline") {
    return BonusBlock::Content::TRAMPOLINE;
  } else if (contentstring == "rain") {
    return BonusBlock::Content::RAIN;
  } else if (contentstring == "explode") {
    return BonusBlock::Content::EXPLODE;
  } else {
    log_warning("Invalid box contents '{}'", contentstring);
    return BonusBlock::Content::COIN;
  }
}

void bonus_preload_contents(ArchetypeObject& self, BonusBlock& block, int d)
{
  switch (d)
  {
    case 6: // Light
    case 15: // Light (On)
      SoundManager::current()->preload("sounds/switch.ogg");
      block.lightsprite = Surface::from_file("/images/objects/lightmap_light/bonusblock_light.png");
      break;

    case 8: // Trampoline
      block.object.value = trampoline::create(self.get_pos(), true);
      break;

    case 9: // Rock
      block.object.value = PortableObject::create("rock", self.get_pos());
      break;

    case 12: // Red potion
      block.object.value = std::make_unique<PowerUp>(self.get_pos(), "images/powerups/potions/red-potion.sprite");
      break;

    default:
      break;
  }
}

void bonus_read(ArchetypeObject& self, ReaderMapping const& mapping)
{
  BonusBlock& block = ecs::get<BonusBlock>(self.get_entity());

  int d = mapping.get("data", 0);
  block.contents = bonus_content_by_data(d);
  bonus_preload_contents(self, block, d);

  std::string contentstring;
  if (mapping.read("contents", contentstring))
  {
    block.contents = bonus_content_from_string(contentstring);

    if (block.contents == BonusBlock::Content::CUSTOM)
    {
      ReaderCollection content_collection;
      if (!mapping.read("custom-contents", content_collection))
      {
        log_warning("bonusblock is missing 'custom-contents' tag");
      }
      else
      {
        auto const& object_specs = content_collection.get_objects();
        if (!object_specs.empty()) {
          if (object_specs.size() > 1) {
            log_warning("only one object allowed in bonusblock 'custom-contents', ignoring the rest");
          }

          ReaderObject const& spec = object_specs[0];
          auto game_object = GameObjectFactory::instance().create(spec.get_name(), spec.get_mapping());
          block.object.value = to_moving_object(std::move(game_object));
          if (!block.object.value) {
            log_warning("Only MovingObjects are allowed inside BonusBlocks");
          }
        }
      }
    }
  }

  if (block.contents == BonusBlock::Content::CUSTOM && !block.object.value) {
    throw std::runtime_error("Need to specify content object for custom block");
  }

  if (block.contents == BonusBlock::Content::LIGHT || block.contents == BonusBlock::Content::LIGHT_ON) {
    SoundManager::current()->preload("sounds/switch.ogg");
    block.lightsprite = Surface::from_file("/images/objects/lightmap_light/bonusblock_light.png");
    if (block.contents == BonusBlock::Content::LIGHT_ON) {
      self.m_sprite->set_action("on");
    }
  }
}

Direction bonus_direction(ArchetypeObject const& self, Player const& player)
{
  return (player.get_bbox().get_middle().x > self.m_col.m_bbox.get_middle().x) ? Direction::LEFT : Direction::RIGHT;
}

void bonus_raise_growup(ArchetypeObject& self, Player* player, BonusType const& bonus, Direction const& dir)
{
  std::unique_ptr<MovingObject> obj;
  if (player->get_status().bonus[player->get_id()] == NO_BONUS)
  {
    obj = std::make_unique<GrowUp>(self.get_pos(), dir);
  }
  else
  {
    obj = std::make_unique<Flower>(bonus);
  }

  Sector::get().add<SpecialRiser>(self.get_pos(), std::move(obj));
  SoundManager::current()->play("sounds/upgrade.wav", self.get_pos(), upgrade_sound_gain);
}

void bonus_drop_growup(ArchetypeObject& self, Player* player, std::string const& bonus_sprite_name,
                       Direction const& dir, bool& countdown)
{
  if (player->get_status().bonus[player->get_id()] == NO_BONUS)
  {
    Sector::get().add<GrowUp>(self.get_pos() + Vector(0, 32), dir);
  }
  else
  {
    Sector::get().add<PowerUp>(self.get_pos() + Vector(0, 32), bonus_sprite_name);
  }
  SoundManager::current()->play("sounds/upgrade.wav", self.get_pos(), upgrade_sound_gain);
  countdown = true;
}

void bonus_try_drop(ArchetypeObject& self, Player* player)
{
  BonusBlock& block = ecs::get<BonusBlock>(self.get_entity());
  Rectf const& bbox = self.m_col.m_bbox;

  SoundManager::current()->play("sounds/brick.wav", self.get_pos());

  if (self.m_sprite->get_action() == "empty")
    return;

  // First what's below the bonus block, if solid send it up anyway (excepting doll)
  Rectf dest_;
  dest_.set_left(bbox.get_left() + 1);
  dest_.set_top(bbox.get_bottom() + 1);
  dest_.set_right(bbox.get_right() - 1);
  dest_.set_bottom(dest_.get_top() + 30);

  if (!Sector::get().is_free_of_statics(dest_, &self, true) && !(block.contents == BonusBlock::Content::ONEUP))
  {
    bonus_block::try_open(self, player);
    return;
  }

  if (player == nullptr)
    player = Sector::get().get_nearest_player(bbox);

  if (player == nullptr)
    return;

  Direction direction = bonus_direction(self, *player);

  bool countdown = false;
  bool play_upgrade_sound = false;

  switch (block.contents) {
    case BonusBlock::Content::COIN:
      bonus_block::try_open(self, player);
      break;

    case BonusBlock::Content::FIREGROW:
      bonus_drop_growup(self, player, "images/powerups/fireflower/fireflower.sprite", direction, countdown);
      break;

    case BonusBlock::Content::ICEGROW:
      bonus_drop_growup(self, player, "images/powerups/iceflower/iceflower.sprite", direction, countdown);
      break;

    case BonusBlock::Content::AIRGROW:
      bonus_drop_growup(self, player, "images/powerups/airflower/airflower.sprite", direction, countdown);
      break;

    case BonusBlock::Content::EARTHGROW:
      bonus_drop_growup(self, player, "images/powerups/earthflower/earthflower.sprite", direction, countdown);
      break;

    case BonusBlock::Content::STAR:
      Sector::get().add<Star>(self.get_pos() + Vector(0, 32), direction);
      play_upgrade_sound = true;
      countdown = true;
      break;

    case BonusBlock::Content::ONEUP:
      Sector::get().add<OneUp>(self.get_pos(), Direction::DOWN);
      play_upgrade_sound = true;
      countdown = true;
      break;

    case BonusBlock::Content::CUSTOM:
      //NOTE: non-portable trampolines could be moved to Content::CUSTOM, but they should not drop
      block.object.value->set_pos(self.get_pos() + Vector(0, 32));
      Sector::get().add_object(std::move(block.object.value));
      play_upgrade_sound = true;
      countdown = true;
      break;

    case BonusBlock::Content::SCRIPT:
      countdown = true;
      break; // because scripts always run, this prevents default contents from being assumed

    case BonusBlock::Content::LIGHT:
    case BonusBlock::Content::LIGHT_ON:
    case BonusBlock::Content::TRAMPOLINE:
    case BonusBlock::Content::RAIN:
      bonus_block::try_open(self, player);
      break;

    case BonusBlock::Content::EXPLODE:
      Sector::get().add<CoinExplode>(self.get_pos() + Vector (0, 40));
      play_upgrade_sound = true;
      countdown = true;
      break;
  }

  if (play_upgrade_sound)
    SoundManager::current()->play("sounds/upgrade.wav", self.get_pos(), upgrade_sound_gain);

  if (!block.script.empty()) { // scripts always run if defined
    Sector::get().run_script(block.script, "powerup-script");
  }

  if (countdown) { // only decrease hit counter if try_open was not called
    if (block.hit_counter == 1) {
      self.m_sprite->set_action("empty");
    } else {
      block.hit_counter--;
    }
  }
}

void bonus_hit(ArchetypeObject& self, Player& player)
{
  bonus_block::try_open(self, &player);
}

HitResponse bonus_collision(ArchetypeObject& self, GameObject& other, CollisionHit const& hit)
{
  Rectf const& bbox = self.m_col.m_bbox;

  auto player = dynamic_cast<Player*> (&other);
  if (player) {
    if (player->m_does_buttjump ||
        (player->is_swimboosting() && player->get_bbox().get_bottom() < bbox.get_top() + SHIFT_DELTA))
    {
      bonus_try_drop(self, player);
    }
  }

  auto badguy = dynamic_cast<BadGuy*> (&other);
  if (badguy) {
    // hit contains no information for collisions with blocks.
    // Badguy's bottom has to be below the top of the block
    // SHIFT_DELTA is required to slide over one tile gaps.
    if (badguy->can_break() && (badguy->get_bbox().get_bottom() > bbox.get_top() + SHIFT_DELTA)) {
      bonus_block::try_open(self, player);
    }
  }

  if (dynamic_cast<Crusher*> (&other))
  {
    bonus_block::try_open(self, player);
  }

  auto portable = dynamic_cast<Portable*> (&other);
  if (portable && !badguy) {
    auto moving = dynamic_cast<MovingObject*> (&other);
    if (moving->get_bbox().get_top() > bbox.get_bottom() - SHIFT_DELTA) {
      bonus_block::try_open(self, player);
    }
  }

  return block_collision(self, other, hit);
}

void bonus_draw(ArchetypeObject& self, DrawingContext& context)
{
  BonusBlock const& block = ecs::get<BonusBlock>(self.get_entity());

  // do the regular drawing first
  block_draw(self, context);

  // then Draw the light if on.
  if (self.m_sprite->get_action() == "on") {
    Vector pos = self.get_pos() + (self.m_col.m_bbox.get_size().as_vector() -
                                   Vector(static_cast<float>(block.lightsprite->get_width()),
                                          static_cast<float>(block.lightsprite->get_height()))) / 2.0f;
    context.light().draw_surface(block.lightsprite, pos, 10);
  }
}

// Brick --------------------------------------------------------------

void brick_construct(ArchetypeObject& self)
{
  Brick& brick = ecs::get<Brick>(self.get_entity());
  if (!brick.breakable) {
    brick.coin_counter = 5;
  }
}

void heavy_brick_ricochet(ArchetypeObject& self, GameObject* collider)
{
  SoundManager::current()->play("sounds/metal_hit.ogg", self.get_pos());
  block::start_bounce(self, collider);
}

void brick_hit(ArchetypeObject& self, Player& player)
{
  if (ecs::get<Brick>(self.get_entity()).heavy) {
    heavy_brick_ricochet(self, &player);
    return;
  }

  if (self.m_sprite->get_action() == "empty")
    return;

  brick::try_break(self, &player);
}

HitResponse heavy_brick_collision(ArchetypeObject& self, GameObject& other, CollisionHit const& hit)
{
  Rectf const& bbox = self.m_col.m_bbox;

  auto player = dynamic_cast<Player*>(&other);
  if (player)
  {
    if (player->is_stone() && player->get_velocity().y >= 280)
      brick::try_break(self, player);
    else if (player->m_does_buttjump)
      heavy_brick_ricochet(self, &other);
  }

  auto crusher = dynamic_cast<Crusher*> (&other);
  if (crusher)
  {
    if (crusher->is_big())
      brick::try_break(self, nullptr);
    else
      heavy_brick_ricochet(self, &other);
  }

  auto badguy = dynamic_cast<BadGuy*> (&other);
  if (badguy && badguy->can_break() && (badguy->get_bbox().get_bottom() > bbox.get_top() + SHIFT_DELTA))
    heavy_brick_ricochet(self, &other);

  auto portable = dynamic_cast<Portable*> (&other);
  if (portable)
  {
    auto moving = dynamic_cast<MovingObject*> (&other);
    if (moving->get_bbox().get_top() > bbox.get_bottom() - SHIFT_DELTA)
      heavy_brick_ricochet(self, &other);
  }

  return block_collision(self, other, hit);
}

HitResponse brick_collision(ArchetypeObject& self, GameObject& other, CollisionHit const& hit)
{
  Brick const& brick = ecs::get<Brick>(self.get_entity());
  if (brick.heavy) {
    return heavy_brick_collision(self, other, hit);
  }

  Rectf const& bbox = self.m_col.m_bbox;

  auto player = dynamic_cast<Player*> (&other);
  if (player) {
    if (player->m_does_buttjump) brick::try_break(self, player);
    if (player->is_stone() && player->get_velocity().y >= 280) brick::try_break(self, player); // stoneform breaks through bricks
  }

  auto badguy = dynamic_cast<BadGuy*> (&other);
  if (badguy) {
    // hit contains no information for collisions with blocks.
    // Badguy's bottom has to be below the top of the brick
    // SHIFT_DELTA is required to slide over one tile gaps.
    if (badguy->can_break() && (badguy->get_bbox().get_bottom() > bbox.get_top() + SHIFT_DELTA)) {
      brick::try_break(self, nullptr);
    }
  }

  auto portable = dynamic_cast<Portable*> (&other);
  if (portable && !badguy) {
    auto moving = dynamic_cast<MovingObject*> (&other);
    if (moving->get_bbox().get_top() > bbox.get_bottom() - SHIFT_DELTA) {
      brick::try_break(self, nullptr);
    }
  }

  auto explosion = dynamic_cast<Explosion*> (&other);
  if (explosion && explosion->hurts()) {
    brick::try_break(self, nullptr);
  }

  auto crusher = dynamic_cast<Crusher*> (&other);
  if (crusher && brick.coin_counter == 0)
    brick::try_break(self, nullptr);

  return block_collision(self, other, hit);
}

// InvisibleBlock -----------------------------------------------------

void invisible_construct(ArchetypeObject& /*self*/)
{
  SoundManager::current()->preload("sounds/brick.wav");
}

void invisible_draw(ArchetypeObject& self, DrawingContext& context)
{
  if (ecs::get<InvisibleBlock>(self.get_entity()).visible)
    self.m_sprite->draw(context.color(), self.get_pos(), LAYER_OBJECTS);
}

bool invisible_collides(ArchetypeObject const& self, GameObject& other, CollisionHit const& /*hit*/)
{
  if (ecs::get<InvisibleBlock>(self.get_entity()).visible)
    return true;

  // if we're not visible, only register a collision if this will make us visible
  auto player = dynamic_cast<Player*> (&other);
  if ((player)
      && (player->get_movement().y <= 0)
      && (player->get_bbox().get_top() > self.get_bbox().get_bottom() - SHIFT_DELTA)) {
    return true;
  }

  return false;
}

void invisible_hit(ArchetypeObject& self, Player& player)
{
  InvisibleBlock& block = ecs::get<InvisibleBlock>(self.get_entity());

  SoundManager::current()->play("sounds/brick.wav", self.get_pos());

  if (block.visible)
    return;

  self.m_sprite->set_action("empty");
  block::start_bounce(self, &player);
  self.set_group(COLGROUP_STATIC);
  block.visible = true;
}

// InfoBlock ----------------------------------------------------------

void info_read(ArchetypeObject& self, ReaderMapping const& /*mapping*/)
{
  InfoBlock& block = ecs::get<InfoBlock>(self.get_entity());
  if (block.message.empty())
  {
    log_warning("No message in InfoBlock");
  }

  // Split text string lines into a vector
  block.lines.value = InfoBoxLine::split(block.message, 400);
  for (auto const& line : block.lines.value) block.lines_height += line->get_height();
}

void info_hit(ArchetypeObject& self, Player& player)
{
  InfoBlock& block = ecs::get<InfoBlock>(self.get_entity());

  block::start_bounce(self, &player);

  if (block.dest_pct != 1) {

    // first hide all other InfoBlocks' messages in same sector
    for (auto& other : Sector::get().get_objects_by_type<ArchetypeObject>())
    {
      if (&other != &self)
      {
        if (auto* other_block = ecs::try_get<InfoBlock>(other.get_entity())) {
          other_block->dest_pct = 0;
        }
      }
    }

    float const original_y = ecs::get<Block>(self.get_entity()).original_y;
    Camera& cam = Sector::get().get_singleton_by_type<Camera>();
    if (original_y - block.lines_height - 10.f < cam.get_translation().y)
      block.initial_y = cam.get_translation().y + 10.0f;
    else
      block.initial_y = original_y - block.lines_height - 10.f;

    block.dest_pct = 1;
  } else {
    block.dest_pct = 0;
  }
}

HitResponse info_collision(ArchetypeObject& self, GameObject& other, CollisionHit const& hit)
{
  auto player = dynamic_cast<Player*> (&other);
  if (player)
  {
    if (player->m_does_buttjump)
      info_hit(self, *player);
  }
  return block_collision(self, other, hit);
}

void info_update(ArchetypeObject& self, float dt_sec)
{
  InfoBlock& block = ecs::get<InfoBlock>(self.get_entity());

  if (dt_sec == 0) return;

  // hide message if player is too far away
  if (block.dest_pct > 0) {
    if (auto* player = Sector::get().get_nearest_player(self.m_col.m_bbox)) {
      Vector p1 = self.m_col.m_bbox.get_middle();
      Vector p2 = player->get_bbox().get_middle();
      Vector dist = (p2 - p1);
      float d = glm::length(dist);
      if (d > 128) block.dest_pct = 0;
    }
  }

  // handle soft fade-in and fade-out
  if (block.shown_pct != block.dest_pct) {
    float transitionspeed = block.fadetransition ? 1.f : 2.5f;
    if (block.dest_pct > block.shown_pct) block.shown_pct = std::min(block.shown_pct + 2 * dt_sec * transitionspeed, block.dest_pct);
    if (block.dest_pct < block.shown_pct) block.shown_pct = std::max(block.shown_pct - 2 * dt_sec * transitionspeed, block.dest_pct);
  }
}

void info_draw(ArchetypeObject& self, DrawingContext& context)
{
  InfoBlock const& block = ecs::get<InfoBlock>(self.get_entity());
  Rectf const& bbox = self.m_col.m_bbox;

  block_draw(self, context);

  if (block.shown_pct <= 0) return;

  context.push_transform();
  if (block.fadetransition)
    context.set_alpha(block.shown_pct);

  float border = 8;
  float width = 400; // this is the text width only
  float height = block.lines_height; // this is the text height only
  float x1 = (bbox.get_left() + bbox.get_right())/2 - width/2;
  float x2 = (bbox.get_left() + bbox.get_right())/2 + width/2;
  float y1 = block.initial_y;

  if (x1 < 0) {
    x1 = 0;
    x2 = width;
  }

  if (x2 > Sector::get().get_width()) {
    x2 = Sector::get().get_width();
    x1 = x2 - width;
  }

  float growposx = (x1 - border) + (((width + 2 * border) / 2) - (((width + 2 * border) / 2) * block.shown_pct));
  float growposy = (y1 - border) + ((height + 2 * border - 4) / 2) - (((height + 2 * border - 4) / 2) * block.shown_pct);

  // lines_height includes one ITEMS_SPACE too much, so the bottom border is reduced by 4px
  context.color().draw_filled_rect(Rectf(Vector(block.fadetransition ? x1 - border : growposx,
                                                block.fadetransition ? y1 - border : growposy),
                                         Sizef(width + 2 * border, height + 2 * border - 4) * (block.fadetransition ? 1.f : block.shown_pct)),
                                   block.frontcolor, block.roundness, LAYER_GUI - 50);

  context.color().draw_filled_rect(Rectf(Vector((block.fadetransition ? x1 - border : growposx) - 4.f,
                                                (block.fadetransition ? y1 - border : growposy) - 4.f),
                                         Sizef(8.f, 8.f) + (Sizef((width + 2 * border), (height + 2 * border - 4)) * (block.fadetransition ? 1.f : block.shown_pct))),
                                   block.backcolor, block.roundness + 4.f, LAYER_GUI - 51);

  float y = y1;
  for (size_t i = 0; i < block.lines.value.size(); ++i) {
    if (y >= y1 + height) {
      break;
    }

    if (block.fadetransition || block.shown_pct >= 1.f)
    {
      block.lines.value[i]->draw(context, Rectf(x1, y, x2, y), LAYER_GUI - 50 + 1);
      y += block.lines.value[i]->get_height();
    }
  }

  context.pop_transform();
}

// PathFollower -------------------------------------------------------

void path_read(ArchetypeObject& self, ReaderMapping const& mapping)
{
  PathFollower& follower = ecs::get<PathFollower>(self.get_entity());
  follower.path.value = std::make_unique<PathObject>();
  follower.path.value->init_path(mapping, follower.running);
}

void path_move_to(ArchetypeObject& self, Vector const& pos)
{
  Vector shift = pos - self.m_col.m_bbox.p1();
  if (PathObject* path = path_follower::get(self); path && path->get_path()) {
    path->get_path()->move_by(shift);
  }
  self.set_pos(pos);
}

// Platform -----------------------------------------------------------

void platform_read(ArchetypeObject& self, ReaderMapping const& mapping)
{
  Platform& platform = ecs::get<Platform>(self.get_entity());

  bool running = true;
  mapping.read("running", running);
  if ((self.get_name().empty()) && (!running)) {
    platform.automatic = true;
  }
  platform.starting_node = static_cast<int>(mapping.get("starting-node", 0.f));
}

void platform_finish_construction(ArchetypeObject& self)
{
  Platform& platform = ecs::get<Platform>(self.get_entity());
  PathObject& path = *path_follower::get(self);

  if (!path.get_path())
  {
    // If no path is given, make a one-node dummy path
    path.init_path_pos(self.m_col.m_bbox.p1(), false);
  }

  if (platform.starting_node >= static_cast<int>(path.get_path()->get_nodes().size()))
    platform.starting_node = static_cast<int>(path.get_path()->get_nodes().size()) - 1;

  path.get_walker()->jump_to_node(platform.starting_node);

  self.m_col.m_bbox.set_pos(path.get_path_handle().get_pos(self.m_col.m_bbox.get_size(),
                                                           path.get_path()->get_nodes()[platform.starting_node].position));
}

HitResponse platform_collision(ArchetypeObject& self, GameObject& other, CollisionHit const& /*hit*/)
{
  if (dynamic_cast<Player*>(&other)) {
    ecs::get<Platform>(self.get_entity()).player_contact = true;
  }
  return FORCE_MOVE;
}

void platform_update(ArchetypeObject& self, float dt_sec)
{
  Platform& platform = ecs::get<Platform>(self.get_entity());
  PathObject& path = *path_follower::get(self);

  if (!path.get_path()) return;
  if (!path.get_path()->is_valid()) return;

  // check if Platform should automatically pick a destination
  if (platform.automatic)
  {
    if (!platform.player_contact && !path.get_walker()->is_running()) {
      // Player doesn't touch platform and Platform is not moving

      // Travel to node nearest to nearest player
      if (auto* player = Sector::get().get_nearest_player(self.m_col.m_bbox)) {
        int nearest_node_id = path.get_path()->get_nearest_node_no(player->get_bbox().p2());
        if (nearest_node_id != -1) {
          platform::goto_node(self, nearest_node_id);
        }
      }
    }

    if (platform.player_contact && !platform.last_player_contact && !path.get_walker()->is_running()) {
      // Player touched platform, didn't touch last frame and Platform is not moving

      // Travel to node farthest from current position
      int farthest_node_id = path.get_path()->get_farthest_node_no(self.get_pos());
      if (farthest_node_id != -1) {
        platform::goto_node(self, farthest_node_id);
      }
    }

    // Clear player_contact flag set by collision() method
    platform.last_player_contact = platform.player_contact;
    platform.player_contact = false;
  }

  path.get_walker()->update(dt_sec);
  Vector movement = path.get_walker()->get_pos(self.m_col.m_bbox.get_size(), path.get_path_handle()) - self.get_pos();
  self.m_col.set_movement(movement);
  self.m_col.propagate_movement(movement);
  platform.speed = movement / dt_sec;
}

void platform_expose(ArchetypeObject& self, HSQUIRRELVM vm, SQInteger table_idx)
{
  if (self.get_name().empty())
    return;
  expose_object(vm, table_idx, std::make_unique<scripting::Platform>(self.get_uid()), self.get_name());
}

void platform_unexpose(ArchetypeObject& self, HSQUIRRELVM vm, SQInteger table_idx)
{
  if (self.get_name().empty())
    return;
  unexpose_object(vm, table_idx, self.get_name());
}

// Hurting ------------------------------------------------------------

HitResponse hurting_collision(ArchetypeObject& /*self*/, GameObject& other, CollisionHit const& /*hit*/)
{
  auto player = dynamic_cast<Player*>(&other);
  if (player) {
    if (player->is_invincible()) {
      return ABORT_MOVE;
    }
    player->kill(false);
  }

  auto badguy = dynamic_cast<BadGuy*>(&other);
  if (badguy) {
    badguy->kill_fall();
  }

  return FORCE_MOVE;
}

// Coin ---------------------------------------------------------------

void coin_construct(ArchetypeObject& /*self*/)
{
  SoundManager::current()->preload("sounds/coin.wav");
}

void coin_finish_construction(ArchetypeObject& self)
{
  Coin& coin = ecs::get<Coin>(self.get_entity());
  PathObject* path = path_follower::get(self);
  if (!path)
    return;

  if (path->get_path())
  {
    if (coin.starting_node >= static_cast<int>(path->get_path()->get_nodes().size()))
      coin.starting_node = static_cast<int>(path->get_path()->get_nodes().size()) - 1;

    self.set_pos(path->get_path_handle().get_pos(self.m_col.m_bbox.get_size(),
                                                 path->get_path()->get_nodes()[coin.starting_node].position));
    path->get_walker()->jump_to_node(coin.starting_node);
  }
}

void coin_update(ArchetypeObject& self, float dt_sec)
{
  // if we have a path to follow, follow it
  PathObject* path = path_follower::get(self);
  if (path && path->get_walker()) {
    path->get_walker()->update(dt_sec);
    Vector v = path->get_walker()->get_pos(self.m_col.m_bbox.get_size(), path->get_path_handle());

    if (path->get_path()->is_valid()) {
      self.m_col.set_movement(v - self.get_pos());
    }
  }
}

HitResponse coin_collision(ArchetypeObject& self, GameObject& other, CollisionHit const& /*hit*/)
{
  auto player = dynamic_cast<Player*>(&other);
  if (player == nullptr)
    return ABORT_MOVE;

  if (self.m_col.get_bbox().contains(player->get_bbox().grown(-0.1f)))
    coin::collect(self);

  return ABORT_MOVE;
}

// HeavyCoin ----------------------------------------------------------

void heavy_coin_construct(ArchetypeObject& self)
{
  Physic& physic = ecs::emplace<Physic>(self.get_entity());
  physic.enable_gravity(true);
  SoundManager::current()->preload("sounds/coin2.ogg");
}

void heavy_coin_update(ArchetypeObject& self, float dt_sec)
{
  // enable physics
  self.m_col.set_movement(ecs::get<Physic>(self.get_entity()).get_movement(dt_sec));
}

void heavy_coin_collision_solid(ArchetypeObject& self, CollisionHit const& hit)
{
  HeavyCoin& coin = ecs::get<HeavyCoin>(self.get_entity());
  Physic& physic = ecs::get<Physic>(self.get_entity());

  float clink_threshold = 100.0f; // sets the minimum speed needed to result in collision noise
  //TODO: colliding HeavyCoins should have their own unique sound

  if (hit.bottom) {
    if (physic.get_velocity_y() > clink_threshold && !coin.last_hit.bottom)
      SoundManager::current()->play("sounds/coin2.ogg", self.get_pos());
    if (physic.get_velocity_y() > 200) {// lets some coins bounce
      physic.set_velocity_y(-99);
    } else {
      physic.set_velocity_y(0);
      physic.set_velocity_x(0);
    }
  }
  if (hit.right || hit.left) {
    if ((physic.get_velocity_x() > clink_threshold ||
         physic.get_velocity_x() < -clink_threshold) &&
        hit.right != coin.last_hit.right && hit.left != coin.last_hit.left)
      SoundManager::current()->play("sounds/coin2.ogg", self.get_pos());
    physic.set_velocity_x(-physic.get_velocity_x());
  }
  if (hit.top) {
    if (physic.get_velocity_y() < -clink_threshold && !coin.last_hit.top)
      SoundManager::current()->play("sounds/coin2.ogg", self.get_pos());
    physic.set_velocity_y(-physic.get_velocity_y());
  }

  // Only make a sound if the coin wasn't hittin anything last frame (A coin
  // stuck in solid matter would flood the sound manager - see #1555 on GitHub)
  coin.last_hit = hit;
}

// Rock ---------------------------------------------------------------

const std::string ROCK_SOUND = "sounds/brick.wav"; //TODO use own sound.
constexpr float GROUND_FRICTION = 0.1f; // Amount of friction to apply while on ground.

PortableObject& as_portable(ArchetypeObject& self)
{
  return dynamic_cast<PortableObject&>(self);
}

PortableObject const& as_portable(ArchetypeObject const& self)
{
  return dynamic_cast<PortableObject const&>(self);
}

void rock_construct(ArchetypeObject& self)
{
  ecs::emplace<Physic>(self.get_entity());
  SoundManager::current()->preload(ROCK_SOUND);
}

void rock_update(ArchetypeObject& self, float dt_sec)
{
  if (!as_portable(self).is_grabbed())
    self.m_col.set_movement(ecs::get<Physic>(self.get_entity()).get_movement(dt_sec));
}

void rock_collision_solid(ArchetypeObject& self, CollisionHit const& hit)
{
  Rock& rock = ecs::get<Rock>(self.get_entity());
  Physic& physic = ecs::get<Physic>(self.get_entity());

  if (as_portable(self).is_grabbed()) {
    return;
  }
  if (hit.top || hit.bottom)
    physic.set_velocity_y(0);
  if (hit.left || hit.right) {
    // Bounce back slightly when hitting a wall
    float velx = physic.get_velocity_x();
    physic.set_velocity_x(-0.1f * velx);
  }
  if (hit.crush)
    physic.set_velocity(0, 0);

  if (hit.bottom && !rock.on_ground && !as_portable(self).is_grabbed()) {
    SoundManager::current()->play(ROCK_SOUND, self.get_pos());
    physic.set_velocity_x(0);
    rock.on_ground = true;
  }

  if (rock.on_ground) {
    // Full friction!
    physic.set_velocity_x(physic.get_velocity_x() * (1.f - GROUND_FRICTION));
  }
}

HitResponse rock_collision(ArchetypeObject& self, GameObject& other, CollisionHit const& hit)
{
  Rock& rock = ecs::get<Rock>(self.get_entity());
  Physic& physic = ecs::get<Physic>(self.get_entity());

  if (ecs::try_get<HeavyCoin>(other.get_entity())) {
    return ABORT_MOVE;
  }

  if (dynamic_cast<Explosion*>(&other)) {
    return ABORT_MOVE;
  }

  // Why is it necessary to list exceptions here? Why doesn't the rock just not
  // affect object that have ABORT_MOVE on all collisions?
  if (dynamic_cast<LitObject*>(&other)) {
    return ABORT_MOVE;
  }

  if (dynamic_cast<PushButton*>(&other)) {
    return ABORT_MOVE;
  }

  if (as_portable(self).is_grabbed()) {
    return ABORT_MOVE;
  }

  auto crusher = dynamic_cast<Crusher*>(&other);
  if (crusher) {
    auto state = crusher->get_state();
    if (state == Crusher::CrusherState::RECOVERING ||
        state == Crusher::CrusherState::IDLE) {
      return ABORT_MOVE;
    }
  }

  // Don't fall further if we are on a rock which is on the ground.
  // This is to avoid jittering.
  auto* other_rock = ecs::try_get<Rock>(other.get_entity());
  if (other_rock && other_rock->on_ground && hit.bottom) {
    physic.set_velocity_y(0);
    return CONTINUE;
  }

  if (!rock.on_ground) {
    if (hit.bottom && physic.get_velocity_y() > 200) {
      auto badguy = dynamic_cast<BadGuy*>(&other);
      auto player = dynamic_cast<Player*>(&other);
      if (badguy && badguy->get_group() != COLGROUP_TOUCHABLE) {
        //Getting a rock on the head hurts. A lot.
        badguy->kill_fall();
        physic.set_velocity_y(0);
      }
      else if (player)
      {
        player->kill(false);
        physic.set_velocity_y(0);
      }
    }
    return FORCE_MOVE;
  }

  return FORCE_MOVE;
}

void rock_grab(ArchetypeObject& self, MovingObject& object, Vector const& pos, Direction dir)
{
  Rock& rock = ecs::get<Rock>(self.get_entity());

  as_portable(self).default_grab(object, pos, dir);
  Vector movement = pos - self.get_pos();
  self.m_col.set_movement(movement);
  rock.last_movement = movement;
  self.set_group(COLGROUP_TOUCHABLE); //needed for lanterns catching willowisps
  rock.on_ground = false;

  if (!rock.on_grab_script.empty()) {
    Sector::get().run_script(rock.on_grab_script, "Rock::on_grab");
  }
}

void rock_ungrab(ArchetypeObject& self, MovingObject& object, Direction dir)
{
  Rock& rock = ecs::get<Rock>(self.get_entity());
  Physic& physic = ecs::get<Physic>(self.get_entity());

  auto player = dynamic_cast<Player*>(&object);
  self.set_group(COLGROUP_MOVING_STATIC);
  rock.on_ground = false;
  if (player)
  {
    if (player->is_swimming() || player->is_water_jumping())
    {
      float swimangle = player->get_swimming_angle();
      physic.set_velocity(player->get_velocity() + Vector(std::cos(swimangle), std::sin(swimangle)));
    }
    else
    {
      physic.set_velocity_x(fabsf(player->get_physic().get_velocity_x()) < 1.f ? 0.f :
                            player->m_dir == Direction::LEFT ? -200.f : 200.f);
      physic.set_velocity_y((dir == Direction::UP) ? -500.f : (dir == Direction::DOWN) ? 500.f :
                            (glm::length(rock.last_movement) > 1) ? -200.f : 0.f);
    }
  }

  if (!rock.on_ungrab_script.empty())
  {
    Sector::get().run_script(rock.on_ungrab_script, "Rock::on_ungrab");
  }
  as_portable(self).default_ungrab(object, dir);
}

void rock_expose(ArchetypeObject& self, HSQUIRRELVM vm, SQInteger table_idx)
{
  if (self.get_name().empty())
    return;
  expose_object(vm, table_idx, std::make_unique<scripting::Rock>(self.get_uid()), self.get_name());
}

void rock_unexpose(ArchetypeObject& self, HSQUIRRELVM vm, SQInteger table_idx)
{
  if (self.get_name().empty())
    return;
  unexpose_object(vm, table_idx, self.get_name());
}

// Trampoline ---------------------------------------------------------

/* Trampoline will accelerate Tux to to VY_INITIAL, if
 * he jumps on it to VY_MIN. */
const std::string TRAMPOLINE_SOUND = "sounds/trampoline.wav";
constexpr float VY_MIN = -900; //negative, upwards
constexpr float VY_INITIAL = -500;

void trampoline_read(ArchetypeObject& self, ReaderMapping const& mapping)
{
  Trampoline const& trampoline = ecs::get<Trampoline>(self.get_entity());

  //Check if this trampoline is not portable
  std::string sprite;
  if (!trampoline.portable && !mapping.read("sprite", sprite)) {
    //we need another sprite
    self.m_sprite_name = trampoline.fixed_sprite;
    self.m_default_sprite_name = self.m_sprite_name;
    self.m_sprite = SpriteManager::current()->create(self.m_sprite_name);
    self.m_sprite->set_action("normal");
  }
}

void trampoline_construct(ArchetypeObject& /*self*/)
{
  SoundManager::current()->preload(TRAMPOLINE_SOUND);
}

void trampoline_update(ArchetypeObject& self, float /*dt_sec*/)
{
  if (self.m_sprite->animation_done()) {
    self.m_sprite->set_action("normal");
  }
}

HitResponse trampoline_collision(ArchetypeObject& self, GameObject& other, CollisionHit const& hit)
{
  if (ecs::try_get<HeavyCoin>(other.get_entity())) {
    return ABORT_MOVE;
  }

  //Tramponine has to be on ground to work.
  if (ecs::get<Rock>(self.get_entity()).on_ground) {
    auto player = dynamic_cast<Player*>(&other);
    //Trampoline works for player
    if (player) {
      player->override_velocity();
      if (player->m_does_buttjump)
        player->m_does_buttjump = false;
      float vy = player->get_physic().get_velocity_y();
      //player is falling down on trampoline
      if (hit.top && vy >= 0) {
        if (!(player->get_status().bonus[player->get_id()] == AIR_BONUS))
        {
          if (player->get_controller().hold(Control::JUMP))
            vy = VY_MIN;
          else if (player->get_controller().hold(Control::DOWN))
            vy = VY_MIN + 100;
          else
            vy = VY_INITIAL;
        }
        else
        {
          if (player->get_controller().hold(Control::JUMP))
            vy = VY_MIN - 80;
          else if (player->get_controller().hold(Control::DOWN))
            vy = VY_MIN - 70;
          else
            vy = VY_INITIAL - 40;
        }
        player->get_physic().set_velocity_y(vy);
        SoundManager::current()->play(TRAMPOLINE_SOUND, self.get_pos());
        self.m_sprite->set_action("swinging", 1);
        return FORCE_MOVE;
      }
    }

    //Trampoline also works for walking badguys
    if (ecs::try_get<Walker>(other.get_entity())) {
      Physic& physic = ecs::get<Physic>(other.get_entity());
      //walking badguy is falling down on trampoline
      if (hit.top && physic.get_velocity_y() >= 0) {
        physic.set_velocity_y(VY_INITIAL);
        SoundManager::current()->play(TRAMPOLINE_SOUND, self.get_pos());
        self.m_sprite->set_action("swinging", 1);
        return FORCE_MOVE;
      }
    }
  }

  return rock_collision(self, other, hit);
}

void trampoline_grab(ArchetypeObject& self, MovingObject& object, Vector const& pos, Direction dir)
{
  self.m_sprite->set_animation_loops(0);
  rock_grab(self, object, pos, dir);
}

bool trampoline_is_portable(ArchetypeObject const& self)
{
  return as_portable(self).Portable::is_portable() && ecs::get<Trampoline>(self.get_entity()).portable;
}

// RustyTrampoline ----------------------------------------------------

/* Trampoline will accelerate Tux to to VY_BOUNCE, if
 * he jumps on it to VY_TRIGGER. */
constexpr float VY_TRIGGER = -900; //negative, upwards
constexpr float VY_BOUNCE = -500;

void rusty_construct(ArchetypeObject& /*self*/)
{
  SoundManager::current()->preload(TRAMPOLINE_SOUND);
}

void rusty_update(ArchetypeObject& self, float /*dt_sec*/)
{
  if (self.m_sprite->animation_done()) {
    if (ecs::get<RustyTrampoline>(self.get_entity()).counter < 1) {
      self.remove_me();
    } else {
      self.m_sprite->set_action("normal");
    }
  }
}

void rusty_bounced(ArchetypeObject& self, RustyTrampoline& trampoline)
{
  SoundManager::current()->play(TRAMPOLINE_SOUND, self.get_pos());
  trampoline.counter--;
  if (trampoline.counter > 0) {
    self.m_sprite->set_action("swinging", 1);
  } else {
    self.m_sprite->set_action("breaking", 1);
  }
}

HitResponse rusty_collision(ArchetypeObject& self, GameObject& other, CollisionHit const& hit)
{
  RustyTrampoline& trampoline = ecs::get<RustyTrampoline>(self.get_entity());

  //Trampoline has to be on ground to work.
  if (ecs::get<Rock>(self.get_entity()).on_ground) {
    auto player = dynamic_cast<Player*>(&other);
    //Trampoline works for player
    if (player) {
      float vy = player->get_physic().get_velocity_y();
      //player is falling down on trampoline
      if (hit.top && vy >= 0) {
        if (player->get_controller().hold(Control::JUMP)) {
          vy = VY_TRIGGER;
        } else {
          vy = VY_BOUNCE;
        }
        player->get_physic().set_velocity_y(vy);
        rusty_bounced(self, trampoline);
        return FORCE_MOVE;
      }
    }

    //Trampoline also works for walking badguys
    if (ecs::try_get<Walker>(other.get_entity())) {
      Physic& physic = ecs::get<Physic>(other.get_entity());
      //walking badguy is falling down on trampoline
      if (hit.top && physic.get_velocity_y() >= 0) {
        physic.set_velocity_y(VY_BOUNCE);
        rusty_bounced(self, trampoline);
        return FORCE_MOVE;
      }
    }
  }

  return rock_collision(self, other, hit);
}

void rusty_ungrab(ArchetypeObject& self, MovingObject& object, Direction dir)
{
  rock_ungrab(self, object, dir);
  self.m_sprite->set_action("breaking", 1);
  ecs::get<RustyTrampoline>(self.get_entity()).counter = 0; //remove in update()
}

bool rusty_is_portable(ArchetypeObject const& self)
{
  return as_portable(self).Portable::is_portable() && ecs::get<RustyTrampoline>(self.get_entity()).portable;
}

} // namespace

namespace rock {

void add_wind_velocity(ArchetypeObject& self, Vector const& velocity, Vector const& end_speed)
{
  Physic& physic = ecs::get<Physic>(self.get_entity());

  // only add velocity in the same direction as the wind
  if (end_speed.x > 0 && physic.get_velocity_x() < end_speed.x)
    physic.set_velocity_x(std::min(physic.get_velocity_x() + velocity.x, end_speed.x));
  if (end_speed.x < 0 && physic.get_velocity_x() > end_speed.x)
    physic.set_velocity_x(std::max(physic.get_velocity_x() + velocity.x, end_speed.x));
  if (end_speed.y > 0 && physic.get_velocity_y() < end_speed.y)
    physic.set_velocity_y(std::min(physic.get_velocity_y() + velocity.y, end_speed.y));
  if (end_speed.y < 0 && physic.get_velocity_y() > end_speed.y)
    physic.set_velocity_y(std::max(physic.get_velocity_y() + velocity.y, end_speed.y));
}

} // namespace rock

namespace trampoline {

std::unique_ptr<PortableObject> create(Vector const& pos, bool portable)
{
  auto object = PortableObject::create("trampoline", pos);
  ecs::get<Trampoline>(object->get_entity()).portable = portable;
  if (!portable) {
    object->m_sprite_name = ecs::get<Trampoline>(object->get_entity()).fixed_sprite;
    object->m_sprite = SpriteManager::current()->create(object->m_sprite_name);
    object->m_sprite->set_action("normal");
  }
  return object;
}

} // namespace trampoline

namespace coin {

void collect(ArchetypeObject& self)
{
  static Timer sound_timer;
  static int pitch_one = 128;
  static float last_pitch = 1;
  float pitch = 1;

  int tile = static_cast<int>(self.get_pos().y / 32);

  if (!sound_timer.started()) {
    pitch_one = tile;
    pitch = 1;
    last_pitch = 1;
  } else if (sound_timer.get_timegone() < 0.02f) {
    pitch = last_pitch;
  } else {
    switch ((pitch_one - tile) % 7) {
      case -6: pitch = 1.f/2; break;  // C
      case -5: pitch = 5.f/8; break;  // E
      case -4: pitch = 4.f/6; break;  // F
      case -3: pitch = 3.f/4; break;  // G
      case -2: pitch = 5.f/6; break;  // A
      case -1: pitch = 9.f/10; break; // Bb
      case 0: pitch = 1.f; break;     // c
      case 1: pitch = 9.f/8; break;   // d
      case 2: pitch = 5.f/4; break;   // e
      case 3: pitch = 4.f/3; break;   // f
      case 4: pitch = 3.f/2; break;   // g
      case 5: pitch = 5.f/3; break;   // a
      case 6: pitch = 9.f/5; break;   // bb
    }
    last_pitch = pitch;
  }
  sound_timer.start(1);

  std::unique_ptr<SoundSource> soundSource = SoundManager::current()->create_sound_source("sounds/coin.wav");
  soundSource->set_position(self.get_pos());
  soundSource->set_pitch(pitch);
  soundSource->play();
  SoundManager::current()->manage_source(std::move(soundSource));

  Sector::get().get_players()[0]->get_status().add_coins(1, false);
  Sector::get().add<BouncyCoin>(self.get_pos(), false, self.get_sprite_name());
  Sector::get().get_level().m_stats.increment_coins();
  self.remove_me();

  std::string const& collect_script = ecs::get<Coin>(self.get_entity()).collect_script;
  if (!collect_script.empty()) {
    Sector::get().run_script(collect_script, "collect-script");
  }
}

void spawn_heavy(Vector const& pos, Vector const& velocity)
{
  auto object = ArchetypeObject::create("heavycoin", pos);
  ecs::get<Physic>(object->get_entity()).set_velocity(velocity);
  Sector::get().add_object(std::move(object));
}

} // namespace coin

namespace path_follower {

PathObject* get(GameObject const& self)
{
  auto* follower = ecs::try_get<PathFollower>(self.get_entity());
  return follower ? follower->path.value.get() : nullptr;
}

} // namespace path_follower

namespace platform {

void goto_node(ArchetypeObject& self, int node_no)
{
  path_follower::get(self)->get_walker()->goto_node(node_no);
}

void start_moving(ArchetypeObject& self)
{
  path_follower::get(self)->get_walker()->start_moving();
}

void stop_moving(ArchetypeObject& self)
{
  path_follower::get(self)->get_walker()->stop_moving();
}

} // namespace platform

namespace block {

void start_bounce(ArchetypeObject& self, GameObject* hitter)
{
  Block& block = ecs::get<Block>(self.get_entity());
  Rectf const& bbox = self.m_col.m_bbox;

  if (block.original_y == -1) {
    block.original_y = bbox.get_top();
  }
  block.bouncing = true;
  block.bounce_dir = -BOUNCY_BRICK_SPEED;
  block.bounce_offset = 0;

  MovingObject* hitter_mo = dynamic_cast<MovingObject*>(hitter);
  if (hitter_mo) {
    float center_of_hitter = hitter_mo->get_bbox().get_middle().x;
    float offset = (bbox.get_middle().x - center_of_hitter)*2 / bbox.get_width();
    // Without this, hitting a multi-coin bonus block from the side (e. g. with
    // an ice block or a snail) would turn the block 90 degrees.
    if (offset > 2 || offset < -2)
      offset = 0;
    self.m_sprite->set_angle(BUMP_ROTATION_ANGLE*offset);
  }
}

} // namespace block

namespace bonus_block {

void try_open(ArchetypeObject& self, Player* player)
{
  BonusBlock& block = ecs::get<BonusBlock>(self.get_entity());

  SoundManager::current()->play("sounds/brick.wav", self.get_pos());
  if (self.m_sprite->get_action() == "empty")
    return;

  if (player == nullptr)
    player = Sector::get().get_nearest_player(self.m_col.m_bbox);
  if (player == nullptr)
    return;

  Direction direction = bonus_direction(self, *player);

  bool play_upgrade_sound = false;
  switch (block.contents) {
    case BonusBlock::Content::COIN:
      Sector::get().add<BouncyCoin>(self.get_pos(), true);
      SoundManager::current()->play("sounds/coin.wav", self.get_pos());
      player->get_status().add_coins(1, false);
      if (block.hit_counter != 0)
        Sector::get().get_level().m_stats.increment_coins();
      break;

    case BonusBlock::Content::FIREGROW:
      bonus_raise_growup(self, player, FIRE_BONUS, direction);
      break;

    case BonusBlock::Content::ICEGROW:
      bonus_raise_growup(self, player, ICE_BONUS, direction);
      break;

    case BonusBlock::Content::AIRGROW:
      bonus_raise_growup(self, player, AIR_BONUS, direction);
      break;

    case BonusBlock::Content::EARTHGROW:
      bonus_raise_growup(self, player, EARTH_BONUS, direction);
      break;

    case BonusBlock::Content::STAR:
      Sector::get().add<Star>(self.get_pos() + Vector(0, -32), direction);
      play_upgrade_sound = true;
      break;

    case BonusBlock::Content::ONEUP:
      Sector::get().add<OneUp>(self.get_pos(), direction);
      play_upgrade_sound = true;
      break;

    case BonusBlock::Content::CUSTOM:
      Sector::get().add<SpecialRiser>(self.get_pos(), std::move(block.object.value), true);
      play_upgrade_sound = true;
      break;

    case BonusBlock::Content::SCRIPT:
      break; // because scripts always run, this prevents default contents from being assumed

    case BonusBlock::Content::LIGHT:
    case BonusBlock::Content::LIGHT_ON:
      if (self.m_sprite->get_action() == "on")
        self.m_sprite->set_action("off");
      else
        self.m_sprite->set_action("on");
      SoundManager::current()->play("sounds/switch.ogg", self.get_pos());
      break;

    case BonusBlock::Content::TRAMPOLINE:
      Sector::get().add<SpecialRiser>(self.get_pos(), trampoline::create(self.get_pos(), false), true);
      play_upgrade_sound = true;
      break;

    case BonusBlock::Content::RAIN:
      Sector::get().add<CoinRain>(self.get_pos(), true);
      play_upgrade_sound = true;
      break;

    case BonusBlock::Content::EXPLODE:
      Sector::get().add<CoinExplode>(self.get_pos() + Vector (0, -40));
      play_upgrade_sound = true;
      break;
  }

  if (play_upgrade_sound)
    SoundManager::current()->play("sounds/upgrade.wav", self.get_pos(), upgrade_sound_gain);

  if (!block.script.empty()) { // scripts always run if defined
    Sector::get().run_script(block.script, "BonusBlockScript");
  }

  block::start_bounce(self, player);
  if (block.hit_counter <= 0 ||
      block.contents == BonusBlock::Content::LIGHT ||
      block.contents == BonusBlock::Content::LIGHT_ON) { //use 0 to allow infinite hits
  } else if (block.hit_counter == 1) {
    self.m_sprite->set_action("empty");
  } else {
    block.hit_counter--;
  }
}

} // namespace bonus_block

namespace brick {

void try_break(ArchetypeObject& self, Player* player)
{
  Brick& brick = ecs::get<Brick>(self.get_entity());

  if (self.m_sprite->get_action() == "empty")
    return;

  SoundManager::current()->play("sounds/brick.wav", self.get_pos());

  if (brick.coin_counter > 0) {
    Sector::get().add<BouncyCoin>(self.get_pos(), true);
    brick.coin_counter--;
    Player& player_one = *Sector::get().get_players()[0];
    player_one.get_status().add_coins(1);
    if (brick.coin_counter == 0)
      self.m_sprite->set_action("empty");
    block::start_bounce(self, player);
  } else if (brick.breakable) {
    if (player) {
      if (player->is_big()) {
        block_start_break(self, player);
        return;
      } else {
        block::start_bounce(self, player);
        return;
      }
    }
    block_break_me(self);
  }
}

void break_for_crusher(ArchetypeObject& self, Crusher& crusher)
{
  float shake_vel_x = crusher.is_sideways() ? crusher.get_physic().get_velocity_x() >= 0.f ? 6.f : -6.f : 0.f;
  float shake_vel_y = crusher.is_sideways() ? 0.f : 6.f;
  Sector::get().get_camera().shake(0.1f, shake_vel_x, shake_vel_y);
  try_break(self, nullptr);
  block_start_break(self, &crusher);
}

} // namespace brick

namespace weak_block {

void start_burning(ArchetypeObject& self)
{
  WeakBlock& block = ecs::get<WeakBlock>(self.get_entity());
  if (block.state != WeakBlock::State::NORMAL) return;

  block.state = WeakBlock::State::BURNING;
  self.m_sprite->set_action("burning", 1);

  // FIXME: Not hardcode these sounds?
  if (self.m_sprite_name == MELTBOX_SPRITE) {
    SoundManager::current()->play("sounds/sizzle.ogg", self.get_pos());
  } else if (self.m_sprite_name == STRAWBOX_SPRITE) {
    SoundManager::current()->play("sounds/fire.ogg", self.get_pos());
  }
}

} // namespace weak_block

template<>
ObjectBehavior const& object_behavior_of<UnstableTile>()
{
  static ObjectBehavior const behavior = {
    .construct = &unstable_construct,
    .update = &unstable_update,
    .draw = &unstable_draw,
    .collision = &unstable_collision,
  };
  return behavior;
}

template<>
ObjectBehavior const& object_behavior_of<MagicBlock>()
{
  static ObjectBehavior const behavior = {
    .construct = &magicblock_construct,
    .update = &magicblock_update,
    .draw = &magicblock_draw,
    .collides = &magicblock_collides,
  };
  return behavior;
}

template<>
ObjectBehavior const& object_behavior_of<ResetPoint>()
{
  static ObjectBehavior const behavior = {
    .construct = &reset_point_construct,
    .draw = &reset_point_draw,
    .collision = &reset_point_collision,
  };
  return behavior;
}

template<>
ObjectBehavior const& object_behavior_of<Block>()
{
  static ObjectBehavior const behavior = {
    .construct = &block_construct,
    .update = &block_update,
    .draw = &block_draw,
    .collision = &block_collision,
  };
  return behavior;
}

template<>
ObjectBehavior const& object_behavior_of<BonusBlock>()
{
  static ObjectBehavior const behavior = {
    .read = &bonus_read,
    .draw = &bonus_draw,
    .collision = &bonus_collision,
    .hit = &bonus_hit,
  };
  return behavior;
}

template<>
ObjectBehavior const& object_behavior_of<Brick>()
{
  static ObjectBehavior const behavior = {
    .construct = &brick_construct,
    .collision = &brick_collision,
    .hit = &brick_hit,
  };
  return behavior;
}

template<>
ObjectBehavior const& object_behavior_of<InvisibleBlock>()
{
  static ObjectBehavior const behavior = {
    .construct = &invisible_construct,
    .draw = &invisible_draw,
    .collides = &invisible_collides,
    .hit = &invisible_hit,
  };
  return behavior;
}

template<>
ObjectBehavior const& object_behavior_of<InfoBlock>()
{
  static ObjectBehavior const behavior = {
    .read = &info_read,
    .update = &info_update,
    .draw = &info_draw,
    .collision = &info_collision,
    .hit = &info_hit,
  };
  return behavior;
}

template<>
ObjectBehavior const& object_behavior_of<PathFollower>()
{
  static ObjectBehavior const behavior = {
    .read = &path_read,
    .move_to = &path_move_to,
  };
  return behavior;
}

template<>
ObjectBehavior const& object_behavior_of<Platform>()
{
  static ObjectBehavior const behavior = {
    .read = &platform_read,
    .finish_construction = &platform_finish_construction,
    .expose = &platform_expose,
    .unexpose = &platform_unexpose,
    .update = &platform_update,
    .collision = &platform_collision,
  };
  return behavior;
}

template<>
ObjectBehavior const& object_behavior_of<Hurting>()
{
  static ObjectBehavior const behavior = {
    .collision = &hurting_collision,
  };
  return behavior;
}

template<>
ObjectBehavior const& object_behavior_of<Coin>()
{
  static ObjectBehavior const behavior = {
    .construct = &coin_construct,
    .finish_construction = &coin_finish_construction,
    .update = &coin_update,
    .collision = &coin_collision,
  };
  return behavior;
}

template<>
ObjectBehavior const& object_behavior_of<HeavyCoin>()
{
  static ObjectBehavior const behavior = {
    .construct = &heavy_coin_construct,
    .update = &heavy_coin_update,
    .collision_solid = &heavy_coin_collision_solid,
  };
  return behavior;
}

template<>
ObjectBehavior const& object_behavior_of<Rock>()
{
  static ObjectBehavior const behavior = {
    .construct = &rock_construct,
    .expose = &rock_expose,
    .unexpose = &rock_unexpose,
    .update = &rock_update,
    .collision = &rock_collision,
    .collision_solid = &rock_collision_solid,
    .grab = &rock_grab,
    .ungrab = &rock_ungrab,
  };
  return behavior;
}

template<>
ObjectBehavior const& object_behavior_of<Trampoline>()
{
  static ObjectBehavior const behavior = {
    .read = &trampoline_read,
    .construct = &trampoline_construct,
    .update = &trampoline_update,
    .collision = &trampoline_collision,
    .is_portable = &trampoline_is_portable,
    .grab = &trampoline_grab,
  };
  return behavior;
}

template<>
ObjectBehavior const& object_behavior_of<RustyTrampoline>()
{
  static ObjectBehavior const behavior = {
    .construct = &rusty_construct,
    .update = &rusty_update,
    .collision = &rusty_collision,
    .is_portable = &rusty_is_portable,
    .ungrab = &rusty_ungrab,
  };
  return behavior;
}

template<>
ObjectBehavior const& object_behavior_of<WeakBlock>()
{
  static ObjectBehavior const behavior = {
    .construct = &weak_block_construct,
    .update = &weak_block_update,
    .draw = &weak_block_draw,
    .collision = &weak_block_collision,
  };
  return behavior;
}

/* EOF */
