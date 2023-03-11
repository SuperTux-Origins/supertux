//  SuperTux
//  Copyright (C) 2020 A. Semphris <semphris@protonmail.com>
//  Copyright (C) 2022 Daniel Ward <weluvgoatz@gmail.com>
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

#ifndef HEADER_SUPERTUX_BADGUY_FISH_SWIMMING_HPP
#define HEADER_SUPERTUX_BADGUY_FISH_SWIMMING_HPP

#include "badguy/badguy.hpp"

class FishSwimming : public BadGuy
{
public:
  FishSwimming(ReaderMapping const& reader);
  FishSwimming(ReaderMapping const& reader, std::string const& spritename);

  void initialize() override;
  void collision_solid(CollisionHit const& hit) override;
  HitResponse collision_badguy(BadGuy& badguy, CollisionHit const& hit) override;
  void active_update(float dt_sec) override;

  void freeze() override;
  void unfreeze(bool melt = true) override;
  bool is_freezable() const override;
  std::string get_overlay_size() const override { return "2x1"; }

  void turn_around();
  void maintain_velocity(float goal_x_velocity);

protected:
  enum FishYState {
    DISRUPTED,
    BALANCED
  };

  FishYState m_state;
  Timer m_beached_timer;
  Timer m_float_timer;
  float m_radius;

private:
  FishSwimming(FishSwimming const&) = delete;
  FishSwimming& operator=(FishSwimming const&) = delete;
};

#endif

/* EOF */
