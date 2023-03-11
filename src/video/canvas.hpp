//  SuperTux
//  Copyright (C) 2006 Matthias Braun <matze@braunis.de>
//                2018 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_SUPERTUX_VIDEO_CANVAS_HPP
#define HEADER_SUPERTUX_VIDEO_CANVAS_HPP

#include <memory>
#include <obstack.h>
#include <string>
#include <vector>

#include "math/rectf.hpp"
#include "math/vector.hpp"
#include "video/blend.hpp"
#include "video/color.hpp"
#include "video/drawing_target.hpp"
#include "video/font.hpp"
#include "video/font_ptr.hpp"
#include "video/gl.hpp"
#include "video/gradient.hpp"
#include "video/layer.hpp"
#include "video/paint_style.hpp"

class DrawingContext;
class Renderer;
class VideoSystem;
struct DrawingRequest;

class Canvas final
{
public:
  enum Filter { BELOW_LIGHTMAP, ABOVE_LIGHTMAP, ALL };

public:
  Canvas(DrawingContext& context, obstack& obst);
  ~Canvas();

  void draw_surface(SurfacePtr const& surface, Vector const& position, int layer);
  void draw_surface(SurfacePtr const& surface, Vector const& position, float angle, Color const& color, Blend const& blend,
                    int layer);
  void draw_surface_part(SurfacePtr const& surface, Rectf const& srcrect, Rectf const& dstrect,
                         int layer, PaintStyle const& style = PaintStyle());
  void draw_surface_scaled(SurfacePtr const& surface, Rectf const& dstrect,
                           int layer, PaintStyle const& style = PaintStyle());
  void draw_surface_batch(SurfacePtr const& surface,
                          std::vector<Rectf> srcrects,
                          std::vector<Rectf> dstrects,
                          Color const& color,
                          int layer);
  void draw_surface_batch(SurfacePtr const& surface,
                          std::vector<Rectf> srcrects,
                          std::vector<Rectf> dstrects,
                          std::vector<float> angles,
                          Color const& color,
                          int layer);
  void draw_text(FontPtr const& font, std::string const& text,
                 Vector const& position, FontAlignment alignment, int layer, Color const& color = Color(1.0,1.0,1.0));
  /** Draw text to the center of the screen */
  void draw_center_text(FontPtr const& font, std::string const& text,
                        Vector const& position, int layer, Color const& color = Color(1.0,1.0,1.0));
  void draw_gradient(Color const& from, Color const& to, int layer, GradientDirection const& direction,
                     Rectf const& region, Blend const& blend = Blend());
  void draw_filled_rect(Rectf const& rect, Color const& color, int layer);
  void draw_filled_rect(Rectf const& rect, Color const& color, float radius, int layer);

  void draw_inverse_ellipse(Vector const& pos, Vector const& size, Color const& color, int layer);

  void draw_line(Vector const& pos1, Vector const& pos2, Color const& color, int layer);
  void draw_triangle(Vector const& pos1, Vector const& pos2, Vector const& pos3, Color const& color, int layer);

  /** on next update, set color to lightmap's color at position */
  void get_pixel(Vector const& position, std::shared_ptr<Color> const& color_out);

  void clear();
  void render(Renderer& renderer, Filter filter);

  DrawingContext& get_context() { return m_context; }

private:
  Vector apply_translate(Vector const& pos) const;
  float scale() const;

private:
  DrawingContext& m_context;
  obstack& m_obst;
  std::vector<DrawingRequest*> m_requests;

private:
  Canvas(Canvas const&) = delete;
  Canvas& operator=(Canvas const&) = delete;
};

#endif

/* EOF */
