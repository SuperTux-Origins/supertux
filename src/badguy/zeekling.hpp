//  Zeekling - flyer that swoops down when she spots the player
//  Copyright (C) 2005 Matthias Braun <matze@braunis.de>
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

#ifndef HEADER_SUPERTUX_BADGUY_ZEEKLING_HPP
#define HEADER_SUPERTUX_BADGUY_ZEEKLING_HPP

#include "badguy/badguy.hpp"

class Zeekling final : public BadGuy
{
public:
  Zeekling(ReaderMapping const& reader);

  void initialize() override;
  void collision_solid(CollisionHit const& hit) override;
  void active_update(float dt_sec) override;

  void freeze() override;
  void unfreeze(bool melt = true) override;
  bool is_freezable() const override;

  std::string get_overlay_size() const override { return "2x1"; }
  static std::string class_name() { return "zeekling"; }
  std::string get_class_name() const override { return class_name(); }
  static std::string display_name() { return _("Zeekling"); }
  std::string get_display_name() const override { return display_name(); }

private:
  bool collision_squished(GameObject& object) override;

  bool should_we_dive();
  void onBumpHorizontal();
  void onBumpVertical();

private:
  enum ZeeklingState {
    FLYING,
    DIVING,
    CLIMBING
  };

private:
  float speed;
  Timer diveRecoverTimer;
  ZeeklingState state;
  MovingObject const* last_player; /**< last player we tracked */
  Vector last_player_pos; /**< position we last spotted the player at */
  Vector last_self_pos; /**< position we last were at */

private:
  Zeekling(Zeekling const&) = delete;
  Zeekling& operator=(Zeekling const&) = delete;
};

#endif

/* EOF */
