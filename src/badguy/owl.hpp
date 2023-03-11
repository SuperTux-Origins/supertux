//  SuperTux
//  Copyright (C) 2008 Wolfgang Becker <uafr@gmx.de>
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

#ifndef HEADER_SUPERTUX_BADGUY_OWL_HPP
#define HEADER_SUPERTUX_BADGUY_OWL_HPP

#include "badguy/badguy.hpp"

class Portable;

class Owl final : public BadGuy
{
private:
  static std::vector<std::string> s_portable_objects;

public:
  Owl(ReaderMapping const& reader);

  void initialize() override;
  void collision_solid(CollisionHit const& hit) override;
  void kill_fall() override;

  void freeze() override;
  void unfreeze(bool melt = true) override;
  bool is_freezable() const override;
  void ignite() override;

  std::string get_overlay_size() const override { return "2x2"; }

protected:
  bool is_above_player() const;
  void active_update (float dt_sec) override;
  bool collision_squished(GameObject& object) override;

  std::string carried_obj_name;
  Portable *carried_object;

private:
  Owl(Owl const&) = delete;
  Owl& operator=(Owl const&) = delete;
};

#endif

/* EOF */
