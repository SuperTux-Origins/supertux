//  SuperTux - "Will-O-Wisp" Badguy
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

#ifndef HEADER_SUPERTUX_BADGUY_WILLOWISP_HPP
#define HEADER_SUPERTUX_BADGUY_WILLOWISP_HPP

#include "audio/fwd.hpp"
#include "badguy/badguy.hpp"
#include "object/path_object.hpp"
#include "squirrel/exposed_object.hpp"
#include "scripting/willowisp.hpp"

class WillOWisp final :
  public BadGuy,
  public ExposedObject<WillOWisp, scripting::WillOWisp>,
  public PathObject
{
public:
  WillOWisp(ReaderMapping const& reader);

  void finish_construction() override;

  void activate() override;
  void deactivate() override;

  void active_update(float dt_sec) override;
  bool is_flammable() const override { return false; }
  bool is_freezable() const override { return false; }
  bool is_hurtable() const override { return false; }
  void kill_fall() override { vanish(); }

  virtual void goto_node(int node_no);
  virtual void set_state(std::string const& state);
  virtual void start_moving();
  virtual void stop_moving();

  void stop_looping_sounds() override;
  void play_looping_sounds() override;

  static std::string class_name() { return "willowisp"; }
  std::string get_class_name() const override { return class_name(); }
  static std::string display_name() { return _("Will o' Wisp"); }
  std::string get_display_name() const override { return display_name(); }

  void move_to(Vector const& pos) override;

  void on_flip(float height) override;

  void expose(HSQUIRRELVM vm, SQInteger table_idx) override
  {
    ExposedObject<WillOWisp, scripting::WillOWisp>::expose(vm, table_idx);
  }

  void unexpose(HSQUIRRELVM vm, SQInteger table_idx) override
  {
    ExposedObject<WillOWisp, scripting::WillOWisp>::unexpose(vm, table_idx);
  }

  /** make WillOWisp vanish */
  void vanish();

  Color get_color() const { return m_color; }

private:
  bool collides(GameObject& other, CollisionHit const& hit) const override;
  HitResponse collision_player(Player& player, CollisionHit const& hit) override;

private:
  enum MyState {
    STATE_STOPPED, STATE_IDLE, STATE_TRACKING, STATE_VANISHING, STATE_WARPING,
    STATE_PATHMOVING, STATE_PATHMOVING_TRACK
  };

private:
  MyState m_mystate;

  std::string m_target_sector;
  std::string m_target_spawnpoint;
  std::string m_hit_script;

  std::unique_ptr<SoundSource> m_sound_source;
  float m_flyspeed;
  float m_track_range;
  float m_vanish_range;

  Color m_color;

  int m_starting_node;

private:
  WillOWisp(WillOWisp const&) = delete;
  WillOWisp& operator=(WillOWisp const&) = delete;
};

#endif

/* EOF */
