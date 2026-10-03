//  SuperTux - Lantern
//  Copyright (C) 2006 Wolfgang Becker <uafr@gmx.de>
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

#include "object/lantern.hpp"


#include "audio/sound_manager.hpp"
#include "ecs/object_behaviors.hpp"
#include "badguy/treewillowisp.hpp"
#include "badguy/archetype_badguy.hpp"
#include "ecs/badguy_behaviors.hpp"
#include "ecs/badguy_components.hpp"
#include "sprite/sprite_manager.hpp"
#include "util/reader_mapping.hpp"

namespace {

Rock read_rock(ReaderMapping const& reader)
{
  Rock rock;
  read_component(reader, rock);
  return rock;
}

} // namespace

Lantern::Lantern(ReaderMapping const& reader) :
  PortableObject(reader, "images/objects/lantern/lantern.sprite", LAYER_OBJECTS, COLGROUP_MOVING_STATIC),
  lightcolor(1.0f, 1.0f, 1.0f),
  lightsprite(SpriteManager::current()->create("images/objects/lightmap_light/lightmap_light.sprite"))
{
  add_behavior(read_rock(reader));

  std::vector<float> vColor;
  if (reader.read("color", vColor)) {
    lightcolor = Color(vColor);
  } else {
    lightcolor = Color(1, 1, 1);
  }
  lightsprite->set_blend(Blend::ADD);
  updateColor();
  SoundManager::current()->preload("sounds/willocatch.wav");
}

Lantern::Lantern(Vector const& pos) :
  PortableObject(pos, "images/objects/lantern/lantern.sprite", LAYER_OBJECTS, COLGROUP_MOVING_STATIC),
  lightcolor(0.0f, 0.0f, 0.0f),
  lightsprite(SpriteManager::current()->create("images/objects/lightmap_light/lightmap_light.sprite"))
{
  add_behavior(Rock());

  lightsprite->set_blend(Blend::ADD);
  updateColor();
  SoundManager::current()->preload("sounds/willocatch.wav");
}

void
Lantern::updateColor(){
  lightsprite->set_color(lightcolor);
  //Turn lantern off if light is black
  if (lightcolor.red == 0 && lightcolor.green == 0 && lightcolor.blue == 0){
    m_sprite->set_action("off");
    m_sprite->set_color(Color(1.0f, 1.0f, 1.0f));
  } else {
    m_sprite->set_action("normal");
    m_sprite->set_color(lightcolor);
  }
}

void
Lantern::draw(DrawingContext& context){
  //Draw the Sprite.
  MovingSprite::draw(context);
  //Let there be light.
  lightsprite->draw(context.light(), m_col.m_bbox.get_middle(), 0);
}

HitResponse Lantern::collision(GameObject& other, CollisionHit const& hit) {

  auto* wow = dynamic_cast<ArchetypeBadguy*>(&other);
  auto* wisp = wow ? ecs::try_get<WillOWisp>(wow->get_entity()) : nullptr;

  if (wisp && (is_open() || wisp->color.greyscale() == 0.f)) {
    // collided with WillOWisp while grabbed and unlit
    SoundManager::current()->play("sounds/willocatch.wav", get_pos());
    lightcolor = wisp->color;
    updateColor();
    willowisp::vanish(*wow);
  }

  TreeWillOWisp* twow = dynamic_cast<TreeWillOWisp*>(&other);
  if (twow && (is_open() || twow->get_color().greyscale() == 0.f)) {
    // collided with TreeWillOWisp while grabbed and unlit
    SoundManager::current()->play("sounds/willocatch.wav", get_pos());
    lightcolor = twow->get_color();
    updateColor();
    twow->vanish();
  }

  return ArchetypeObject::collision(other, hit);
}

void
Lantern::grab(MovingObject& object, Vector const& pos, Direction dir)
{
  PortableObject::grab(object, pos, dir);

  // if lantern is not lit, draw it as opened
  if (is_open()) {
    m_sprite->set_action("off-open");
  }

}

void
Lantern::ungrab(MovingObject& object, Direction dir)
{
  // if lantern is not lit, it was drawn as opened while grabbed. Now draw it as closed again
  if (is_open()) {
    m_sprite->set_action("off");
  }

  PortableObject::ungrab(object, dir);
}

bool
Lantern::is_open() const
{
  return (is_grabbed() && lightcolor.red == 0 && lightcolor.green == 0 && lightcolor.blue == 0);
}

void
Lantern::add_color(Color const& c)
{
  lightcolor.red   = std::min(1.0f, lightcolor.red   + c.red);
  lightcolor.green = std::min(1.0f, lightcolor.green + c.green);
  lightcolor.blue  = std::min(1.0f, lightcolor.blue  + c.blue);
  lightcolor.alpha = std::min(1.0f, lightcolor.alpha + c.alpha);
  updateColor();
}

/* EOF */
