//  SuperTux - Boss "GhostTree"
//  Copyright (C) 2007 Matthias Braun <matze@braunis.de>
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

#ifndef HEADER_SUPERTUX_BADGUY_ROOT_HPP
#define HEADER_SUPERTUX_BADGUY_ROOT_HPP

#include "badguy/badguy.hpp"

class Root final : public BadGuy
{
public:
  Root(Vector const& pos, Flip flip);
  ~Root() override;

  void deactivate() override;
  void active_update(float dt_sec) override;
  void draw(DrawingContext& context) override;
  bool is_flammable() const override { return false; }
  bool is_freezable() const override { return false; }
  void kill_fall() override { }

protected:
  enum MyState {
    STATE_APPEARING, STATE_HATCHING, STATE_GROWING, STATE_SHRINKING, STATE_VANISHING
  };

private:
  MyState mystate;
  SpritePtr base_sprite;
  float offset_y;
  Timer hatch_timer;
};

#endif

/* EOF */
