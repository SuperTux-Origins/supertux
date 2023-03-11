//  SuperTux
//  Copyright (C) 2006 Matthias Braun <matze@braunis.de>
//                2018 Ingo Ruhnke <grumbel@gmail.com>
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

#include "object/textscroller.hpp"

#include <optional>
#include <sexp/value.hpp>

#include "control/input_manager.hpp"
#include "supertux/globals.hpp"
#include "supertux/fadetoblack.hpp"
#include "supertux/info_box_line.hpp"
#include "supertux/screen_manager.hpp"
#include "supertux/sector.hpp"
#include "util/log.hpp"
#include "util/reader.hpp"
#include "util/reader_collection.hpp"
#include "util/reader_document.hpp"
#include "util/reader_mapping.hpp"
#include "video/drawing_context.hpp"
#include "video/video_system.hpp"
#include "video/viewport.hpp"

namespace {

const float LEFT_BORDER = 0;
const float DEFAULT_SPEED = 60;
const float SCROLL_JUMP = 60;

} // namespace

TextScroller::TextScroller(ReaderMapping const& mapping) :
  controller(&InputManager::current()->get_controller()),
  m_filename(),
  m_finish_script(),
  m_lines(),
  m_scroll(),
  m_default_speed(DEFAULT_SPEED),
  m_x_offset(),
  m_controllable(true),
  m_finished(),
  m_fading(),
  m_x_anchor(XAnchor::SCROLLER_ANCHOR_CENTER),
  m_text_align(TextAlign::SCROLLER_ALIGN_CENTER)
{
  mapping.must_read("file", m_filename);

  parse_file(m_filename);

  m_finish_script = mapping.get("finish-script", std::string());
  mapping.read("speed", m_default_speed);
  mapping.read("x-offset", m_x_offset);
  m_controllable = mapping.get("controllable", true);

  std::string x_anchor_str;
  if (mapping.read("x-anchor", x_anchor_str))
  {
    if (x_anchor_str == "left")
      m_x_anchor = XAnchor::SCROLLER_ANCHOR_LEFT;
    else if (x_anchor_str == "right")
      m_x_anchor = XAnchor::SCROLLER_ANCHOR_RIGHT;
    else
      m_x_anchor = XAnchor::SCROLLER_ANCHOR_CENTER;
  }

  std::string text_align_str;
  if (mapping.read("text-align", text_align_str))
  {
    if (text_align_str == "left")
      m_text_align = TextAlign::SCROLLER_ALIGN_LEFT;
    else if (text_align_str == "right")
      m_text_align = TextAlign::SCROLLER_ALIGN_RIGHT;
    else
      m_text_align = TextAlign::SCROLLER_ALIGN_CENTER;
  }

}

TextScroller::TextScroller(ReaderObject const& root) :
  controller(&InputManager::current()->get_controller()),
  m_filename(),
  m_finish_script(),
  m_lines(),
  m_scroll(),
  m_default_speed(DEFAULT_SPEED),
  m_x_offset(),
  m_controllable(true),
  m_finished(),
  m_fading(),
  m_x_anchor(XAnchor::SCROLLER_ANCHOR_CENTER),
  m_text_align(TextAlign::SCROLLER_ALIGN_CENTER)
{
  parse_root(root);
}

void
TextScroller::parse_file(std::string const& filename)
{
  auto doc = load_reader_document(filename);
  auto root = doc.get_root();
  parse_root(root);
}

void
TextScroller::parse_root(ReaderObject const& root)
{
  if (root.get_name() != "supertux-text")
  {
    throw std::runtime_error("File isn't a supertux-text file");
  }
  else
  {
    auto mapping = root.get_mapping();

    int version = 1;
    mapping.read("version", version);
    if (version == 1)
    {
      log_info("[{}] Text uses old format: version 1", mapping.get_document().get_filename());

      std::string text;
      if (!mapping.read("text", text)) {
        throw std::runtime_error("File doesn't contain a text field");
      }

      // Split text string lines into a vector
      m_lines = InfoBoxLine::split(text, static_cast<float>(SCREEN_WIDTH) - 2.0f * LEFT_BORDER);
    }
    else if (version == 2)
    {
      ReaderCollection content_collection;
      if (!mapping.read("content", content_collection)) {
        throw std::runtime_error("File doesn't contain content");
      } else {
        parse_content(content_collection);
      }
    }
  }
}

