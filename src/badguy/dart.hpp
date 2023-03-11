//  Dart - Your average poison dart
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

#ifndef HEADER_SUPERTUX_BADGUY_DART_HPP
#define HEADER_SUPERTUX_BADGUY_DART_HPP

#include "badguy/badguy.hpp"
#include "audio/fwd.hpp"

/** Badguy "Dart" - Your average poison dart */
class Dart final : public BadGuy
{
public:
  Dart(ReaderMapping const& reader);
  Dart(Vector const& pos, Direction d, BadGuy const* parent);

  void initialize() override;
  void activate() override;
  void deactivate() override;

  void active_update(float dt_sec) override;

  void collision_solid(CollisionHit const& hit) override;
  HitResponse collision_badguy(BadGuy& badguy, CollisionHit const& hit) override;
  HitResponse collision_player(Player& player, CollisionHit const& hit) override;

  virtual bool updatePointers(GameObject const* from_object, GameObject* to_object);

  bool is_flammable() const override;

  bool is_hurtable() const override { return false; }

  void stop_looping_sounds() override;
  void play_looping_sounds() override;

  void on_flip(float height) override;

protected:
  BadGuy const* parent; /**< collisions with this BadGuy will be ignored */
  std::unique_ptr<SoundSource> sound_source; /**< SoundSource for ambient sound */

private:
  Dart(Dart const&) = delete;
  Dart& operator=(Dart const&) = delete;
};

#endif

/* EOF */
