//  SuperTux
//  Copyright (C) 2010 Florian Forster <supertux at octo.it>
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

#ifndef HEADER_SUPERTUX_BADGUY_SKYDIVE_HPP
#define HEADER_SUPERTUX_BADGUY_SKYDIVE_HPP

#include "badguy/badguy.hpp"

class SkyDive final : public BadGuy
{
public:
  SkyDive(ReaderMapping const& reader);
  
  void kill_fall() override;

  void collision_solid(CollisionHit const& hit) override;
  HitResponse collision_badguy(BadGuy& badguy, CollisionHit const& hit) override;
  void collision_tile(uint32_t tile_attributes) override;

  /* Inherited from Portable */
  void grab(MovingObject& object, Vector const& pos, Direction dir) override;
  void ungrab(MovingObject& object, Direction dir) override;

  bool is_freezable() const override;

  std::string get_overlay_size() const override { return "2x2"; }

private:
  HitResponse collision_player(Player& player, CollisionHit const& hit) override;
  bool collision_squished (GameObject& obj) override;

  void explode();
  bool is_portable() const override;

private:
  SkyDive(SkyDive const&) = delete;
  SkyDive& operator=(SkyDive const&) = delete;
};

#endif

/* EOF */
