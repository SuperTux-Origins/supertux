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

#ifndef HEADER_SUPERTUX_VIDEO_NULL_NULL_VIDEO_SYSTEM_HPP
#define HEADER_SUPERTUX_VIDEO_NULL_NULL_VIDEO_SYSTEM_HPP

#include "video/viewport.hpp"
#include "video/video_system.hpp"

class TextureManager;
class NullRenderer;

/** A video system that doesn't produce any output and doesn't open a
    window. Useful for debugging, testing and automation. */
class NullVideoSystem : public VideoSystem
{
public:
  NullVideoSystem();
  ~NullVideoSystem() override;

  std::string get_name() const override { return "Null"; }

  Renderer* get_back_renderer() const override;
  Renderer& get_renderer() const override;
  Renderer& get_lightmap() const override;

  TexturePtr new_texture(SDL_Surface const& image, Sampler const& sampler)  override;

  Viewport const& get_viewport() const override;
  void apply_config() override;
  void flip() override;
  void on_resize(int w, int h) override;
  Size get_window_size() const override;

  void set_vsync(int mode) override;
  int get_vsync() const override;
  void set_gamma(float gamma) override;
  void set_title(std::string const& title) override;
  void set_icon(SDL_Surface const& icon) override;
  SDLSurfacePtr make_screenshot() override;

private:
  Size m_window_size;
  int m_vsync_mode;
  Viewport m_viewport;
  std::unique_ptr<NullRenderer> m_screen_renderer;
  std::unique_ptr<NullRenderer> m_lightmap_renderer;
  std::unique_ptr<TextureManager> m_texture_manager;

private:
  NullVideoSystem(NullVideoSystem const&) = delete;
  NullVideoSystem& operator=(NullVideoSystem const&) = delete;
};

#endif

/* EOF */
