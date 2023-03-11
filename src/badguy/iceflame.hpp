//  SuperTux badguy - Iceflame a flame-like enemy that can be killed with fireballs
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

#ifndef HEADER_SUPERTUX_BADGUY_ICEFLAME_HPP
#define HEADER_SUPERTUX_BADGUY_ICEFLAME_HPP

#include "badguy/flame.hpp"

class Iceflame final : public Flame
{
public:
  Iceflame(ReaderMapping const& reader);

  void active_update(float dt_sec) override;

  void ignite() override;
  bool is_flammable() const override;
  bool is_freezable() const override;
  static std::string class_name() { return "iceflame"; }
  std::string get_class_name() const override { return class_name(); }
  static std::string display_name() { return _("Ice Flame"); }
  std::string get_display_name() const override { return display_name(); }

private:
  Iceflame(Iceflame const&) = delete;
  Iceflame& operator=(Iceflame const&) = delete;
};

#endif

/* EOF */
