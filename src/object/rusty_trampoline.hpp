//  SuperTux - Rusty Trampoline
//  Copyright (C) 2006 Wolfgang Becker <uafr@gmx.de>
//  Copyright (C) 2011 Jonas Kuemmerlin <rgcjonas@googlemail.com>
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

#ifndef HEADER_SUPERTUX_OBJECT_RUSTY_TRAMPOLINE_HPP
#define HEADER_SUPERTUX_OBJECT_RUSTY_TRAMPOLINE_HPP

#include "object/rock.hpp"

/** Jumping on a trampoline makes tux jump higher.
    After 3 jumps, it breaks (configurable)
    It cannot be carried (breaks on ungrab) */
class RustyTrampoline final : public Rock
{
public:
  RustyTrampoline(ReaderMapping const& reader);

  HitResponse collision(GameObject& other, CollisionHit const& hit) override;
  void collision_solid(CollisionHit const& hit) override;
  void update(float dt_sec) override;

  void grab(MovingObject&, Vector const& pos, Direction) override;
  void ungrab(MovingObject&, Direction) override;
  bool is_portable() const override;

private:
  bool portable;
  int counter;

private:
  RustyTrampoline(RustyTrampoline const&) = delete;
  RustyTrampoline& operator=(RustyTrampoline const&) = delete;
};

#endif

/* EOF */
