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

#include "object/ambient_light.hpp"


#include "util/log.hpp"
#include "util/reader_mapping.hpp"
#include "video/drawing_context.hpp"

AmbientLight::AmbientLight(Color const& color) :
  m_ambient_light(color),
  m_ambient_light_fading(false),
  m_source_ambient_light(color),
  m_target_ambient_light(color),
  m_ambient_light_fade_duration(0.0f),
  m_ambient_light_fade_accum(0.0f)
{
}

AmbientLight::AmbientLight(ReaderMapping const& mapping) :
  m_ambient_light(1.0f, 1.0f, 1.0f, 1.0f),
  m_ambient_light_fading(false),
  m_source_ambient_light(1.0f, 1.0f, 1.0f, 1.0f),
  m_target_ambient_light(1.0f, 1.0f, 1.0f, 1.0f),
  m_ambient_light_fade_duration(0.0f),
  m_ambient_light_fade_accum(0.0f)
{
  std::vector<float> color_vec;
  if (mapping.read("color", color_vec))
  {
    if (color_vec.size() < 3) {
      log_warning("ambient-light requires three float arguments");
    } else {
      m_ambient_light = Color(color_vec);
    }
  }
}

void
AmbientLight::update(float dt_sec)
{
  if (m_ambient_light_fading)
  {
    m_ambient_light_fade_accum += dt_sec;
    float percent_done = m_ambient_light_fade_accum / m_ambient_light_fade_duration * 1.0f;
    float r = (1.0f - percent_done) * m_source_ambient_light.red + percent_done * m_target_ambient_light.red;
    float g = (1.0f - percent_done) * m_source_ambient_light.green + percent_done * m_target_ambient_light.green;
    float b = (1.0f - percent_done) * m_source_ambient_light.blue + percent_done * m_target_ambient_light.blue;

    if (r > 1.0f)
      r = 1.0;
    if (g > 1.0f)
      g = 1.0;
    if (b > 1.0f)
      b = 1.0;

    if (r < 0)
      r = 0;
    if (g < 0)
      g = 0;
    if (b < 0)
      b = 0;

    m_ambient_light = Color(r, g, b);

    if (m_ambient_light_fade_accum >= m_ambient_light_fade_duration)
    {
      m_ambient_light = m_target_ambient_light;
      m_ambient_light_fading = false;
      m_ambient_light_fade_accum = 0;
    }
  }
}

void
AmbientLight::draw(DrawingContext& context)
{
  context.set_ambient_color(m_ambient_light);
}

void
AmbientLight::set_ambient_light(Color const& ambient_light)
{
  m_ambient_light = ambient_light;
}

Color
AmbientLight::get_ambient_light() const
{
  return m_ambient_light;
}

void
AmbientLight::fade_to_ambient_light(float red, float green, float blue, float seconds)
{
  if (seconds == 0)
  {
    m_ambient_light = Color(red, green, blue);
    return;
  }

  m_ambient_light_fading = true;
  m_ambient_light_fade_accum = 0;
  m_ambient_light_fade_duration = seconds;
  m_source_ambient_light = m_ambient_light;
  m_target_ambient_light = Color(red, green, blue);
}

/* EOF */
