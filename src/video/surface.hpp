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

#ifndef HEADER_SUPERTUX_VIDEO_SURFACE_HPP
#define HEADER_SUPERTUX_VIDEO_SURFACE_HPP

#include <optional>
#include <string>

#include "math/rect.hpp"
#include "math/vector.hpp"
#include "util/reader_fwd.hpp"
#include "video/flip.hpp"
#include "video/surface_ptr.hpp"
#include "video/texture_ptr.hpp"

class SurfaceData;

/** A rectangular image.  The class basically holds a reference to a
    texture with additional UV coordinates that specify a rectangular
    area on this texture */
class Surface final
{
public:
  static SurfacePtr from_texture(TexturePtr const& texture);
  static SurfacePtr from_file(std::string const& filename, std::optional<Rect> const& rect = std::nullopt);
  static SurfacePtr from_reader(ReaderMapping const& mapping, std::optional<Rect> const& rect = std::nullopt, std::string const& filename = "");

private:
  Surface(TexturePtr const& diffuse_texture, TexturePtr const& displacement_texture, Flip flip, std::string const& filename = "");
  Surface(TexturePtr const& diffuse_texture, TexturePtr const& displacement_texture, Rect const& region, Flip flip, std::string const& filename = "");

public:
  ~Surface();

  SurfacePtr region(Rect const& rect) const;
  SurfacePtr clone(Flip flip = NO_FLIP) const;

  TexturePtr get_texture() const;
  TexturePtr get_displacement_texture() const;
  Rect get_region() const { return m_region; }
  int get_width() const;
  int get_height() const;
  Flip get_flip() const { return m_flip; }
  std::string get_filename() const { return m_source_filename; }

private:
  const TexturePtr m_diffuse_texture;
  const TexturePtr m_displacement_texture;
  const Rect m_region;
  const Flip m_flip;
  const std::string m_source_filename;

private:
  Surface& operator=(Surface const&) = delete;
};

#endif

/* EOF */
