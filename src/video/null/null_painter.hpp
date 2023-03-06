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

#ifndef HEADER_SUPERTUX_VIDEO_NULL_NULL_PAINTER_HPP
#define HEADER_SUPERTUX_VIDEO_NULL_NULL_PAINTER_HPP

#include "video/painter.hpp"

#include <optional>

class NullPainter : public Painter
{
public:
  NullPainter();
  ~NullPainter() override;

  virtual void draw_texture(TextureRequest const& request) override;
  virtual void draw_gradient(GradientRequest const& request) override;
  virtual void draw_filled_rect(FillRectRequest const& request) override;
  virtual void draw_inverse_ellipse(InverseEllipseRequest const& request) override;
  virtual void draw_line(LineRequest const& request) override;
  virtual void draw_triangle(TriangleRequest const& request) override;

  virtual void clear(Color const& color) override;
  virtual void get_pixel(GetPixelRequest const& request) const override;

  virtual void set_clip_rect(Rect const& rect) override;
  virtual void clear_clip_rect() override;

private:
  std::optional<Rect> m_clip_rect;

private:
  NullPainter(NullPainter const&) = delete;
  NullPainter& operator=(NullPainter const&) = delete;
};


#endif

/* EOF */
