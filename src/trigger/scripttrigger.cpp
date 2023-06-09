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

#include "trigger/scripttrigger.hpp"

#include "supertux/debug.hpp"
#include "supertux/sector.hpp"
#include "util/log.hpp"
#include "util/reader_mapping.hpp"
#include "video/drawing_context.hpp"

ScriptTrigger::ScriptTrigger(ReaderMapping const& reader) :
  TriggerBase(reader),
  triggerevent(),
  script(),
  new_size(0.0f, 0.0f),
  must_activate(false),
  oneshot(false),
  runcount(0)
{
  if (m_col.m_bbox.get_width() == 0.f)
    m_col.m_bbox.set_width(32.f);

  if (m_col.m_bbox.get_height() == 0.f)
    m_col.m_bbox.set_height(32.f);

  reader.read("script", script);
  reader.read("button", must_activate);
  reader.read("oneshot", oneshot);
  if (script.empty()) {
    log_warning("No script set in script trigger");
  }

  if (must_activate)
    triggerevent = EVENT_ACTIVATE;
  else
    triggerevent = EVENT_TOUCH;
}

ScriptTrigger::ScriptTrigger(Vector const& pos, std::string const& script_) :
  TriggerBase(),
  triggerevent(EVENT_TOUCH),
  script(script_),
  new_size(0.0f, 0.0f),
  must_activate(),
  oneshot(false),
  runcount(0)
{
  m_col.m_bbox.set_pos(pos);
  m_col.m_bbox.set_size(32, 32);
}

void
ScriptTrigger::event(Player& , EventType type)
{
  if (type != triggerevent)
    return;

  if (oneshot && runcount >= 1) {
    return;
  }

  Sector::get().run_script(script, "ScriptTrigger");
  runcount++;
}

void
ScriptTrigger::draw(DrawingContext& context)
{
  if (g_debug.show_collision_rects) {
    context.color().draw_filled_rect(m_col.m_bbox, Color(1.0f, 0.0f, 1.0f, 0.6f),
                             0.0f, LAYER_OBJECTS);
  }
}

/* EOF */
