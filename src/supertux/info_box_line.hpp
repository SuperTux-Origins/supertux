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

#ifndef HEADER_SUPERTUX_SUPERTUX_INFO_BOX_LINE_HPP
#define HEADER_SUPERTUX_SUPERTUX_INFO_BOX_LINE_HPP

#include <memory>
#include <string>
#include <vector>

#include "video/color.hpp"
#include "video/font_ptr.hpp"
#include "video/surface_ptr.hpp"

class DrawingContext;
class Rectf;

/**
 * Helper class for InfoBox: Represents a line of text
 */
class InfoBoxLine final
{
public:
  enum LineType { NORMAL, NORMAL_LEFT, SMALL, HEADING, REFERENCE, IMAGE};
  enum LineAlignment { LEFT, CENTER, RIGHT };

  InfoBoxLine(char format_char, std::string const& text);

  void draw(DrawingContext& context, Rectf const& bbox, int layer, LineAlignment alignment = CENTER);
  float get_height() const;

  static std::vector<std::unique_ptr<InfoBoxLine> > split(std::string const& text, float width);

  static bool is_valid_format_char(char format_char)
  {
    switch (format_char)
    {
      case ' ':
      case '-':
      case '*':
      case '\t':
      case '#':
      case '!':
        return true;
      default:
        return false;
    }
  }

private:
  InfoBoxLine::LineType lineType;
  FontPtr font;
  Color color;
  std::string text;
  SurfacePtr image;

private:
  InfoBoxLine(InfoBoxLine const&) = delete;
  InfoBoxLine& operator=(InfoBoxLine const&) = delete;
};

#endif

/* EOF */
