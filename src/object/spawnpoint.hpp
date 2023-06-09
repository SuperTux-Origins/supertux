//  SuperTux
//  Copyright (C) 2015 Hume2 <teratux.mail@gmail.com>
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

#ifndef HEADER_SUPERTUX_OBJECT_SPAWNPOINT_HPP
#define HEADER_SUPERTUX_OBJECT_SPAWNPOINT_HPP

#include "supertux/moving_object.hpp"

#include "video/layer.hpp"
#include "video/surface_ptr.hpp"

class DrawingContext;

class SpawnPointMarker final : public MovingObject
{
public:
  SpawnPointMarker(std::string const& name, Vector const& pos);
  SpawnPointMarker(ReaderMapping const& mapping);

  void update(float dt_sec) override {
    // No updates needed
  }

  void draw(DrawingContext& context) override;

  void collision_solid(CollisionHit const& hit) override {
    // This function wouldn't be called anyway.
  }

  HitResponse collision(GameObject& other, CollisionHit const& hit) override { return FORCE_MOVE; }


  int get_layer() const override { return LAYER_FOREGROUND1; }

private:
  SurfacePtr m_surface;

private:
  SpawnPointMarker(SpawnPointMarker const&) = delete;
  SpawnPointMarker& operator=(SpawnPointMarker const&) = delete;
};

#endif

/* EOF */
