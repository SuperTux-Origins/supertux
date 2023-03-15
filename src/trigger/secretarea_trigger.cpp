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

#include "trigger/secretarea_trigger.hpp"

#include "audio/sound_manager.hpp"
#include "object/tilemap.hpp"
#include "supertux/level.hpp"
#include "supertux/resources.hpp"
#include "supertux/sector.hpp"
#include "util/reader_mapping.hpp"
#include "video/drawing_context.hpp"

static const float MESSAGE_TIME=3.5;

SecretAreaTrigger::SecretAreaTrigger(ReaderMapping const& reader) :
  TriggerBase(reader),
  message_timer(),
  message_displayed(false),
  message(),
  fade_tilemap(),
  script(),
  new_size(0.0f, 0.0f)
{
  reader.read("x", m_col.m_bbox.get_left());
  reader.read("y", m_col.m_bbox.get_top());
  float w,h;
  w = reader.get("width", 32.0f);
  h = reader.get("height", 32.0f);
  m_col.m_bbox.set_size(w, h);
  new_size.x = w;
  new_size.y = h;
  reader.read("fade-tilemap", fade_tilemap);
  reader.read("message", message);
  if (message.empty()) {
    message = _("You found a secret area!");
  }
  reader.read("script", script);
}

SecretAreaTrigger::SecretAreaTrigger(Rectf const& area, std::string const& fade_tilemap_) :
  message_timer(),
  message_displayed(false),
  message(_("You found a secret area!")),
  fade_tilemap(fade_tilemap_),
  script(),
  new_size(0.0f, 0.0f)
{
  m_col.m_bbox = area;
}

std::string
SecretAreaTrigger::get_fade_tilemap_name() const
{
  return fade_tilemap;
}

void
SecretAreaTrigger::draw(DrawingContext& context)
{
  if (message_timer.started()) {
    context.push_transform();
    context.set_translation(Vector(0, 0));
    context.transform().scale = 1.f;
    Vector pos = Vector(static_cast<float>(context.get_width()) / 2.0f, static_cast<float>(context.get_height()) / 2.0f - Resources::normal_font->get_height() / 2.0f);
    context.color().draw_text(Resources::normal_font, message, pos, FontAlignment::ALIGN_CENTER, LAYER_HUD, SecretAreaTrigger::text_color);
    context.pop_transform();
  }
  if (message_timer.check()) {
    remove_me();
  }
}

void
SecretAreaTrigger::event(Player& , EventType type)
{
  if (type == EVENT_TOUCH) {
    if (!message_displayed) {
      message_timer.start(MESSAGE_TIME);
      message_displayed = true;
      Sector::get().get_level().m_stats.increment_secrets();
      SoundManager::current()->play("sounds/welldone.ogg");

      if (!fade_tilemap.empty()) {
        // fade away tilemaps
        for (auto& tm : Sector::get().get_objects_by_type<TileMap>()) {
          if (tm.get_name() == fade_tilemap) {
            tm.fade(0.0, 1.0);
          }
        }
      }

      if (!script.empty()) {
        Sector::get().run_script(script, "SecretAreaScript");
      }
    }
  }
}

/* EOF */
