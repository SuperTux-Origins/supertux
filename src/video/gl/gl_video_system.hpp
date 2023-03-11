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

#ifndef HEADER_SUPERTUX_VIDEO_GL_GL_VIDEO_SYSTEM_HPP
#define HEADER_SUPERTUX_VIDEO_GL_GL_VIDEO_SYSTEM_HPP

#include <memory>
#include <SDL.h>

#include "math/size.hpp"
#include "video/sdlbase_video_system.hpp"
#include "video/viewport.hpp"

class GLContext;
class GLLightmap;
class GLProgram;
class GLScreenRenderer;
class GLTexture;
class GLTextureRenderer;
class GLVertexArrays;
class Rect;
class TextureManager;
struct SDL_Surface;

class GLVideoSystem final : public SDLBaseVideoSystem
{
public:
  GLVideoSystem();
  ~GLVideoSystem() override;

  std::string get_name() const override;

  Renderer* get_back_renderer() const override;
  Renderer& get_renderer() const override;
  Renderer& get_lightmap() const override;

  TexturePtr new_texture(SDL_Surface const& image, Sampler const& sampler) override;

  Viewport const& get_viewport() const override { return m_viewport; }
  void apply_config() override;
  void flip() override;

  void set_vsync(int mode) override;
  int get_vsync() const override;

  SDLSurfacePtr make_screenshot() override;

  GLContext& get_context() const { return *m_context; }

private:
  void create_gl_window();
  void create_gl_context();

private:
  std::unique_ptr<TextureManager> m_texture_manager;
  std::unique_ptr<GLScreenRenderer> m_renderer;
  std::unique_ptr<GLTextureRenderer> m_lightmap;
  std::unique_ptr<GLTextureRenderer> m_back_renderer;
  std::unique_ptr<GLContext> m_context;

  SDL_GLContext m_glcontext;
  Viewport m_viewport;

private:
  GLVideoSystem(GLVideoSystem const&) = delete;
  GLVideoSystem& operator=(GLVideoSystem const&) = delete;
};

#endif

/* EOF */
