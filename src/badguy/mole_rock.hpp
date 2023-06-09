//  MoleRock - Rock thrown by "Mole" Badguy
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

#ifndef HEADER_SUPERTUX_BADGUY_MOLE_ROCK_HPP
#define HEADER_SUPERTUX_BADGUY_MOLE_ROCK_HPP

#include "badguy/badguy.hpp"

/** Badguy "MoleRock" - Rock thrown by "Mole" Badguy */
class MoleRock final : public BadGuy
{
public:
  MoleRock(ReaderMapping const& reader);
  MoleRock(Vector const& pos, Vector const& velocity, BadGuy const* parent);

  void initialize() override;
  void deactivate() override;

  void active_update(float dt_sec) override;

  void collision_solid(CollisionHit const& hit) override;
  HitResponse collision_badguy(BadGuy& badguy, CollisionHit const& hit) override;
  HitResponse collision_player(Player& player, CollisionHit const& hit) override;

  virtual bool updatePointers(GameObject const* from_object, GameObject* to_object);

  bool is_flammable() const override;
  bool is_hurtable() const override { return false; }

protected:
  BadGuy const* parent; /**< collisions with this BadGuy will be ignored */
  const Vector initial_velocity; /**< velocity at time of creation */

private:
  MoleRock(MoleRock const&) = delete;
  MoleRock& operator=(MoleRock const&) = delete;
};

#endif

/* EOF */
