//  SuperTux badguy - walking flame that glows
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

#ifndef HEADER_SUPERTUX_BADGUY_LIVEFIRE_HPP
#define HEADER_SUPERTUX_BADGUY_LIVEFIRE_HPP

#include "badguy/walking_badguy.hpp"

class LiveFire : public WalkingBadguy
{
public:
  LiveFire(ReaderMapping const& reader);

  void collision_solid(CollisionHit const& hit) override;
  HitResponse collision_badguy(BadGuy& badguy, CollisionHit const& hit) override;
  void active_update(float dt_sec) override;

  void freeze() override;
  bool is_freezable() const override;
  bool is_flammable() const override;

  void kill_fall() override;

private:
  std::string death_sound;

protected:
  enum SState {
    STATE_SLEEPING,
    STATE_WAKING,
    STATE_WALKING,
    STATE_DORMANT,
    STATE_DEAD
  };

protected:
  SState state;

private:
  LiveFire(LiveFire const&) = delete;
  LiveFire& operator=(LiveFire const&) = delete;
};

class LiveFireAsleep final : public LiveFire
{
public:
  LiveFireAsleep(ReaderMapping const& reader);

  void draw(DrawingContext& context) override;

  void initialize() override;

private:
  LiveFireAsleep(LiveFireAsleep const&) = delete;
  LiveFireAsleep& operator=(LiveFireAsleep const&) = delete;
};

class LiveFireDormant final : public LiveFire
{
public:
  LiveFireDormant(ReaderMapping const& reader);

  void draw(DrawingContext& context) override;

  void initialize() override;

private:
  LiveFireDormant(LiveFireDormant const&) = delete;
  LiveFireDormant& operator=(LiveFireDormant const&) = delete;
};

#endif

/* EOF */
