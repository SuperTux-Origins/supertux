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

#ifndef HEADER_SUPERTUX_BADGUY_STUMPY_HPP
#define HEADER_SUPERTUX_BADGUY_STUMPY_HPP

#include "badguy/walking_badguy.hpp"

class Stumpy final : public WalkingBadguy
{
public:
  Stumpy(ReaderMapping const& reader);
  Stumpy(Vector const& pos, Direction d);

  void initialize() override;
  void active_update(float dt_sec) override;
  void collision_solid(CollisionHit const& hit) override;
  HitResponse collision_badguy(BadGuy& badguy, CollisionHit const& hit) override;

  bool is_freezable() const override;

  std::string get_overlay_size() const override { return "2x2"; }

protected:
  enum MyState {
    STATE_INVINCIBLE, STATE_NORMAL
  };

protected:
  bool collision_squished(GameObject& object) override;

private:
  MyState mystate;
  Timer   invincible_timer;

private:
  Stumpy(Stumpy const&) = delete;
  Stumpy& operator=(Stumpy const&) = delete;
};

#endif

/* EOF */
