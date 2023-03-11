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

#ifndef HEADER_SUPERTUX_BADGUY_JUMPY_HPP
#define HEADER_SUPERTUX_BADGUY_JUMPY_HPP

#include "badguy/badguy.hpp"

class Jumpy final : public BadGuy
{
public:
  Jumpy(ReaderMapping const& reader);

  void collision_solid(CollisionHit const& hit) override;
  HitResponse collision_badguy(BadGuy& other, CollisionHit const& hit) override;

  void active_update(float) override;
  void on_flip(float height) override;

  void freeze() override;
  bool is_freezable() const override;
  bool is_flammable() const override;

private:
  HitResponse hit(CollisionHit const& hit);

private:
  Vector pos_groundhit;
  bool groundhit_pos_set;

private:
  Jumpy(Jumpy const&) = delete;
  Jumpy& operator=(Jumpy const&) = delete;
};

#endif

/* EOF */
