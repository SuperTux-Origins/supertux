//  SuperTux - Crystallo
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

#ifndef HEADER_SUPERTUX_BADGUY_CRYSTALLO_HPP
#define HEADER_SUPERTUX_BADGUY_CRYSTALLO_HPP

#include "badguy/walking_badguy.hpp"

/** Basic badguy, patrols around a fixed position. */
class Crystallo final : public WalkingBadguy
{
public:
  Crystallo(ReaderMapping const& reader);


  void active_update(float dt_sec) override;
  bool is_flammable() const override;

protected:
  bool collision_squished(GameObject& object) override;

private:
  float m_radius;

private:
  Crystallo(Crystallo const&) = delete;
  Crystallo& operator=(Crystallo const&) = delete;
};

#endif

/* EOF */
