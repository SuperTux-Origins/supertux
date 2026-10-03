//  SuperTux - WalkingBadguy
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
#include "badguy/walking_badguy.hpp"

#include <math.h>

#include "ecs/badguy_behaviors.hpp"

namespace {

Walker make_walker(std::string const& left_action, std::string const& right_action)
{
  Walker walker;
  walker.left_action = left_action;
  walker.right_action = right_action;
  return walker;
}

} // namespace

WalkingBadguy::WalkingBadguy(Vector const& pos,
                             std::string const& sprite_name_,
                             std::string const& walk_left_action_,
                             std::string const& walk_right_action_,
                             int layer_,
                             std::string const& light_sprite_name) :
  ArchetypeBadguy(pos, sprite_name_, layer_, light_sprite_name),
  m_walker(add_behavior(make_walker(walk_left_action_, walk_right_action_)))
{
}

WalkingBadguy::WalkingBadguy(Vector const& pos,
                             Direction direction,
                             std::string const& sprite_name_,
                             std::string const& walk_left_action_,
                             std::string const& walk_right_action_,
                             int layer_,
                             std::string const& light_sprite_name) :
  ArchetypeBadguy(pos, direction, sprite_name_, layer_, light_sprite_name),
  m_walker(add_behavior(make_walker(walk_left_action_, walk_right_action_)))
{
}

WalkingBadguy::WalkingBadguy(ReaderMapping const& reader,
                             std::string const& sprite_name_,
                             std::string const& walk_left_action_,
                             std::string const& walk_right_action_,
                             int layer_,
                             std::string const& light_sprite_name) :
  ArchetypeBadguy(reader, sprite_name_, layer_, light_sprite_name),
  m_walker(add_behavior(make_walker(walk_left_action_, walk_right_action_)))
{
}

void
WalkingBadguy::active_update(float dt_sec, float target_velocity, float modifier)
{
  BadGuy::active_update(dt_sec);
  walker::walk(*this, m_walker, target_velocity, modifier);
}

float
WalkingBadguy::get_walk_speed() const
{
  return m_walker.speed;
}

void
WalkingBadguy::set_walk_speed(float speed)
{
  m_walker.speed = fabsf(speed);
}

void
WalkingBadguy::turn_around()
{
  walker::turn_around(*this, m_walker);
}

/* EOF */
