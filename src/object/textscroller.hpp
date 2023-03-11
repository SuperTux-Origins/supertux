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

#ifndef HEADER_SUPERTUX_OBJECT_TEXTSCROLLER_HPP
#define HEADER_SUPERTUX_OBJECT_TEXTSCROLLER_HPP

#include <memory>
#include <vector>

#include "control/controller.hpp"
#include "supertux/game_object.hpp"
#include "supertux/info_box_line.hpp"

class DrawingContext;
class InfoBoxLine;

class TextScroller : public GameObject
{
public:
  TextScroller(ReaderMapping const& mapping);
  TextScroller(ReaderObject const& root);

  void draw(DrawingContext& context) override;
  void update(float dt_sec) override;

  const std::string get_icon_path() const override { return "images/engine/editor/textscroller.png"; }

  void set_default_speed(float default_speed);
  void scroll(float offset);
  bool is_finished() const { return m_finished; }

protected:
  Controller const* controller;

private:
  void parse_file(std::string const& filename);
  void parse_root(ReaderObject const& root);
  void parse_content(ReaderCollection const& collection);

private:
  std::string m_filename;
  std::string m_finish_script;
  std::vector<std::unique_ptr<InfoBoxLine> > m_lines;
  float m_scroll;
  float m_default_speed;
  float m_x_offset;
  bool m_controllable;
  bool m_finished;
  bool m_fading;

  enum XAnchor {
    SCROLLER_ANCHOR_LEFT,
    SCROLLER_ANCHOR_CENTER,
    SCROLLER_ANCHOR_RIGHT
  };
  enum TextAlign {
    SCROLLER_ALIGN_LEFT,
    SCROLLER_ALIGN_CENTER,
    SCROLLER_ALIGN_RIGHT
  };

  XAnchor m_x_anchor;
  TextAlign m_text_align;

private:
  TextScroller(TextScroller const&) = delete;
  TextScroller& operator=(TextScroller const&) = delete;
};

#endif

/* EOF */
