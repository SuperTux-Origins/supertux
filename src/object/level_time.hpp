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

#ifndef HEADER_SUPERTUX_OBJECT_LEVEL_TIME_HPP
#define HEADER_SUPERTUX_OBJECT_LEVEL_TIME_HPP

#include "squirrel/exposed_object.hpp"
#include "scripting/level_time.hpp"
#include "supertux/game_object.hpp"
#include "video/color.hpp"
#include "video/surface_ptr.hpp"

class LevelTime final : public GameObject,
                        public ExposedObject<LevelTime, scripting::LevelTime>
{
  static Color text_color;
public:
  LevelTime(ReaderMapping const& reader);

  void update(float dt_sec) override;
  void draw(DrawingContext& context) override;

  /** @name Scriptable Methods
      @{ */

  /** Resumes the countdown */
  void start();

  /** Pauses the countdown */
  void stop();

  /** Returns the number of seconds left on the clock */
  float get_time() const;

  /** Changes the number of seconds left on the clock */
  void set_time(float time_left);

  /** @} */
  static std::string class_name() { return "leveltime"; }
  std::string get_class_name() const override { return class_name(); }
  static std::string display_name() { return _("Time Limit"); }
  std::string get_display_name() const override { return display_name(); }

  const std::string get_icon_path() const override { return "images/engine/editor/clock.png"; }

private:
  SurfacePtr time_surface;
  bool running;
  float time_left;

private:
  LevelTime(LevelTime const&) = delete;
  LevelTime& operator=(LevelTime const&) = delete;
};

#endif

/* EOF */
