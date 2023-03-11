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

#ifndef HEADER_SUPERTUX_BADGUY_FISH_JUMPING_HPP
#define HEADER_SUPERTUX_BADGUY_FISH_JUMPING_HPP

#include "badguy/badguy.hpp"

class FishJumping final : public BadGuy
{
public:
  FishJumping(ReaderMapping const& );

  void collision_solid(CollisionHit const& hit) override;
  HitResponse collision_badguy(BadGuy& , CollisionHit const& ) override;
  void collision_tile(uint32_t tile_attributes) override;

  void active_update(float) override;

  void freeze() override;
  void unfreeze(bool melt = true) override;
  void kill_fall() override;
  bool is_freezable() const override;

  std::string get_overlay_size() const override { return "1x2"; }
  static std::string class_name() { return "fish-jumping"; }
  std::string get_class_name() const override { return class_name(); }
  static std::string display_name() { return _("Jumping Fish"); }
  std::string get_display_name() const override { return display_name(); }

private:
  HitResponse hit(CollisionHit const& );
  void start_waiting();
  void jump();

  Timer m_wait_timer;
  Timer m_beached_timer;
  float m_stop_y; /**< y-coordinate to stop at */

private:
  FishJumping(FishJumping const&) = delete;
  FishJumping& operator=(FishJumping const&) = delete;
};

#endif

/* EOF */
