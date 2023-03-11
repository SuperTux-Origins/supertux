//  SuperTux - Trampoline
//  Copyright (C) 2006 Wolfgang Becker <uafr@gmx.de>
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

#ifndef HEADER_SUPERTUX_OBJECT_TRAMPOLINE_HPP
#define HEADER_SUPERTUX_OBJECT_TRAMPOLINE_HPP

#include "object/rock.hpp"

/** Jumping on a trampoline makes tux jump higher. */
class Trampoline final : public Rock
{
public:
  Trampoline(ReaderMapping const& reader);
  Trampoline(Vector const& pos, bool port);

  HitResponse collision(GameObject& other, CollisionHit const& hit) override;
  void update(float dt_sec) override;

  void grab(MovingObject&, Vector const& pos, Direction) override;
  bool is_portable() const override;
  static std::string class_name() { return "trampoline"; }
  std::string get_class_name() const override { return class_name(); }
  static std::string display_name() { return _("Trampoline"); }
  std::string get_display_name() const override { return display_name(); }

private:
  bool portable;

private:
  Trampoline(Trampoline const&) = delete;
  Trampoline& operator=(Trampoline const&) = delete;

};

#endif

/* EOF */
