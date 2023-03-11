//  SkullyHop - A Hopping Skull
//  Copyright (C) 2006 Christoph Sommer <christoph.sommer@2006.expires.deltadevelopment.de>
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

#ifndef HEADER_SUPERTUX_BADGUY_SKULLYHOP_HPP
#define HEADER_SUPERTUX_BADGUY_SKULLYHOP_HPP

#include "badguy/badguy.hpp"

/** Badguy "SkullyHop" - A Hopping Skull */
class SkullyHop final : public BadGuy
{
public:
  SkullyHop(ReaderMapping const& reader);

  void initialize() override;
  void collision_solid(CollisionHit const& hit) override;
  HitResponse collision_badguy(BadGuy& badguy, CollisionHit const& hit) override;
  bool collision_squished(GameObject& object) override;
  void active_update(float dt_sec) override;

  void unfreeze(bool melt = true) override;
  bool is_freezable() const override;
  static std::string class_name() { return "skullyhop"; }
  std::string get_class_name() const override { return class_name(); }
  static std::string display_name() { return _("Skullyhop"); }
  std::string get_display_name() const override { return display_name(); }

private:
  enum SkullyHopState {
    STANDING,
    CHARGING,
    JUMPING
  };

private:
  void set_state(SkullyHopState newState);

private:
  Timer recover_timer;
  SkullyHopState state;

private:
  SkullyHop(SkullyHop const&) = delete;
  SkullyHop& operator=(SkullyHop const&) = delete;
};

#endif

/* EOF */
