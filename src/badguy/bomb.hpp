//  SuperTux
//  Copyright (C) 2006 Matthias Braun <matze@braunis.de>
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

#ifndef HEADER_SUPERTUX_BADGUY_BOMB_HPP
#define HEADER_SUPERTUX_BADGUY_BOMB_HPP

#include "audio/fwd.hpp"
#include "badguy/badguy.hpp"

class Bomb final : public BadGuy
{
public:
  Bomb(Vector const& pos, Direction dir, std::string const& custom_sprite = "images/creatures/mr_bomb/bomb.sprite" );
  virtual bool is_saveable() const override {
    return false;
  }

  virtual void collision_solid(CollisionHit const& hit) override;
  virtual HitResponse collision_player(Player& player, CollisionHit const& hit) override;
  virtual HitResponse collision_badguy(BadGuy& badguy, CollisionHit const& hit) override;

  virtual void active_update(float dt_sec) override;
  virtual void kill_fall() override;
  virtual void ignite() override;
  void explode();

  virtual bool is_portable() const override;
  virtual void grab(MovingObject& object, Vector const& pos, Direction dir) override;
  virtual void ungrab(MovingObject& object, Direction dir) override;

  virtual void stop_looping_sounds() override;
  virtual void play_looping_sounds() override;

private:
  std::unique_ptr<SoundSource> ticking;

private:
  Bomb(Bomb const&) = delete;
  Bomb& operator=(Bomb const&) = delete;
};

#endif

/* EOF */
