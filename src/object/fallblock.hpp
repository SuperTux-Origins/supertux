//  Copyright (C) 2020 Daniel Ward <weluvgoatz@gmail.com>
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

#ifndef HEADER_SUPERTUX_OBJECT_FALLBLOCK_HPP
#define HEADER_SUPERTUX_OBJECT_FALLBLOCK_HPP

#include "object/moving_sprite.hpp"
#include "supertux/physic.hpp"
#include "supertux/timer.hpp"

class Player;

class FallBlock : public MovingSprite

{
public:
  FallBlock(ReaderMapping const& reader);
  
  void update(float dt_sec) override;

  HitResponse collision(GameObject& other, CollisionHit const& hit) override;
  void collision_solid(CollisionHit const& hit) override;

  void draw(DrawingContext& context) override;
  
  static std::string class_name() { return "fallblock"; }
  std::string get_class_name() const override { return class_name(); }
  static std::string display_name() { return _("Falling Platform"); }
  std::string get_display_name() const override { return display_name(); }

  void on_flip(float height) override;
  
protected:
  enum State
  {
    IDLE,
    SHAKE,
    FALL,
    LAND
  };
  
private:
  State state;
    
  Physic physic;
  Timer timer;
  
  bool found_victim_down() const;

private:
  FallBlock(FallBlock const&) = delete;
  FallBlock& operator=(FallBlock const&) = delete;
};

#endif

/* EOF */
