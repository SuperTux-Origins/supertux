//  SuperTux
//  Copyright (C) 2009 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_SUPERTUX_OBJECT_GHOST_PARTICLE_SYSTEM_HPP
#define HEADER_SUPERTUX_OBJECT_GHOST_PARTICLE_SYSTEM_HPP

#include "object/particlesystem.hpp"
#include "video/surface_ptr.hpp"

class GhostParticleSystem final : public ParticleSystem
{
public:
  GhostParticleSystem();
  GhostParticleSystem(ReaderMapping const& reader);
  ~GhostParticleSystem() override;

  void init();
  void update(float dt_sec) override;


  const std::string get_icon_path() const override {
    return "images/engine/editor/ghostparticles.png";
  }

private:
  class GhostParticle : public Particle
  {
  public:
    float speed;

    GhostParticle() :
      speed()
    {}
  };

  SurfacePtr ghosts[2];

private:
  GhostParticleSystem(GhostParticleSystem const&) = delete;
  GhostParticleSystem& operator=(GhostParticleSystem const&) = delete;
};

#endif

/* EOF */
