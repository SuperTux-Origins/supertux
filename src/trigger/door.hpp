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

#ifndef HEADER_SUPERTUX_TRIGGER_DOOR_HPP
#define HEADER_SUPERTUX_TRIGGER_DOOR_HPP

#include "supertux/timer.hpp"
#include "trigger/trigger_base.hpp"
#include "video/flip.hpp"

class Player;

class Door final : public TriggerBase
{
public:
  Door(ReaderMapping const& reader);
  Door(int x, int y, std::string const& sector, std::string const& spawnpoint);
  ~Door() override;


  void update(float dt_sec) override;
  void draw(DrawingContext& context) override;
  void event(Player& player, EventType type) override;
  HitResponse collision(GameObject& other, CollisionHit const& hit) override;

private:
  enum DoorState {
    CLOSED,
    OPENING,
    OPEN,
    CLOSING
  };

private:
  DoorState state; /**< current state of the door */
  std::string target_sector; /**< target sector to teleport to */
  std::string target_spawnpoint; /**< target spawnpoint to teleport to */
  std::string script;
  std::string sprite_name;
  SpritePtr sprite; /**< "door" sprite to render */
  Timer stay_open_timer; /**< time until door will close again */
  Flip m_flip;

private:
  Door(Door const&) = delete;
  Door& operator=(Door const&) = delete;
};

#endif

/* EOF */
