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

#include "trigger/sequence_trigger.hpp"

#include "object/player.hpp"
#include "supertux/debug.hpp"
#include "util/reader_mapping.hpp"
#include "util/writer.hpp"
#include "video/drawing_context.hpp"

SequenceTrigger::SequenceTrigger(ReaderMapping const& reader) :
  triggerevent(EVENT_TOUCH),
  sequence(SEQ_ENDSEQUENCE),
  new_size(0.0f, 0.0f),
  new_spawnpoint(),
  fade_tilemap(),
  fade()
{
  m_col.m_bbox.get_left() = reader.get("x", 0.0f);
  m_col.m_bbox.get_top() = reader.get("y", 0.0f);
  float w, h;
  w = reader.get("width", 32.0f);
  h = reader.get("height", 32.0f);
  m_col.m_bbox.set_size(w, h);
  new_size.x = w;
  new_size.y = h;
  std::string sequence_name;
  if (reader.read("sequence", sequence_name)) {
    sequence = string_to_sequence(sequence_name);
  }

  reader.read("new_spawnpoint", new_spawnpoint);
  reader.read("fade_tilemap", fade_tilemap);
  reader.read("fade", reinterpret_cast<int&>(fade));
}

SequenceTrigger::SequenceTrigger(Vector const& pos, std::string const& sequence_name) :
  triggerevent(EVENT_TOUCH),
  sequence(string_to_sequence(sequence_name)),
  new_size(0.0f, 0.0f),
  new_spawnpoint(),
  fade_tilemap(),
  fade()
{
  m_col.m_bbox.set_pos(pos);
  m_col.m_bbox.set_size(32, 32);
}

void
SequenceTrigger::event(Player& player, EventType type)
{
  if (type == triggerevent) {
    auto data = SequenceData(new_spawnpoint, fade_tilemap, fade);
    player.trigger_sequence(sequence, &data);
  }
}

std::string
SequenceTrigger::get_sequence_name() const
{
  return sequence_to_string(sequence);
}

void
SequenceTrigger::draw(DrawingContext& context)
{
  if (g_debug.show_collision_rects) {
    context.color().draw_filled_rect(m_col.m_bbox, Color(1.0f, 0.0f, 0.0f, 0.6f),
                             0.0f, LAYER_OBJECTS);
  }
}

/* EOF */
