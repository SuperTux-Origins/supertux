//  SuperTux
//  Copyright (C) 2006 Matthias Braun <matze@braunis.de>
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

#ifndef HEADER_SUPERTUX_BADGUY_SHORT_FUSE_HPP
#define HEADER_SUPERTUX_BADGUY_SHORT_FUSE_HPP

#include "badguy/walking_badguy.hpp"

class ShortFuse final : public WalkingBadguy
{
public:
  ShortFuse(ReaderMapping const& reader);

  static std::string class_name() { return "short_fuse"; }
  std::string get_class_name() const override { return class_name(); }
  static std::string display_name() { return _("Short Fuse"); }
  std::string get_display_name() const override { return display_name(); }

protected:
  HitResponse collision_player (Player& player, CollisionHit const& hit) override;
  bool collision_squished (GameObject& object) override;
  bool is_freezable() const override;
  void freeze() override;
  void kill_fall() override;
  void ignite() override;

  void explode();

private:
  ShortFuse(ShortFuse const&) = delete;
  ShortFuse& operator=(ShortFuse const&) = delete;
};

#endif

/* EOF */
