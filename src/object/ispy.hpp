//  SuperTux - Ispy
//  Copyright (C) 2007 Christoph Sommer <christoph.sommer@2007.expires.deltadevelopment.de>
//                2022 Jiri Palecek <narre@protonmail.com>
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

#ifndef HEADER_SUPERTUX_OBJECT_ISPY_HPP
#define HEADER_SUPERTUX_OBJECT_ISPY_HPP

#include "object/moving_sprite.hpp"
#include "supertux/direction.hpp"

/** An Ispy: When it spots Tux, a script will run. */
class Ispy final : public MovingSprite
{
public:
  Ispy(ReaderMapping const& mapping);

  HitResponse collision(GameObject& other, CollisionHit const& hit) override;

  void update(float dt_sec) override;

  void on_flip(float height) override;

private:
  void set_sprite_action(std::string const& action, int loops = -1);

private:
  enum IspyState {
    ISPYSTATE_IDLE,
    ISPYSTATE_ALERT,
    ISPYSTATE_HIDING,
    ISPYSTATE_SHOWING
  };

private:
  IspyState m_state; /**< current state */

  std::string m_script; /**< script to execute when Tux is spotted */
  Direction m_dir;

private:
  Ispy(Ispy const&) = delete;
  Ispy& operator=(Ispy const&) = delete;
};

#endif

/* EOF */
