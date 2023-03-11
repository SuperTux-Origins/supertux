//  SuperTux - "Will-O-Wisp" Badguy
//  Copyright (C) 2007 Matthias Braun <matze@braunis.de>
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

#ifndef HEADER_SUPERTUX_BADGUY_TREEWILLOWISP_HPP
#define HEADER_SUPERTUX_BADGUY_TREEWILLOWISP_HPP

#include "audio/fwd.hpp"
#include "badguy/badguy.hpp"

class GhostTree;

class TreeWillOWisp final : public BadGuy
{
public:
  TreeWillOWisp(GhostTree* tree, Vector const& pos, float radius, float speed);
  ~TreeWillOWisp() override;

  void activate() override;
  void active_update(float dt_sec) override;

  bool is_flammable() const override { return false; }
  bool is_freezable() const override { return false; }
  void kill_fall() override { vanish(); }

  void draw(DrawingContext& context) override;

  void stop_looping_sounds() override;
  void play_looping_sounds() override;

  /** make TreeWillOWisp vanish */
  void vanish();
  void start_sucking(Vector const& suck_target);

  void set_color(Color const& color);
  Color get_color() const;

protected:
  bool collides(GameObject& other, CollisionHit const& hit) const override;
  HitResponse collision_player(Player& player, CollisionHit const& hit) override;

private:
  enum MyState {
    STATE_DEFAULT, STATE_VANISHING, STATE_SUCKED
  };

public:
  bool was_sucked;

private:
  MyState mystate;

  Color color;
  float angle;
  float radius;
  float speed;

  std::unique_ptr<SoundSource> sound_source;
  GhostTree* tree;

  Vector suck_target;

private:
  TreeWillOWisp(TreeWillOWisp const&) = delete;
  TreeWillOWisp& operator=(TreeWillOWisp const&) = delete;
};

#endif

/* EOF */
