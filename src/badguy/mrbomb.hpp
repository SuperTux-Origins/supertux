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

#ifndef HEADER_SUPERTUX_BADGUY_MRBOMB_HPP
#define HEADER_SUPERTUX_BADGUY_MRBOMB_HPP

#include "badguy/walking_badguy.hpp"

class MrBomb final : public WalkingBadguy
{
public:
  MrBomb(ReaderMapping const& reader);

  void kill_fall() override;
  void ignite() override;
  HitResponse collision(GameObject& object, CollisionHit const& hit) override;
  HitResponse collision_player(Player& player, CollisionHit const& hit) override;

  void active_update(float dt_sec) override;

  void grab(MovingObject& object, Vector const& pos, Direction dir) override;
  bool is_portable() const override;

  bool is_freezable() const override;

protected:
  bool collision_squished(GameObject& object) override;

private:
  MrBomb(MrBomb const&) = delete;
  MrBomb& operator=(MrBomb const&) = delete;
};

#endif

/* EOF */
