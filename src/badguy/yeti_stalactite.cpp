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
#include "badguy/yeti_stalactite.hpp"

#include "ecs/badguy_behaviors.hpp"
#include "sprite/sprite.hpp"

namespace {

Stalactite read_stalactite(ReaderMapping const& mapping)
{
  Stalactite stalactite;
  read_component(mapping, stalactite);
  return stalactite;
}

} // namespace

YetiStalactite::YetiStalactite(ReaderMapping const& mapping) :
  ArchetypeBadguy(mapping, "images/creatures/stalactite/stalactite.sprite", LAYER_TILES - 1,
                  "images/objects/lightmap_light/lightmap_light-medium.sprite"),
  m_stalactite(add_behavior(read_stalactite(mapping)))
{
}

void
YetiStalactite::start_shaking()
{
  m_stalactite.timer.start(stalactite::SHAKE_TIME);
  m_stalactite.state = Stalactite::State::SHAKING;
  if ((static_cast<int>(get_pos().x) / 32) % 2 == 0) {
    m_physic.set_velocity_y(100);
  }
}

bool
YetiStalactite::is_hanging() const
{
  return m_stalactite.state == Stalactite::State::HANGING;
}

void
YetiStalactite::active_update(float dt_sec)
{
  if (m_stalactite.state == Stalactite::State::HANGING)
    return;

  ArchetypeBadguy::active_update(dt_sec);
}

void
YetiStalactite::update(float dt_sec)
{
  // Respawn instead of removing once squished
  if (get_state() == STATE_SQUISHED && check_state_timer()) {
    set_state(STATE_ACTIVE);
    m_stalactite.state = Stalactite::State::HANGING;
    // Hopefully we shouldn't come into contact with anything...
    m_sprite->set_action("normal");
    set_pos(m_start_position);
    set_colgroup_active(COLGROUP_TOUCHABLE);
  }

  // Call back to badguy to do normal stuff
  ArchetypeBadguy::update(dt_sec);
}

/* EOF */
