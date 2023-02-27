//  SuperTux
//  Copyright (C) 2021 mrkubax10 <mrkubax10@onet.pl>
//                2021 A. Semphris <semphris@protonmail.com>
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

#include "trigger/text_area.hpp"

#include "math/anchor_point.hpp"
#include "object/player.hpp"
#include "object/text_object.hpp"
#include "supertux/sector.hpp"
#include "util/reader_mapping.hpp"
#include "video/drawing_context.hpp"
#include "video/layer.hpp"

TextArea::TextArea(const ReaderMapping& mapping) :
  TriggerBase(mapping),
  m_once(false),
  m_items(),
  m_delay(4.0f),
  m_fade_delay(1.0f),
  m_current_text(0),
  m_status(Status::NOT_STARTED),
  m_timer(),
  m_anchor(AnchorPoint::ANCHOR_MIDDLE),
  m_anchor_offset(0, 0)
{
  float w, h;

  m_col.m_bbox.get_left() = mapping.get("x", 0.0f);
  m_col.m_bbox.get_top() = mapping.get("y", 0.0f);
  w = mapping.get("width", 32.0f);
  h = mapping.get("height", 32.0f);
  mapping.read("strings", m_items);
  mapping.read("delay", m_delay);
  mapping.read("once", m_once);
  mapping.read("fade-delay", m_fade_delay);
  mapping.read("anchor-offset-x", m_anchor_offset.x);
  mapping.read("anchor-offset-y", m_anchor_offset.y);

  std::string anchor;
  if (mapping.read("anchor-point", anchor)) {
    m_anchor = string_to_anchor_point(anchor);
  }

  m_col.m_bbox.set_size(w, h);
}

TextArea::TextArea(const Vector& pos) :
  m_once(false),
  m_items(),
  m_delay(4.0f),
  m_fade_delay(1.0f),
  m_current_text(0),
  m_status(Status::NOT_STARTED),
  m_timer(),
  m_anchor(AnchorPoint::ANCHOR_MIDDLE)
{
  m_col.m_bbox.set_pos(pos);
  m_col.m_bbox.set_size(32, 32);
}

void
TextArea::draw(DrawingContext& context)
{
}

void
TextArea::event(Player& player, EventType type)
{
  switch (type)
  {
    case EVENT_TOUCH:
      if (m_status == Status::NOT_STARTED)
      {
        if (m_items.size() < 1)
        {
          log_warning << "Attempt to run a TextArea with no text, aborting" << std::endl;
          return;
        }

        TextObject& text_object = Sector::get().get_singleton_by_type<TextObject>();

        m_current_text = 0;
        m_status = Status::FADING_IN;
        m_timer.start(m_fade_delay);
        text_object.set_anchor_point(m_anchor);
        text_object.set_anchor_offset(m_anchor_offset);
        text_object.set_text(m_items[m_current_text]);
        text_object.fade_in(m_fade_delay);
      }
      break;

    default:
      break;
  }
}

void
TextArea::update(float dt_sec)
{
  TriggerBase::update(dt_sec);

  if (m_timer.check())
  {
    TextObject& text_object = Sector::get().get_singleton_by_type<TextObject>();

    switch(m_status)
    {
      case Status::FADING_IN:
        m_status = Status::WAITING;
        m_timer.start(m_delay);
        break;

      case Status::WAITING:
        m_status = Status::FADING_OUT;
        m_timer.start(m_fade_delay);
        text_object.fade_out(m_fade_delay);
        break;

      case Status::FADING_OUT:
        if (++m_current_text >= m_items.size())
        {
          m_current_text = 0;
          m_status = m_once ? Status::FINISHED : Status::NOT_STARTED;
        }
        else
        {
          m_status = Status::FADING_IN;
          m_timer.start(m_fade_delay);
          text_object.set_anchor_point(m_anchor);
          text_object.set_anchor_offset(m_anchor_offset);
          text_object.set_text(m_items[m_current_text]);
          text_object.fade_in(m_fade_delay);
        }
        break;

      default:
        break;
    }
  }
}

/* EOF */
