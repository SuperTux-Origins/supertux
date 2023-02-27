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

#include "object/invisible_wall.hpp"

#include "util/reader_mapping.hpp"
#include "video/drawing_context.hpp"

InvisibleWall::InvisibleWall(const ReaderMapping& mapping):
  MovingObject(mapping),
  width(),
  height()
{
  m_col.m_bbox.get_left() = mapping.get("x", 0.0f);
  m_col.m_bbox.get_top() = mapping.get("y", 0.0f);
  width = mapping.get("width", 32.0f);
  height = mapping.get("height", 32.0f);

  m_col.m_bbox.set_size(width, height);

  m_col.m_group = COLGROUP_STATIC;
}

HitResponse
InvisibleWall::collision(GameObject& , const CollisionHit& )
{
  return FORCE_MOVE;
}

void
InvisibleWall::draw(DrawingContext& context)
{
}

void
InvisibleWall::update(float )
{
}

/* EOF */
