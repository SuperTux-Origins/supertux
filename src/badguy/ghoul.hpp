//  SuperTux
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

#ifndef HEADER_SUPERTUX_BADGUY_GHOUL_HPP
#define HEADER_SUPERTUX_BADGUY_GHOUL_HPP

#include "badguy/badguy.hpp"
#include "object/path_object.hpp"

// FIXME: Ghoul inherits PathObject, but does not override get_settings() to add
//        the missing options.
class Ghoul final : public BadGuy,
                    public PathObject
{
public:
  Ghoul(ReaderMapping const& reader);
  bool is_freezable() const override;
  bool is_flammable() const override;

  void finish_construction() override;

  void activate() override;
  void deactivate() override;
  void active_update(float dt_sec) override;
  
  void goto_node(int node_no);
  void set_state(std::string const& state);
  void start_moving();
  void stop_moving();

  void move_to(Vector const& pos) override;

protected:
  bool collision_squished(GameObject& object) override;
  
private:
  enum MyState {
    STATE_STOPPED, STATE_IDLE, STATE_TRACKING, STATE_PATHMOVING, STATE_PATHMOVING_TRACK
  };
  
private:
  MyState m_mystate;
  float m_flyspeed;
  float m_track_range;
  
private:
  Ghoul(Ghoul const&) = delete;
  Ghoul& operator=(Ghoul const&) = delete;
};

#endif

/* EOF */