void
TextScroller::parse_content(ReaderCollection const& collection)
{
  for (auto const& item : collection.get_objects())
  {
    if (item.get_name() == "image")
    {
      ReaderMapping const& mapping = item.get_mapping();
      std::string image_file;
      mapping.read("file", image_file);
      m_lines.emplace_back(new InfoBoxLine('!', image_file));
    }
    else if (item.get_name() == "person")
    {
      bool simple;
      std::string name, info, image_file;

      ReaderMapping const& mapping = item.get_mapping();
      simple = mapping.get("simple", false);

      if (simple) {
        if (!mapping.read("name", name) ||
            !mapping.read("info", info))
        {
          throw std::runtime_error("Simple entry requires both name and info specified");
        }

        if (mapping.read("image", image_file)) {
          log_warning("[{}] Simple person entry shouldn't specify images", collection.get_document().get_filename());
        }

        m_lines.emplace_back(new InfoBoxLine(' ', name + " (" + info + ")")); // NOLINT
      } else {
        if (mapping.read("name", name)) {
          m_lines.emplace_back(new InfoBoxLine('\t', name));
        }

        if (mapping.read("image", image_file)) {
          m_lines.emplace_back(new InfoBoxLine('!', image_file));
        }

        if (mapping.read("info", info)) {
          m_lines.emplace_back(new InfoBoxLine(' ', info));
        }
      }
    }
    else if (item.get_name() == "blank")
    {
      // Empty line
      m_lines.emplace_back(new InfoBoxLine('\t', ""));
    }
    else if (item.get_name() == "text")
    {
      std::string type, string;

      ReaderMapping const& mapping = item.get_mapping();
      if (!mapping.read("type", type)) {
        type = "normal";
      }

      if (!mapping.read("string", string)) {
        throw std::runtime_error("Text entry requires a string");
      }

      if (type == "normal")
        m_lines.emplace_back(new InfoBoxLine('\t', string));
      else if (type == "normal-left")
        m_lines.emplace_back(new InfoBoxLine('#', string));
      else if (type == "small")
        m_lines.emplace_back(new InfoBoxLine(' ', string));
      else if (type == "heading")
        m_lines.emplace_back(new InfoBoxLine('-', string));
      else if (type == "reference")
        m_lines.emplace_back(new InfoBoxLine('*', string));
      else {
        log_warning("[{}] Unknown text type '{}'", item.get_document().get_filename(), type);
        m_lines.emplace_back(new InfoBoxLine('\t', string));
      }
    }
    else
    {
      log_warning("[{}] Unknown token '{}'", item.get_document().get_filename(), item.get_name());
    }
  }
}

void
TextScroller::draw(DrawingContext& context)
{
  context.push_transform();
  context.set_translation(Vector(0, 0));
  context.transform().scale = 1.f;

  const float ctx_w = static_cast<float>(context.get_width());
  const float ctx_h = static_cast<float>(context.get_height());

  float y = floorf(ctx_h - m_scroll);

  { // draw text
    for (auto const& line : m_lines)
    {
      if (y + line->get_height() >= 0 && ctx_h - y >= 0) {
        line->draw(context, Rectf(LEFT_BORDER, y, ctx_w * (m_x_anchor == XAnchor::SCROLLER_ANCHOR_LEFT ? 0.f :
           m_x_anchor == XAnchor::SCROLLER_ANCHOR_RIGHT ? 2.f : 1.f) + m_x_offset, y), LAYER_GUI,
          (m_text_align == TextAlign::SCROLLER_ALIGN_LEFT ? line->LineAlignment::LEFT :
           m_text_align == TextAlign::SCROLLER_ALIGN_RIGHT ? line->LineAlignment::RIGHT :
           line->LineAlignment::CENTER));
      }

      y += floorf(line->get_height());
    }
  }

  context.pop_transform();

  // close when done
  if (y < 0)
  {
    m_finished = true;
    set_default_speed(0.f);
  }
}

void
TextScroller::update(float dt_sec)
{
  float speed = m_default_speed;

  if (controller && m_controllable) {
    // allow changing speed with up and down keys
    if (controller->hold(Control::UP)) {
      speed = -m_default_speed * 5;
    } else if (controller->hold(Control::DOWN)) {
      speed = m_default_speed * 5;
    }

    // allow jumping ahead with certain keys
    if ((controller->pressed(Control::JUMP) ||
         controller->pressed(Control::ACTION) ||
         controller->pressed(Control::MENU_SELECT)) &&
        !(controller->pressed(Control::UP))) { // prevent skipping if jump with up is enabled
      scroll(SCROLL_JUMP);
    }

    // use start or escape keys to exit
    if ((controller->pressed(Control::START) ||
        controller->pressed(Control::ESCAPE)) &&
        !m_fading  && m_finish_script.empty()) {
      m_fading = true;
      ScreenManager::current()->pop_screen(std::make_unique<FadeToBlack>(FadeToBlack::FADEOUT, 0.5f));
      return;
    }
  }

  m_scroll += speed * dt_sec;

  if (m_scroll < 0)
    m_scroll = 0;
  if (!m_finish_script.empty())
  {
    Sector::get().run_script(m_finish_script, "finishscript");
  }
  else
  {
    // close when done
    if (m_finished && !m_fading)
    {
	  m_fading = true;
      ScreenManager::current()->pop_screen(std::unique_ptr<ScreenFade>(new FadeToBlack(FadeToBlack::FADEOUT, 0.25f)));
    }
  }
}

void
TextScroller::set_default_speed(float default_speed)
{
  m_default_speed = default_speed;
}

void
TextScroller::scroll(float offset)
{
  m_scroll += offset;
  if (m_scroll < 0.0f)
  {
    m_scroll = 0.0f;
  }
}

/* EOF */
