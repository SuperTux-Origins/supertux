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

#ifndef HEADER_SUPERTUX_BADGUY_SSPIKY_HPP
#define HEADER_SUPERTUX_BADGUY_SSPIKY_HPP

#include "badguy/walking_badguy.hpp"

class SSpiky final : public WalkingBadguy
{
public:
  SSpiky(ReaderMapping const& reader);

  void initialize() override;
  void collision_solid(CollisionHit const& hit) override;
  HitResponse collision_badguy(BadGuy& badguy, CollisionHit const& hit) override;
  void active_update(float dt_sec) override;

  void freeze() override;
  bool is_freezable() const override;
  bool is_flammable() const override;
  static std::string class_name() { return "sspiky"; }
  std::string get_class_name() const override { return class_name(); }
  static std::string display_name() { return _("Sleeping Spiky"); }
  std::string get_display_name() const override { return display_name(); }

protected:
  enum SSpikyState {
    SSPIKY_SLEEPING,
    SSPIKY_WAKING,
    SSPIKY_WALKING
  };
  SSpikyState state;

private:
  SSpiky(SSpiky const&) = delete;
  SSpiky& operator=(SSpiky const&) = delete;
};

#endif

/* EOF */
