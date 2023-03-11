//  SuperTux - Badguy "Igel"
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

#ifndef HEADER_SUPERTUX_BADGUY_IGEL_HPP
#define HEADER_SUPERTUX_BADGUY_IGEL_HPP

#include "badguy/walking_badguy.hpp"

/** Badguy "Igel" - a hedgehog that can absorb bullets */
class Igel final : public WalkingBadguy
{
public:
  Igel(ReaderMapping const& reader);

  HitResponse collision_bullet(Bullet& bullet, CollisionHit const& hit) override;

  void active_update(float dt_sec) override;

  bool is_freezable() const override;

  std::string get_overlay_size() const override { return "2x1"; }
  static std::string class_name() { return "igel"; }
  std::string get_class_name() const override { return class_name(); }
  static std::string display_name() { return _("Igel"); }
  std::string get_display_name() const override { return display_name(); }

protected:
  //  bool collision_squished(GameObject& object) override;
  // Enable this and the igel will no longer be butt-jumpable when frozen.
  // Remember to enable it in .cpp too!
  void be_normal(); /**< switch to state STATE_NORMAL */
  void turn_around(); /**< reverse direction, assumes we are in STATE_NORMAL */
  bool can_see(MovingObject const& o) const; /**< check if we can see o */

private:
  Timer turn_recover_timer; /**< wait time until we will turn around again when shot at */

private:
  Igel(Igel const&) = delete;
  Igel& operator=(Igel const&) = delete;
};

#endif

/* EOF */
