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

#include "supertux/moving_object.hpp"

#include "supertux/sector.hpp"
#include "util/reader_mapping.hpp"
#include "util/writer.hpp"

MovingObject::MovingObject() :
  m_col(COLGROUP_MOVING, *this)
{
}

MovingObject::MovingObject(const ReaderMapping& reader) :
  GameObject(reader),
  m_col(COLGROUP_MOVING, *this)
{
  float height, width;

  if (reader.read("width", width))
    m_col.m_bbox.set_width(width);

  if (reader.read("height", height))
    m_col.m_bbox.set_height(height);

  reader.read("x", m_col.m_bbox.get_left());
  reader.read("y", m_col.m_bbox.get_top());
}

MovingObject::~MovingObject()
{
}

void
MovingObject::on_flip(float height)
{
  Vector pos = get_pos();
  pos.y = height - pos.y - get_bbox().get_height();
  set_pos(pos);
}

/* EOF */
