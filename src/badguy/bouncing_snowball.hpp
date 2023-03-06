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

#ifndef HEADER_SUPERTUX_BADGUY_BOUNCING_SNOWBALL_HPP
#define HEADER_SUPERTUX_BADGUY_BOUNCING_SNOWBALL_HPP

#include "badguy/badguy.hpp"

class BouncingSnowball final : public BadGuy
{
public:
  BouncingSnowball(ReaderMapping const& reader);

  virtual void initialize() override;
  virtual void active_update(float) override;
  virtual void collision_solid(CollisionHit const& hit) override;
  virtual HitResponse collision_badguy(BadGuy& badguy, CollisionHit const& hit) override;
  static std::string class_name() { return "bouncingsnowball"; }
  virtual std::string get_class_name() const override { return class_name(); }
  static std::string display_name() { return _("Bouncing Snowball"); }
  virtual std::string get_display_name() const override { return display_name(); }

protected:
  virtual bool collision_squished(GameObject& object) override;

private:
  BouncingSnowball(BouncingSnowball const&) = delete;
  BouncingSnowball& operator=(BouncingSnowball const&) = delete;
};

#endif

/* EOF */
