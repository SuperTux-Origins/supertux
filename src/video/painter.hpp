//  SuperTux
//  Copyright (C) 2006 Matthias Braun <matze@braunis.de>
//                2016 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_SUPERTUX_VIDEO_PAINTER_HPP
#define HEADER_SUPERTUX_VIDEO_PAINTER_HPP

#include "math/rect.hpp"
#include "math/vector.hpp"
#include "video/color.hpp"

class Rect;
struct DrawingRequest;
struct FillRectRequest;
struct GetPixelRequest;
struct GradientRequest;
struct InverseEllipseRequest;
struct LineRequest;
struct TextureBatchRequest;
struct TextureRequest;
struct TriangleRequest;

class Painter
{
public:
  Painter() {}
  virtual ~Painter() {}

  virtual void draw_texture(TextureRequest const& request) = 0;
  virtual void draw_gradient(GradientRequest const& request) = 0;
  virtual void draw_filled_rect(FillRectRequest const& request) = 0;
  virtual void draw_inverse_ellipse(InverseEllipseRequest const& request) = 0;
  virtual void draw_line(LineRequest const& request) = 0;
  virtual void draw_triangle(TriangleRequest const& request) = 0;

  virtual void clear(Color const& color) = 0;
  virtual void get_pixel(GetPixelRequest const& request) const = 0;

  virtual void set_clip_rect(Rect const& rect) = 0;
  virtual void clear_clip_rect() = 0;

private:
  Painter(Painter const&) = delete;
  Painter& operator=(Painter const&) = delete;
};

#endif

/* EOF */
