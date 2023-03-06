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

#include "object/spawnpoint.hpp"

#include "supertux/debug.hpp"
#include "util/reader_mapping.hpp"
#include "video/drawing_context.hpp"
#include "video/surface.hpp"

SpawnPointMarker::SpawnPointMarker(std::string const& name, Vector const& pos) :
  m_surface(Surface::from_file("images/engine/editor/spawnpoint.png"))
{
  m_name = name;
  m_col.m_bbox.set_p1(pos);
  m_col.m_bbox.set_size(32, 32);

  set_group(COLGROUP_DISABLED);
}

SpawnPointMarker::SpawnPointMarker(ReaderMapping const& mapping) :
  m_surface(Surface::from_file("images/engine/editor/spawnpoint.png"))
{
  m_name = mapping.get("name", std::string());
  m_col.m_bbox.get_left() = mapping.get("x", 0.0f);
  m_col.m_bbox.get_top() = mapping.get("y", 0.0f);

  m_col.m_bbox.set_size(32, 32);
  set_group(COLGROUP_DISABLED);
}

void
SpawnPointMarker::draw(DrawingContext& context)
{
  if (g_debug.show_collision_rects)
  {
    context.color().draw_surface(m_surface, m_col.m_bbox.p1(), LAYER_FOREGROUND1);
  }
}

/* EOF */
