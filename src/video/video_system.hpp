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

#ifndef HEADER_SUPERTUX_VIDEO_VIDEO_SYSTEM_HPP
#define HEADER_SUPERTUX_VIDEO_VIDEO_SYSTEM_HPP

#include <string>
#include <vector>
#include <SDL.h>

#include "math/size.hpp"
#include "util/currenton.hpp"
#include "video/sampler.hpp"
#include "video/texture_ptr.hpp"

class Rect;
class Renderer;
class SDLSurfacePtr;
class Sampler;
class Surface;
class SurfaceData;
class Viewport;

class VideoSystem : public Currenton<VideoSystem>
{
public:
  enum Enum {
    VIDEO_AUTO,
    VIDEO_OPENGL33CORE,
    VIDEO_NULL
  };

  static std::unique_ptr<VideoSystem> create(VideoSystem::Enum video_system);

  static Enum get_video_system(std::string const&video);
  static std::string get_video_string(Enum video);
  static std::vector<std::string> get_available_video_systems();

public:
  VideoSystem() {}
  ~VideoSystem() override {}

  /** Return a human readable name of the current video system */
  virtual std::string get_name() const = 0;

  virtual Renderer* get_back_renderer() const = 0;
  virtual Renderer& get_renderer() const = 0;
  virtual Renderer& get_lightmap() const = 0;

  virtual TexturePtr new_texture(SDL_Surface const& image, Sampler const& sampler = Sampler()) = 0;

  virtual Viewport const& get_viewport() const = 0;
  virtual void apply_config() = 0;
  virtual void flip() = 0;
  virtual void on_resize(int w, int h) = 0;
  virtual Size get_window_size() const = 0;

  virtual void set_vsync(int mode) = 0;
  virtual int get_vsync() const = 0;
  virtual void set_gamma(float gamma) = 0;
  virtual void set_title(std::string const& title) = 0;
  virtual void set_icon(SDL_Surface const& icon) = 0;
  virtual SDLSurfacePtr make_screenshot() = 0;

  void do_take_screenshot();

private:
  VideoSystem(VideoSystem const&) = delete;
  VideoSystem& operator=(VideoSystem const&) = delete;
};

#endif

/* EOF */
