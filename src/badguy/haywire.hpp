//  SuperTux
//  Copyright (C) 2006 Matthias Braun <matze@braunis.de>
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

#ifndef HEADER_SUPERTUX_BADGUY_HAYWIRE_HPP
#define HEADER_SUPERTUX_BADGUY_HAYWIRE_HPP

#include "audio/fwd.hpp"
#include "badguy/walking_badguy.hpp"

class Haywire final : public WalkingBadguy
{
public:
  Haywire(ReaderMapping const& reader);

  void kill_fall() override;
  void ignite() override;

  void active_update(float dt_sec) override;
  void deactivate() override;

  bool is_freezable() const override;
  void freeze() override;

  void stop_looping_sounds() override;
  void play_looping_sounds() override;

  HitResponse collision_badguy(BadGuy& badguy, CollisionHit const& hit) override;


protected:
  bool collision_squished(GameObject& object) override;

private:
  void start_exploding();
  void stop_exploding();

private:
  bool is_exploding;
  float time_until_explosion;
  bool is_stunned;
  float time_stunned;

  std::unique_ptr<SoundSource> ticking;
  std::unique_ptr<SoundSource> grunting;
  
  Timer stomped_timer;

private:
  Haywire(Haywire const&) = delete;
  Haywire& operator=(Haywire const&) = delete;
};

#endif

/* EOF */
