//  SuperTux
//  Copyright (C) 2006 Christoph Sommer <christoph.sommer@2006.expires.deltadevelopment.de>
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

#include "object/candle.hpp"

#include "math/random.hpp"
#include "object/sprite_particle.hpp"
#include "sprite/sprite_manager.hpp"
#include "supertux/sector.hpp"
#include "util/reader_mapping.hpp"

Candle::Candle(ReaderMapping const& mapping) :
  MovingSprite(mapping, "images/objects/candle/candle.sprite", LAYER_BACKGROUNDTILES+1, COLGROUP_DISABLED),
  ExposedObject<Candle, scripting::Candle>(this),
  burning(true),
  flicker(true),
  lightcolor(1.0f, 1.0f, 1.0f),
  candle_light_1(SpriteManager::current()->create("images/objects/candle/candle-light-1.sprite")),
  candle_light_2(SpriteManager::current()->create("images/objects/candle/candle-light-2.sprite"))
{
  burning = mapping.get("burning", true);
  flicker = mapping.get("flicker", true);
  std::vector<float> vColor;
  if (!mapping.read("color", vColor)) vColor = {1.0f, 1.0f, 1.0f};
  m_layer = mapping.get("layer", 0);

  //change the light color if defined
  if (vColor.size() >= 3) {
    lightcolor = Color(vColor);
    candle_light_1->set_blend(Blend::ADD);
    candle_light_2->set_blend(Blend::ADD);
    candle_light_1->set_color(lightcolor);
    candle_light_2->set_color(lightcolor);
    //the following allows the original candle appearance to be preserved
    candle_light_1->set_action("white");
    candle_light_2->set_action("white");
  }

  if (burning) {
    m_sprite->set_action("on");
  } else {
    m_sprite->set_action("off");
  }

}

void
Candle::draw(DrawingContext& context)
{
  // draw regular sprite
  MovingSprite::draw(context);

  // draw on lightmap
  if (burning) {
    //Vector pos = get_pos() + (bbox.get_size() - candle_light_1->get_size()) / 2;
    // draw approx. 1 in 10 frames darker. Makes the candle flicker
    if (gameRandom.rand(10) != 0 || !flicker) {
      //context.color().draw_surface(candle_light_1, pos, layer);
      candle_light_1->draw(context.light(), m_col.m_bbox.get_middle(), m_layer);
    } else {
      //context.color().draw_surface(candle_light_2, pos, layer);
      candle_light_2->draw(context.light(), m_col.m_bbox.get_middle(), m_layer);
    }
  }
}

HitResponse
Candle::collision(GameObject&, CollisionHit const& )
{
  return FORCE_MOVE;
}

void
Candle::puff_smoke()
{
  Vector ppos = m_col.m_bbox.get_middle();
  Vector pspeed = Vector(0, -150);
  Vector paccel = Vector(0,0);
  Sector::get().add<SpriteParticle>("images/particles/smoke.sprite",
                                         "default",
                                         ppos, ANCHOR_MIDDLE,
                                         pspeed, paccel,
                                         LAYER_BACKGROUNDTILES+2);
}

bool
Candle::get_burning() const
{
  return burning;
}

void
Candle::set_burning(bool burning_)
{
  if (burning == burning_) return;
  burning = burning_;
  if (burning_) {
    m_sprite->set_action("on");
  } else {
    m_sprite->set_action("off");
  }
  //puff smoke for flickering light sources only
  if (flicker) puff_smoke();
}

/* EOF */
