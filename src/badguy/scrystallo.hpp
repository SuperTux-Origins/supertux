//  SuperTux
//  Copyright (C) 2021 Daniel Ward <weluvgoatz@gmail.com>
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

#ifndef HEADER_SUPERTUX_BADGUY_SCRYSTALLO_HPP
#define HEADER_SUPERTUX_BADGUY_SCRYSTALLO_HPP

#include "badguy/walking_badguy.hpp"

class SCrystallo final : public WalkingBadguy
{
public:
  SCrystallo(ReaderMapping const& reader);

  void initialize() override;
  static std::string class_name() { return "scrystallo"; }
  std::string get_class_name() const override { return class_name(); }
  static std::string display_name() { return _("Sleeping Crystallo"); }
  std::string get_display_name() const override { return display_name(); }

  void collision_solid(CollisionHit const& hit) override;
  HitResponse collision_badguy(BadGuy& badguy, CollisionHit const& hit) override;

  void active_update(float dt_sec) override;
  bool is_flammable() const override;

protected:
  bool collision_squished(GameObject& object) override;
protected:
  enum SCrystalloState
  {
    SCRYSTALLO_SLEEPING,
    SCRYSTALLO_WAKING,
    SCRYSTALLO_JUMPING,
    SCRYSTALLO_WALKING
  };
  SCrystalloState state;

private:
  float m_radius;
  float m_range;
  Vector m_radius_anchor;
private:
  SCrystallo(SCrystallo const&) = delete;
  SCrystallo& operator=(SCrystallo const&) = delete;
};

#endif
