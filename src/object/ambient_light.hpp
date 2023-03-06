//  SuperTux
//  Copyright (C) 2018 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_SUPERTUX_OBJECT_AMBIENT_LIGHT_HPP
#define HEADER_SUPERTUX_OBJECT_AMBIENT_LIGHT_HPP

#include "supertux/game_object.hpp"

#include "video/color.hpp"

class AmbientLight : public GameObject
{
public:
  AmbientLight(Color const& color);
  AmbientLight(ReaderMapping const& mapping);

  virtual void update(float dt_sec) override;
  virtual void draw(DrawingContext& context) override;

  virtual bool is_singleton() const override { return true; }

  static std::string class_name() { return "ambient-light"; }
  virtual std::string get_class_name() const override { return class_name(); }
  static std::string display_name() { return _("Ambient Light"); }
  virtual std::string get_display_name() const override { return display_name(); }
  virtual const std::string get_icon_path() const override { return "images/engine/editor/ambient_light.png"; }

  void set_ambient_light(Color const& ambient_light);
  Color get_ambient_light() const;

  /** Fades to the target ambient light */
  void fade_to_ambient_light(float red, float green, float blue, float seconds);

private:
  Color m_ambient_light;

  /** Specifies whether we're fading the ambient light*/
  bool m_ambient_light_fading;

  /** Source color for fading */
  Color m_source_ambient_light;

  /** Target color for fading */
  Color m_target_ambient_light;

  /** Ambient light fade duration */
  float m_ambient_light_fade_duration;

  /** Accumulated time for fading */
  float m_ambient_light_fade_accum;

private:
  AmbientLight(AmbientLight const&) = delete;
  AmbientLight& operator=(AmbientLight const&) = delete;
};

#endif

/* EOF */
