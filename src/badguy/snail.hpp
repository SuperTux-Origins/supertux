//  SuperTux - Badguy "Snail"
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

#ifndef HEADER_SUPERTUX_BADGUY_SNAIL_HPP
#define HEADER_SUPERTUX_BADGUY_SNAIL_HPP

#include "badguy/walking_badguy.hpp"

/** Badguy "Snail" - a snail-like creature that can be flipped and
    tossed around at an angle */
class Snail final :
  public WalkingBadguy
{
public:
  Snail(ReaderMapping const& reader);

  void initialize() override;
  void collision_solid(CollisionHit const& hit) override;
  HitResponse collision_badguy(BadGuy& badguy, CollisionHit const& hit) override;
  HitResponse collision_player(Player& player, CollisionHit const& hit) override;
  bool can_break() const override;

  void active_update(float dt_sec) override;

  bool is_freezable() const override;

  bool is_portable() const override;
  void ungrab(MovingObject& , Direction dir_) override;
  void grab(MovingObject&, Vector const& pos, Direction dir_) override;

protected:
  bool collision_squished(GameObject& object) override;

  void be_normal(); /**< switch to state STATE_NORMAL */
  void be_flat(); /**< switch to state STATE_FLAT */
  void be_kicked(bool upwards); /**< switch to state STATE_KICKED_DELAY */
  void be_grabbed();
  void wake_up();

private:
  enum State {
    STATE_NORMAL, /**< walking around */
    STATE_FLAT, /**< flipped upside-down */
    STATE_WAKING, /**< is waking up */
    STATE_KICKED_DELAY, /**< short delay before being launched */
    STATE_KICKED, /**< launched */
    STATE_GRABBED, /**< grabbed by tux */
  };

private:
  State state;
  Timer kicked_delay_timer; /**< wait time until switching from STATE_KICKED_DELAY to STATE_KICKED */
  Timer flat_timer;
  int   squishcount;

private:
  Snail(Snail const&) = delete;
  Snail& operator=(Snail const&) = delete;
};

#endif

/* EOF */
