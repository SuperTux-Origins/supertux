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

#ifndef HEADER_SUPERTUX_VIDEO_GL_GL33CORE_CONTEXT_HPP
#define HEADER_SUPERTUX_VIDEO_GL_GL33CORE_CONTEXT_HPP

#include "video/gl/gl_context.hpp"

#include <memory>

class GLProgram;
class GLTexture;
class GLVertexArrays;
class GLVideoSystem;

class GL33CoreContext final : public GLContext
{
public:
  GL33CoreContext(GLVideoSystem& video_system);
  ~GL33CoreContext() override;

  std::string get_name() const override { return "opengl33"; }

  void bind() override;

  void ortho(float width, float height, bool vflip) override;

  void blend_func(GLenum src, GLenum dst) override;

  void set_positions(float const* data, size_t size) override;

  void set_texcoords(float const* data, size_t size) override;
  void set_texcoord(float u, float v) override;

  void set_colors(float const* data, size_t size) override;
  void set_color(Color const& color) override;

  void bind_texture(Texture const& texture, Texture const* displacement_texture) override;
  void bind_no_texture() override;
  void draw_arrays(GLenum type, GLint first, GLsizei count) override;

  bool supports_framebuffer() const override { return true; }

  GLProgram& get_program() const { return *m_program; }
  GLVertexArrays& get_vertex_arrays() const { return *m_vertex_arrays; }
  GLTexture& get_white_texture() const { return *m_white_texture; }

private:
  GLVideoSystem& m_video_system;
  std::unique_ptr<GLProgram> m_program;
  std::unique_ptr<GLVertexArrays> m_vertex_arrays;
  std::unique_ptr<GLTexture> m_white_texture;
  std::unique_ptr<GLTexture> m_black_texture;
  std::unique_ptr<GLTexture> m_grey_texture;
  std::unique_ptr<GLTexture> m_transparent_texture;

private:
  GL33CoreContext(GL33CoreContext const&) = delete;
  GL33CoreContext& operator=(GL33CoreContext const&) = delete;
};

#endif

/* EOF */
