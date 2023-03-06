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

#ifndef HEADER_SUPERTUX_BADGUY_KUGELBLITZ_HPP
#define HEADER_SUPERTUX_BADGUY_KUGELBLITZ_HPP

#include "badguy/badguy.hpp"

class Kugelblitz final : public BadGuy
{
public:
  Kugelblitz(ReaderMapping const& reader);

  virtual void initialize() override;
  virtual HitResponse collision_badguy(BadGuy& other, CollisionHit const& hit) override;
  virtual void collision_solid(CollisionHit const& hit) override;
  virtual HitResponse collision_player(Player& player, CollisionHit const& hit) override;

  virtual void active_update(float) override;
  virtual void kill_fall() override;

  virtual bool is_flammable() const override;

  virtual void draw(DrawingContext& context) override;
  static std::string class_name() { return "kugelblitz"; }
  virtual std::string get_class_name() const override { return class_name(); }
  static std::string display_name() { return _("Kugelblitz"); }
  virtual std::string get_display_name() const override { return display_name(); }

  void explode();

private:
  void try_activate();
  HitResponse hit(CollisionHit const& hit);

private:
  Vector pos_groundhit;
  bool groundhit_pos_set;
  bool dying;
  Timer movement_timer;
  Timer lifetime;
  int direction;
  SpritePtr lightsprite;

private:
  Kugelblitz(Kugelblitz const&) = delete;
  Kugelblitz& operator=(Kugelblitz const&) = delete;
};

#endif

/* EOF */
