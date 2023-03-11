//  SuperTux
//  Copyright (C) 2015 Hume2 <teratux.mail@gmail.com>
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

#ifndef HEADER_SUPERTUX_BADGUY_WALKING_CANDLE_HPP
#define HEADER_SUPERTUX_BADGUY_WALKING_CANDLE_HPP

#include "badguy/walking_badguy.hpp"

class WalkingCandle final : public WalkingBadguy
{
public:
  WalkingCandle(ReaderMapping const& reader);

  bool is_freezable() const override;
  bool is_flammable() const override;

  void freeze() override;
  void unfreeze(bool melt = true) override;

  HitResponse collision(GameObject& other, CollisionHit const& hit) override;

  void kill_fall() override;


private:
  Color lightcolor;

private:
  WalkingCandle(WalkingCandle const&) = delete;
  WalkingCandle& operator=(WalkingCandle const&) = delete;
};

#endif

/* EOF */
