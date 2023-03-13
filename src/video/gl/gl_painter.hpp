//  SuperTux
//  Copyright (C) 2014 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_SUPERTUX_VIDEO_GL_GL_PAINTER_HPP
#define HEADER_SUPERTUX_VIDEO_GL_GL_PAINTER_HPP

#include "video/painter.hpp"

#include "video/flip.hpp"

enum class Blend;
class GLRenderer;
class GLVideoSystem;

class GLPainter final : public Painter
{
public:
  GLPainter(GLVideoSystem& video_system, GLRenderer& renderer);

  void draw_texture(TextureRequest const& request) override;
  void draw_gradient(GradientRequest const& request) override;
  void draw_filled_rect(FillRectRequest const& request) override;
  void draw_inverse_ellipse(InverseEllipseRequest const& request) override;
  void draw_line(LineRequest const& request) override;
  void draw_triangle(TriangleRequest const& request) override;

  void clear(Color const& color) override;

  void set_clip_rect(Rect const& rect) override;
  void clear_clip_rect() override;

private:
  GLVideoSystem& m_video_system;
  GLRenderer& m_renderer;

private:
  std::vector<float> m_vertices;
  std::vector<float> m_uvs;

private:
  GLPainter(GLPainter const&) = delete;
  GLPainter& operator=(GLPainter const&) = delete;
};

#endif

/* EOF */
