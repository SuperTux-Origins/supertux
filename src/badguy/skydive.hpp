//  SuperTux
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

#ifndef HEADER_SUPERTUX_BADGUY_SKYDIVE_HPP
#define HEADER_SUPERTUX_BADGUY_SKYDIVE_HPP

#include "badguy/badguy.hpp"

class SkyDive final : public BadGuy
{
public:
  SkyDive(ReaderMapping const& reader);
  
  virtual void kill_fall() override;

  virtual void collision_solid(CollisionHit const& hit) override;
  virtual HitResponse collision_badguy(BadGuy& badguy, CollisionHit const& hit) override;
  virtual void collision_tile(uint32_t tile_attributes) override;

  /* Inherited from Portable */
  virtual void grab(MovingObject& object, Vector const& pos, Direction dir) override;
  virtual void ungrab(MovingObject& object, Direction dir) override;

  virtual bool is_freezable() const override;

  virtual std::string get_overlay_size() const override { return "2x2"; }
  static std::string class_name() { return "skydive"; }
  virtual std::string get_class_name() const override { return class_name(); }
  static std::string display_name() { return _("Skydive"); }
  virtual std::string get_display_name() const override { return display_name(); }

private:
  virtual HitResponse collision_player(Player& player, CollisionHit const& hit) override;
  virtual bool collision_squished (GameObject& obj) override;

  void explode();
  virtual bool is_portable() const override;

private:
  SkyDive(SkyDive const&) = delete;
  SkyDive& operator=(SkyDive const&) = delete;
};

#endif

/* EOF */
