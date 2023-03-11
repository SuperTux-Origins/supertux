//  SuperTux
//  Copyright (C) 2020 A. Semphris <semphris@protonmail.com>
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

#ifndef HEADER_SUPERTUX_OBJECT_CUSTOM_PARTICLE_SYSTEM_FILE_HPP
#define HEADER_SUPERTUX_OBJECT_CUSTOM_PARTICLE_SYSTEM_FILE_HPP

#include "math/easing.hpp"
#include "math/vector.hpp"
#include "object/custom_particle_system.hpp"
#include "object/particle_zone.hpp"
#include "object/particlesystem_interactive.hpp"
#include "scripting/custom_particles.hpp"
#include "video/surface.hpp"
#include "video/surface_ptr.hpp"

class CustomParticleSystemFile final :
  public CustomParticleSystem
{
public:
  CustomParticleSystemFile();
  CustomParticleSystemFile(ReaderMapping const& reader);
  ~CustomParticleSystemFile() override;


  const std::string get_icon_path() const override {
    return "images/engine/editor/sparkle-file.png";
  }

private:
  void update_data();

private:
  std::string m_filename;

private:
  CustomParticleSystemFile(CustomParticleSystemFile const&) = delete;
  CustomParticleSystemFile& operator=(CustomParticleSystemFile const&) = delete;
};

#endif

/* EOF */
