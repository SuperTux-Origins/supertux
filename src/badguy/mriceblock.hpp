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

#ifndef HEADER_SUPERTUX_BADGUY_MRICEBLOCK_HPP
#define HEADER_SUPERTUX_BADGUY_MRICEBLOCK_HPP

#include "badguy/walking_badguy.hpp"

class MrIceBlock : public WalkingBadguy
{
public:
  MrIceBlock(ReaderMapping const& reader);

  void initialize() override;
  HitResponse collision(GameObject& object, CollisionHit const& hit) override;
  void collision_solid(CollisionHit const& hit) override;
  HitResponse collision_badguy(BadGuy& badguy, CollisionHit const& hit) override;
  HitResponse collision_player(Player& player, CollisionHit const& hit) override;

  void active_update(float dt_sec) override;

  void grab(MovingObject& object, Vector const& pos, Direction dir) override;
  void ungrab(MovingObject& object, Direction dir) override;
  bool is_portable() const override;

  bool can_break() const override;

  void ignite() override;


  bool can_break();

protected:
  enum IceState {
    ICESTATE_NORMAL,
    ICESTATE_FLAT,
    ICESTATE_GRABBED,
    ICESTATE_KICKED,
    ICESTATE_WAKING
  };

protected:
  bool collision_squished(GameObject& object) override;
  void set_state(IceState state);

private:
  IceState ice_state;
  Timer nokick_timer;
  Timer flat_timer;
  int squishcount;

private:
  MrIceBlock(MrIceBlock const&) = delete;
  MrIceBlock& operator=(MrIceBlock const&) = delete;
};

#endif

/* EOF */
