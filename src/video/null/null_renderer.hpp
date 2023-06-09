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

#ifndef HEADER_SUPERTUX_VIDEO_NULL_NULL_RENDERER_HPP
#define HEADER_SUPERTUX_VIDEO_NULL_NULL_RENDERER_HPP

#include "video/renderer.hpp"

class NullPainter;
class Rect;
struct DrawingRequest;
struct SDL_Window;

class NullRenderer : public Renderer
{
public:
  NullRenderer();
  ~NullRenderer() override;

  void start_draw() override;
  void end_draw() override;

  Painter& get_painter() override;

  Rect get_rect() const override;
  Size get_logical_size() const override;

  TexturePtr get_texture() const override;

private:
  std::unique_ptr<NullPainter> m_painter;

private:
  NullRenderer(NullRenderer const&) = delete;
  NullRenderer& operator=(NullRenderer const&) = delete;
};

#endif

/* EOF */
