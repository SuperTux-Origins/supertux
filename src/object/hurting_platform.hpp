//  SuperTux - Hurting Platform
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

#ifndef HEADER_SUPERTUX_OBJECT_HURTING_PLATFORM_HPP
#define HEADER_SUPERTUX_OBJECT_HURTING_PLATFORM_HPP

#include "object/platform.hpp"

/** Platform that hurts Tux and Badguys when touched */
class HurtingPlatform final : public Platform
{
public:
  HurtingPlatform(ReaderMapping const& reader);

  HitResponse collision(GameObject& other, CollisionHit const& hit) override;
  static std::string class_name() { return "hurting_platform"; }
  std::string get_class_name() const override { return class_name(); }
  static std::string display_name() { return _("Hurting Platform"); }
  std::string get_display_name() const override { return display_name(); }

private:
  HurtingPlatform(HurtingPlatform const&) = delete;
  HurtingPlatform& operator=(HurtingPlatform const&) = delete;
};

#endif

/* EOF */
