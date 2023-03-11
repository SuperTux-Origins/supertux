//  SuperTux BadGuy GoldBomb - a bomb that throws up coins when exploding
//  Copyright (C) 2006 Matthias Braun <matze@braunis.de>
//  Copyright (C) 2013 LMH <lmh.0013@gmail.com>
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


#ifndef HEADER_SUPERTUX_BADGUY_GOLDBOMB_HPP
#define HEADER_SUPERTUX_BADGUY_GOLDBOMB_HPP

#include "audio/fwd.hpp"
#include "badguy/walking_badguy.hpp"

class GoldBomb final : public WalkingBadguy
{
public:
  GoldBomb(ReaderMapping const& reader);

  void collision_solid(CollisionHit const& hit) override;
  HitResponse collision(GameObject& object, CollisionHit const& hit) override;
  HitResponse collision_player(Player& player, CollisionHit const& hit) override;
  HitResponse collision_badguy(BadGuy& badguy, CollisionHit const& hit) override;

  void active_update(float dt_sec) override;

  void grab(MovingObject& object, Vector const& pos, Direction dir) override;
  void ungrab(MovingObject& object, Direction dir) override;
  bool is_portable() const override;

  void freeze() override;
  bool is_freezable() const override;

  void kill_fall() override;
  void ignite() override;

  void stop_looping_sounds() override;
  void play_looping_sounds() override;

protected:
  bool collision_squished(GameObject& object) override;

private:
  enum Ticking_State {
    STATE_NORMAL,
    STATE_TICKING
  };

private:
  Ticking_State tstate;

  std::unique_ptr<SoundSource> ticking;

private:
  GoldBomb(GoldBomb const&) = delete;
  GoldBomb& operator=(GoldBomb const&) = delete;
};

#endif

/* EOF */
