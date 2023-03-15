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

#include "object/platform.hpp"

#include "object/player.hpp"
#include "supertux/sector.hpp"
#include "util/reader_mapping.hpp"

Platform::Platform(ReaderMapping const& reader) :
  Platform(reader, "images/objects/flying_platform/flying_platform.sprite")
{
}

Platform::Platform(ReaderMapping const& reader, std::string const& default_sprite) :
  MovingSprite(reader, default_sprite, LAYER_OBJECTS, COLGROUP_STATIC),
  ExposedObject<Platform, scripting::Platform>(this),
  PathObject(),
  m_speed(Vector(0,0)),
  m_automatic(false),
  m_player_contact(false),
  m_last_player_contact(false),
  m_starting_node(0)
{
  bool running = true;
  reader.read("running", running);
  if ((get_name().empty()) && (!running)) {
    m_automatic = true;
  }

  m_starting_node = reader.get("starting-node", 0.f);

  init_path(reader, true);
}

void
Platform::finish_construction()
{
  if (!get_path())
  {
    // If no path is given, make a one-node dummy path
    init_path_pos(m_col.m_bbox.p1(), false);
  }

  if (m_starting_node >= static_cast<int>(get_path()->get_nodes().size()))
    m_starting_node = static_cast<int>(get_path()->get_nodes().size()) - 1;

  get_walker()->jump_to_node(m_starting_node);

  m_col.m_bbox.set_pos(m_path_handle.get_pos(m_col.m_bbox.get_size(), get_path()->get_nodes()[m_starting_node].position));
}

HitResponse
Platform::collision(GameObject& other, CollisionHit const& )
{
  if (dynamic_cast<Player*>(&other)) {
    m_player_contact = true;
  }
  return FORCE_MOVE;
}

void
Platform::update(float dt_sec)
{
  if (!get_path()) return;
  if (!get_path()->is_valid()) return;

  // check if Platform should automatically pick a destination
  if (m_automatic)
  {
    if (!m_player_contact && !get_walker()->is_running()) {
      // Player doesn't touch platform and Platform is not moving

      // Travel to node nearest to nearest player
      if (auto* player = Sector::get().get_nearest_player(m_col.m_bbox)) {
        int nearest_node_id = get_path()->get_nearest_node_no(player->get_bbox().p2());
        if (nearest_node_id != -1) {
          goto_node(nearest_node_id);
        }
      }
    }

    if (m_player_contact && !m_last_player_contact && !get_walker()->is_running()) {
      // Player touched platform, didn't touch last frame and Platform is not moving

      // Travel to node farthest from current position
      int farthest_node_id = get_path()->get_farthest_node_no(get_pos());
      if (farthest_node_id != -1) {
        goto_node(farthest_node_id);
      }
    }

    // Clear player_contact flag set by collision() method
    m_last_player_contact = m_player_contact;
    m_player_contact = false;
  }

  get_walker()->update(dt_sec);
  Vector movement = get_walker()->get_pos(m_col.m_bbox.get_size(), m_path_handle) - get_pos();
  m_col.set_movement(movement);
  m_col.propagate_movement(movement);
  m_speed = movement / dt_sec;
}

void
Platform::goto_node(int node_no)
{
  get_walker()->goto_node(node_no);
}

void
Platform::start_moving()
{
  get_walker()->start_moving();
}

void
Platform::stop_moving()
{
  get_walker()->stop_moving();
}

void
Platform::set_action(std::string const& action, int repeat)
{
  MovingSprite::set_action(action, repeat);
}

void
Platform::move_to(Vector const& pos)
{
  Vector shift = pos - m_col.m_bbox.p1();
  if (get_path()) {
    get_path()->move_by(shift);
  }
  set_pos(pos);
}

/* EOF */
