//  SuperTux
//  Copyright (C) 2022 A. Semphris <semphris@protonmail.com>
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

#include "object/lit_object.hpp"

#include "sprite/sprite.hpp"
#include "sprite/sprite_manager.hpp"
#include "supertux/flip_level_transformer.hpp"
#include "util/reader_mapping.hpp"

LitObject::LitObject(const ReaderMapping& reader) :
  MovingObject(reader),
  ExposedObject<LitObject, scripting::LitObject>(this),
  m_light_offset(-6.f, -17.f),
  m_sprite_name("images/objects/lightflower/lightflower1.sprite"),
  m_light_sprite_name("images/objects/lightflower/light/glow_light.sprite"),
  m_sprite_action("default"),
  m_light_sprite_action("default"),
  m_sprite(),
  m_light_sprite(),
  m_layer(0),
  m_flip(NO_FLIP)
{
  reader.read("x", m_col.m_bbox.get_left());
  reader.read("y", m_col.m_bbox.get_top());

  reader.read("light-offset-x", m_light_offset.x);
  reader.read("light-offset-y", m_light_offset.y);

  reader.read("sprite", m_sprite_name);
  reader.read("light-sprite", m_light_sprite_name);
  m_layer = reader.get("layer", 0);

  reader.read("action", m_sprite_action);
  reader.read("light-action", m_light_sprite_action);

  m_sprite = SpriteManager::current()->create(m_sprite_name);
  m_light_sprite = SpriteManager::current()->create(m_light_sprite_name);
  m_light_sprite->set_blend(Blend::ADD);

  m_sprite->set_action(m_sprite_action);
  m_light_sprite->set_action(m_light_sprite_action);

  m_col.m_bbox.set_size(static_cast<float>(m_sprite->get_width()),
                        static_cast<float>(m_sprite->get_height()));
}

void
LitObject::draw(DrawingContext& context)
{
  m_sprite->draw(context.color(), get_pos(), m_layer - 1, m_flip);
  m_light_sprite->draw(context.light(), get_pos() - m_light_offset, m_layer - 1, m_flip);
}

void
LitObject::update(float)
{
}

void
LitObject::on_flip(float height)
{
  MovingObject::on_flip(height);
  FlipLevelTransformer::transform_flip(m_flip);
}

const std::string&
LitObject::get_action() const
{
  return m_sprite->get_action();
}

void
LitObject::set_action(const std::string& action)
{
  m_sprite->set_action(action);
}

const std::string&
LitObject::get_light_action() const
{
  return m_light_sprite->get_action();
}

void
LitObject::set_light_action(const std::string& action)
{
  m_light_sprite->set_action(action);
}

/* EOF */
