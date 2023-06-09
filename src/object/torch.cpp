//  SuperTux
//  Copyright (C) 2014 Ingo Ruhnke <grumbel@gmail.com>
//  Copyright (C) 2017 M. Teufel <mteufel@supertux.org>
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

#include "object/torch.hpp"

#include "object/player.hpp"
#include "sprite/sprite.hpp"
#include "sprite/sprite_manager.hpp"
#include "util/reader_mapping.hpp"

Torch::Torch(ReaderMapping const& reader) :
  MovingObject(reader),
  ExposedObject<Torch, scripting::Torch>(this),
  m_light_color(1.f, 1.f, 1.f),
  m_torch(),
  m_flame(SpriteManager::current()->create("images/objects/torch/flame.sprite")),
  m_flame_glow(SpriteManager::current()->create("images/objects/torch/flame_glow.sprite")),
  m_flame_light(SpriteManager::current()->create("images/objects/torch/flame_light.sprite")),
  m_burning(true),
  sprite_name("images/objects/torch/torch1.sprite"),
  m_layer(0),
  m_flip(NO_FLIP)
{
  reader.read("x", m_col.m_bbox.get_left());
  reader.read("y", m_col.m_bbox.get_top());

  reader.read("sprite", sprite_name);
  reader.read("burning", m_burning);
  reader.read("layer", m_layer);

  std::vector<float> vColor;
  if (!reader.read("color", vColor)) {
    vColor = { 1.f, 1.f, 1.f };
  }

  m_torch = SpriteManager::current()->create(sprite_name);
  m_col.m_bbox.set_size(static_cast<float>(m_torch->get_width()),
                static_cast<float>(m_torch->get_height()));
  m_flame_glow->set_blend(Blend::ADD);
  m_flame_light->set_blend(Blend::ADD);
  if (vColor.size() >= 3)
  {
    m_light_color = Color(vColor);
    m_flame->set_color(m_light_color);
    m_flame_glow->set_color(m_light_color);
    m_flame_light->set_color(m_light_color);
  }
  set_group(COLGROUP_TOUCHABLE);
}

void
Torch::draw(DrawingContext& context)
{
  if (m_burning)
  {
    Vector pos = get_pos();
    if (m_flip != NO_FLIP) pos.y -= 24.0f;
    m_flame->draw(context.color(), pos, m_layer - 1, m_flip);
    m_flame->set_action(m_light_color.greyscale() >= 1.f ? "default" : "greyscale");

    m_flame_light->draw(context.light(), pos, m_layer);
    m_flame_light->set_action(m_light_color.greyscale() >= 1.f ? "default" : "greyscale");

    m_flame_glow->draw(context.color(), pos, m_layer - 1, m_flip);
    m_flame_glow->set_action(m_light_color.greyscale() >= 1.f ? "default" : "greyscale");
  }

  m_torch->draw(context.color(), get_pos(), m_layer - 1, m_flip);
}

void
Torch::update(float)
{
}

HitResponse
Torch::collision(GameObject& other, CollisionHit const& )
{
  auto player = dynamic_cast<Player*>(&other);
  if (player != nullptr && !m_burning)
  {
    m_burning = true;
  }
  return ABORT_MOVE;
}

bool
Torch::get_burning() const
{
  return m_burning;
}

void
Torch::set_burning(bool burning_)
{
  m_burning = burning_;
}

/* EOF */
