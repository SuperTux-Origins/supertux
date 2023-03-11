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

#ifndef HEADER_SUPERTUX_BADGUY_FLYINGSNOWBALL_HPP
#define HEADER_SUPERTUX_BADGUY_FLYINGSNOWBALL_HPP

#include "badguy/badguy.hpp"

class FlyingSnowBall final : public BadGuy
{
public:
  FlyingSnowBall(ReaderMapping const& reader);

  void initialize() override;
  void activate() override;
  void active_update(float dt_sec) override;
  void collision_solid(CollisionHit const& hit) override;

protected:
  bool collision_squished(GameObject& object) override;

private:
  float total_time_elapsed;
  Timer puff_timer; /**< time until the next smoke puff is spawned */

private:
  FlyingSnowBall(FlyingSnowBall const&) = delete;
  FlyingSnowBall& operator=(FlyingSnowBall const&) = delete;
};

#endif

/* EOF */
