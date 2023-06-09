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

#ifndef HEADER_SUPERTUX_BADGUY_STALACTITE_HPP
#define HEADER_SUPERTUX_BADGUY_STALACTITE_HPP

#include "badguy/badguy.hpp"

class Stalactite : public BadGuy
{
public:
  Stalactite(ReaderMapping const& reader);

  void active_update(float dt_sec) override;
  void collision_solid(CollisionHit const& hit) override;
  HitResponse collision_player(Player& player, CollisionHit const& hit) override;
  HitResponse collision_badguy(BadGuy& other, CollisionHit const& hit) override;
  HitResponse collision_bullet(Bullet& bullet, CollisionHit const& hit) override;

  void kill_fall() override;
  void draw(DrawingContext& context) override;
  void deactivate() override;

  void squish();

protected:
  enum StalactiteType {
    ICE,
    ROCK
  };
  enum StalactiteState {
    STALACTITE_HANGING,
    STALACTITE_SHAKING,
    STALACTITE_FALLING,
    STALACTITE_SQUISHED
  };

  static const std::vector<std::string> s_sprites;

  static StalactiteType StalactiteType_from_string(std::string const& type_string);

protected:
  StalactiteType m_type;
  Timer timer;
  StalactiteState state;
  Vector shake_delta;

private:
  Stalactite(Stalactite const&) = delete;
  Stalactite& operator=(Stalactite const&) = delete;
};

#endif

/* EOF */
