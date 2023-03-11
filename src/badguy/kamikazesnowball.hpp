//  SuperTux
//  Copyright (C) 2008 Wolfgang Becker <uafr@gmx.de>
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

#ifndef HEADER_SUPERTUX_BADGUY_KAMIKAZESNOWBALL_HPP
#define HEADER_SUPERTUX_BADGUY_KAMIKAZESNOWBALL_HPP

#include "badguy/badguy.hpp"

/** Kamikaze Snowball will fly in one direction until he hits something.
    On impact he is destroyed, trying to kill what he hit or hit him. */
class KamikazeSnowball : public BadGuy
{
public:
  KamikazeSnowball(ReaderMapping const& reader);

  void initialize() override;
  void collision_solid(CollisionHit const& hit) override;

protected:
  bool collision_squished(GameObject& object) override;
  HitResponse collision_player(Player& player, CollisionHit const& hit) override;
  virtual void kill_collision();

private:
  KamikazeSnowball(KamikazeSnowball const&) = delete;
  KamikazeSnowball& operator=(KamikazeSnowball const&) = delete;
};

class LeafShot final : public KamikazeSnowball
{
public:
  LeafShot(ReaderMapping const& reader);

  void initialize() override;
  bool is_freezable() const override;

  void freeze() override;
  void unfreeze(bool melt = true) override;
  void kill_collision() override;

  std::string get_overlay_size() const override { return "2x1"; }

protected:
  bool collision_squished(GameObject& object) override;

private:
  LeafShot(LeafShot const&) = delete;
  LeafShot& operator=(LeafShot const&) = delete;
};

#endif

/* EOF */
