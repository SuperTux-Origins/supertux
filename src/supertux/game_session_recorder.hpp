//  SuperTux
//  Copyright (C) 2017 Tobias Markus <tobbi.bugs@googlemail.com>
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

#ifndef HEADER_SUPERTUX_SUPERTUX_GAME_SESSION_RECORDER_HPP
#define HEADER_SUPERTUX_SUPERTUX_GAME_SESSION_RECORDER_HPP

#include <memory>
#include <string>

#include "control/codecontroller.hpp"

class GameSessionRecorder
{
public:
  GameSessionRecorder();
  virtual ~GameSessionRecorder();

  void start_recording();
  void record_demo(std::string const& filename);
  int get_demo_random_seed(std::string const& filename) const;
  void play_demo(std::string const& filename);
  void process_events();

  /** Re-sets the demo controller in case the sector (and thus the
      Player instance) changes. */
  void reset_demo_controller();

  bool is_playing_demo() const { return m_playing; }

private:
  void capture_demo_step();

private:
  std::string m_capture_file;
  std::unique_ptr<std::ostream> m_capture_demo_stream;
  std::unique_ptr<std::istream> m_playback_demo_stream;
  std::unique_ptr<CodeController> m_demo_controller;
  bool m_playing;

private:
  GameSessionRecorder(GameSessionRecorder const&) = delete;
  GameSessionRecorder& operator=(GameSessionRecorder const&) = delete;
};

#endif

/* EOF */
