//  Crusher - A block to stand on, which can drop down to crush the player
//  Copyright (C) 2008 Christoph Sommer <christoph.sommer@2008.expires.deltadevelopment.de>
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

#ifndef HEADER_SUPERTUX_BADGUY_CRUSHER_HPP
#define HEADER_SUPERTUX_BADGUY_CRUSHER_HPP

#include "object/moving_sprite.hpp"
#include "supertux/direction.hpp"
#include "supertux/physic.hpp"

class Player;

/** This class is the base class for crushers that tux can stand on */
class Crusher final : public MovingSprite
{
public:
  enum CrusherState
  {
    IDLE,
    CRUSHING,
    RECOVERING
  };

  enum class Direction
  {
    DOWN,
    LEFT,
    RIGHT
  };

private:
  enum CrusherSize
  {
    NORMAL,
    LARGE
  };

public:
  Crusher(ReaderMapping const& reader);

  HitResponse collision(GameObject& other, CollisionHit const& hit) override;
  void collision_solid(CollisionHit const& hit) override;
  void update(float dt_sec) override;
  void draw(DrawingContext& context) override;

  virtual bool is_sideways() const { return m_sideways; }

  Physic& get_physic() { return m_physic; }
  bool is_big() const { return m_ic_size == LARGE; }
  CrusherState get_state() const { return m_state; }

private:
  void spawn_roots(Direction direction);

  bool found_victim() const;
  bool not_ice() const;
  void set_state(CrusherState state, bool force = false);
  void after_sprite_set();
  Vector eye_position(bool right) const;

private:
  CrusherState m_state;
  CrusherSize m_ic_size;
  Vector m_start_position;
  Physic m_physic;
  float m_cooldown_timer;
  bool m_sideways;
  Direction m_side_dir;

  SpritePtr m_lefteye;
  SpritePtr m_righteye;
  SpritePtr m_whites;

private:
  Crusher(Crusher const&) = delete;
  Crusher& operator=(Crusher const&) = delete;
};

class CrusherRoot : public MovingSprite
{
public:
  CrusherRoot(Vector position, Crusher::Direction direction, float delay, int layer);

  HitResponse collision(GameObject& other, CollisionHit const& hit) override;
  void update(float dt_sec) override;

private:
  void start_animation();
  bool delay_gone() { return m_delay_remaining <= 0.f; }

private:
  Vector m_original_pos;
  Crusher::Direction m_direction;
  float m_delay_remaining;

private:
  CrusherRoot(CrusherRoot const&) = delete;
  CrusherRoot& operator=(CrusherRoot const&) = delete;
};

#endif

/* EOF */
