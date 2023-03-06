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

#ifndef HEADER_SUPERTUX_BADGUY_RCRYSTALLO_HPP
#define HEADER_SUPERTUX_BADGUY_RCRYSTALLO_HPP

#include "badguy/walking_badguy.hpp"

class RCrystallo final : public WalkingBadguy
{
public:
  RCrystallo(ReaderMapping const& reader);

  virtual void initialize() override;
  static std::string class_name() { return "rcrystallo"; }
  virtual std::string get_class_name() const override { return class_name(); }
  static std::string display_name() { return _("Roof Crystallo"); }
  virtual std::string get_display_name() const override { return display_name(); }

  virtual void active_update(float dt_sec) override;
  virtual void draw(DrawingContext& context) override;
  virtual void collision_solid(CollisionHit const& hit) override;
  virtual HitResponse collision_badguy(BadGuy& badguy, CollisionHit const& hit) override;
  virtual HitResponse collision_player(Player& player, CollisionHit const& hit) override;
  virtual bool is_flammable() const override;
  virtual void kill_fall() override;

protected:
  enum RCrystalloState
  {
    RCRYSTALLO_ROOF,
    RCRYSTALLO_DETECT,
    RCRYSTALLO_FALLING
  };
  RCrystalloState state;

private:
  float m_radius;

private:
  RCrystallo(RCrystallo const&) = delete;
  RCrystallo& operator=(RCrystallo const&) = delete;
};

#endif

/* EOF */
