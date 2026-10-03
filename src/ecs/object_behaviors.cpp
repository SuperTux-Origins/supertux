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
#include "math/random.hpp"
#include "object/archetype_object.hpp"
#include "object/bullet.hpp"
#include "object/explosion.hpp"
#include "object/player.hpp"
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

} // namespace

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
