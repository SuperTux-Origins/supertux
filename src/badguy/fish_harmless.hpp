//  SuperTux
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

#ifndef HEADER_SUPERTUX_BADGUY_FISH_HARMLESS_HPP
#define HEADER_SUPERTUX_BADGUY_FISH_HARMLESS_HPP

#include "badguy/fish_swimming.hpp"

/** Jumping on a trampoline makes tux jump higher. */
class FishHarmless final : public FishSwimming
{
public:
  FishHarmless(ReaderMapping const& reader);

  HitResponse collision_player(Player& player, CollisionHit const& hit) override;

private:
  FishHarmless(FishHarmless const&) = delete;
  FishHarmless& operator=(FishHarmless const&) = delete;
};

#endif

/* EOF */
