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

#include <utility>

#include "object/gradient.hpp"

#include "object/camera.hpp"
#include "supertux/gameconfig.hpp"
#include "supertux/level.hpp"
#include "supertux/sector.hpp"
#include "util/reader.hpp"
#include "util/reader_mapping.hpp"
#include "util/writer.hpp"
#include "video/video_system.hpp"
#include "video/viewport.hpp"

Gradient::Gradient() :
  ExposedObject<Gradient, scripting::Gradient>(this),
  m_layer(LAYER_BACKGROUND0),
  m_gradient_top(),
  m_gradient_bottom(),
  m_gradient_direction(),
  m_blend(),
  m_target(DrawingTarget::COLORMAP),
  m_start_gradient_top(),
  m_start_gradient_bottom(),
  m_fade_gradient_top(),
  m_fade_gradient_bottom(),
  m_fade_total_time(),
  m_fade_time()
{
}

Gradient::Gradient(ReaderMapping const& reader) :
  GameObject(reader),
  ExposedObject<Gradient, scripting::Gradient>(this),
  m_layer(LAYER_BACKGROUND0),
  m_gradient_top(),
  m_gradient_bottom(),
  m_gradient_direction(),
  m_blend(),
  m_target(DrawingTarget::COLORMAP),
  m_start_gradient_top(),
  m_start_gradient_bottom(),
  m_fade_gradient_top(),
  m_fade_gradient_bottom(),
  m_fade_total_time(),
  m_fade_time()
{
  m_layer = reader_get_layer (reader, LAYER_BACKGROUND0);
  std::vector<float> bkgd_top_color, bkgd_bottom_color;
  std::string direction;
  if (reader.read("direction", direction))
  {
    if (direction == "horizontal")
    {
      m_gradient_direction = HORIZONTAL;
    }
    else if (direction == "horizontal_sector")
    {
      m_gradient_direction = HORIZONTAL_SECTOR;
    }
    else if (direction == "vertical_sector")
    {
      m_gradient_direction = VERTICAL_SECTOR;
    }
    else
    {
      m_gradient_direction = VERTICAL;
    }
  }
  else
  {
    m_gradient_direction = VERTICAL;
  }
  if (m_gradient_direction == HORIZONTAL || m_gradient_direction == HORIZONTAL_SECTOR)
  {
    if (!reader.read("left_color", bkgd_top_color) ||
       !reader.read("right_color", bkgd_bottom_color))
    {
      log_warning <<
        "Horizontal gradients should use left_color and right_color, respectively. "
        "Trying to parse top and bottom color instead" << std::endl;
    }
    else
    {
      m_gradient_top = Color(bkgd_top_color);
      m_gradient_bottom = Color(bkgd_bottom_color);
      return;
    }
  }

  if (reader.read("top_color", bkgd_top_color)) {
    m_gradient_top = Color(bkgd_top_color);
  } else {
    m_gradient_top = Color(0.3f, 0.4f, 0.75f);
  }

  if (reader.read("bottom_color", bkgd_bottom_color)) {
    m_gradient_bottom = Color(bkgd_bottom_color);
  } else {
    m_gradient_bottom = Color(1, 1, 1);
  }

  reader.read("blend", m_blend, Blend_from_string);
  reader.read("target", m_target, DrawingTarget_from_string);
}

Gradient::~Gradient()
{
}

void
Gradient::update(float delta)
{
  if (m_fade_time <= 0) return;
  
  m_fade_time -= delta;
  if (m_fade_time <= 0)
  {
    m_fade_time = 0;
    
    m_gradient_top = m_fade_gradient_top;
    m_gradient_bottom = m_fade_gradient_bottom;
    
    return;
  }
  
  float progress = m_fade_time / m_fade_total_time;
  
  m_gradient_top = Color(
    m_fade_gradient_top.red + (m_start_gradient_top.red - m_fade_gradient_top.red) * progress,
    m_fade_gradient_top.green + (m_start_gradient_top.green - m_fade_gradient_top.green) * progress,
    m_fade_gradient_top.blue + (m_start_gradient_top.blue - m_fade_gradient_top.blue) * progress,
    m_fade_gradient_top.alpha + (m_start_gradient_top.alpha - m_fade_gradient_top.alpha) * progress
  );
  m_gradient_bottom = Color(
    m_fade_gradient_bottom.red + (m_start_gradient_bottom.red - m_fade_gradient_bottom.red) * progress,
    m_fade_gradient_bottom.green + (m_start_gradient_bottom.green - m_fade_gradient_bottom.green) * progress,
    m_fade_gradient_bottom.blue + (m_start_gradient_bottom.blue - m_fade_gradient_bottom.blue) * progress,
    m_fade_gradient_bottom.alpha + (m_start_gradient_bottom.alpha - m_fade_gradient_bottom.alpha) * progress
  );
}

void
Gradient::set_gradient(Color top, Color bottom)
{
  m_gradient_top = top;
  m_gradient_bottom = bottom;

  if (m_gradient_top.red > 1.0f ||
      m_gradient_top.green > 1.0f ||
      m_gradient_top.blue > 1.0f ||
      m_gradient_top.alpha > 1.0f)
  {
    log_warning << "top gradient color has values above 1.0" << std::endl;
  }

  if (m_gradient_bottom.red > 1.0f ||
      m_gradient_bottom.green > 1.0f ||
      m_gradient_bottom.blue > 1.0f ||
      m_gradient_bottom.alpha > 1.0f)
  {
    log_warning << "bottom gradient color has values above 1.0" << std::endl;
  }
}

void
Gradient::fade_gradient(Color top, Color bottom, float time)
{
  m_start_gradient_top = m_gradient_top;
  m_start_gradient_bottom = m_gradient_bottom;
  m_fade_gradient_top = top;
  m_fade_gradient_bottom = bottom;
  m_fade_total_time = time;
  m_fade_time = time;
}

void
Gradient::set_direction(GradientDirection const& direction)
{
  m_gradient_direction = direction;
}

void
Gradient::draw(DrawingContext& context)
{
  Rectf gradient_region;
  if (m_gradient_direction != HORIZONTAL && m_gradient_direction != VERTICAL)
  {
      auto camera_translation = Sector::get().get_camera().get_translation();
      auto sector_width = Sector::get().get_width();
      auto sector_height = Sector::get().get_height();
      gradient_region = Rectf(-camera_translation.x, -camera_translation.y, sector_width, sector_height);
  }
  else
  {
    gradient_region = Rectf(0, 0,
                            static_cast<float>(context.get_width()),
                            static_cast<float>(context.get_height()));
  }

  context.push_transform();
  context.set_translation(Vector(0, 0));
  context.get_canvas(m_target).draw_gradient(m_gradient_top, m_gradient_bottom, m_layer, m_gradient_direction,
                                             gradient_region, m_blend);
  context.pop_transform();
}

bool
Gradient::is_saveable() const
{
  return !(Level::current() &&
           Level::current()->is_worldmap());
}

/* EOF */
