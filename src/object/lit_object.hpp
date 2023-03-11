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

#ifndef HEADER_SUPERTUX_OBJECT_LIT_OBJECT_HPP
#define HEADER_SUPERTUX_OBJECT_LIT_OBJECT_HPP

#include "scripting/lit_object.hpp"
#include "sprite/sprite_ptr.hpp"
#include "squirrel/exposed_object.hpp"
#include "supertux/moving_object.hpp"
#include "video/flip.hpp"

class LitObject final :
  public MovingObject,
  public ExposedObject<LitObject, scripting::LitObject>
{
public:
  LitObject(ReaderMapping const& reader);

  void draw(DrawingContext& context) override;
  void update(float) override;

  HitResponse collision(GameObject&, CollisionHit const&) override
    { return ABORT_MOVE; }


  int get_layer() const override { return m_layer; }

  void on_flip(float height) override;

  std::string const& get_action() const;
  void set_action(std::string const& action);
  std::string const& get_light_action() const;
  void set_light_action(std::string const& action);

private:
  Vector m_light_offset;
  std::string m_sprite_name;
  std::string m_light_sprite_name;
  std::string m_sprite_action;
  std::string m_light_sprite_action;
  SpritePtr m_sprite;
  SpritePtr m_light_sprite;
  int m_layer;
  Flip m_flip;

private:
  LitObject(LitObject const&) = delete;
  LitObject& operator=(LitObject const&) = delete;
};

#endif

/* EOF */
