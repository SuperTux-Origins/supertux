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

#include "video/video_system.hpp"

#include <assert.h>
#include <optional>
#include <config.h>
#include <iomanip>
#include <physfs.h>
#include <sstream>

#include "util/file_system.hpp"
#include "util/log.hpp"
#include "video/null/null_video_system.hpp"
#include "video/sdl_surface.hpp"
#include "video/sdl_surface_ptr.hpp"

#include "video/gl/gl_video_system.hpp"

std::unique_ptr<VideoSystem>
VideoSystem::create(VideoSystem::Enum video_system)
{
  switch (video_system)
  {
    case VIDEO_AUTO:
    case VIDEO_OPENGL33CORE:
      return std::make_unique<GLVideoSystem>();

    case VIDEO_NULL:
      return std::make_unique<NullVideoSystem>();

    default:
      log_fatal << "invalid video system in config" << std::endl;
      assert(false);
      return {};
  }
}

VideoSystem::Enum
VideoSystem::get_video_system(std::string const&video)
{
  if (video == "auto")
  {
    return VIDEO_AUTO;
  }
  else if (video == "opengl" || video == "opengl33" || video == "opengl33core")
  {
    return VIDEO_OPENGL33CORE;
  }
  else if (video == "null")
  {
    return VIDEO_NULL;
  }
  else
  {
    throw std::runtime_error("invalid VideoSystem::Enum, valid values are 'auto', 'opengl33' and 'null'");
  }
}

std::string
VideoSystem::get_video_string(VideoSystem::Enum video)
{
  switch (video)
  {
    case VIDEO_AUTO:
      return "auto";
    case VIDEO_OPENGL33CORE:
      return "opengl";
    case VIDEO_NULL:
      return "null";
    default:
      log_fatal << "invalid video system in config" << std::endl;
      assert(false);
      return "auto";
  }
}

std::vector<std::string>
VideoSystem::get_available_video_systems()
{
  std::vector<std::string> output;
  output.push_back("auto");
  output.push_back("opengl33");
  return output;
}

void
VideoSystem::do_take_screenshot()
{
  SDLSurfacePtr surface = make_screenshot();
  if (!surface) {
    log_warning << "Creating the screenshot has failed" << std::endl;
    return;
  }

  const std::string screenshots_dir = "/screenshots";
  if (!PHYSFS_exists(screenshots_dir.c_str())) {
    if (!PHYSFS_mkdir(screenshots_dir.c_str())) {
      log_warning << "Creating '" << screenshots_dir << "' failed" << std::endl;
      return;
    }
  }

  auto find_filename = [&]() -> std::optional<std::string>
    {
      for (int num = 0; num < 1000000; ++num)
      {
        std::ostringstream oss;
        oss << "screenshot" << std::setw(6) << std::setfill('0') << num << ".png";
        const std::string screenshot_filename = FileSystem::join(screenshots_dir, oss.str());
        if (!PHYSFS_exists(screenshot_filename.c_str())) {
          return screenshot_filename;
        }
      }
      return std::nullopt;
    };

  auto filename = find_filename();
  if (!filename)
  {
    log_info << "Failed to find filename to save screenshot" << std::endl;
  }
  else
  {
    if (SDLSurface::save_png(*surface, *filename)) {
      log_info << "Wrote screenshot to \"" << *filename << "\"" << std::endl;
    }
  }
}

/* EOF */
