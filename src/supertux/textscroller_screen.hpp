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

#ifndef HEADER_SUPERTUX_SUPERTUX_TEXTSCROLLER_SCREEN_HPP
#define HEADER_SUPERTUX_SUPERTUX_TEXTSCROLLER_SCREEN_HPP

#include <memory>

#include "supertux/screen.hpp"
#include "video/color.hpp"
#include "video/surface_ptr.hpp"

class DrawingContext;
class InfoBoxLine;
class TextScroller;

/**
 * Screen that displays intro text, extro text, etc.
 */
class TextScrollerScreen final : public Screen
{
public:
  TextScrollerScreen(std::string const& file);
  ~TextScrollerScreen() override;

  void setup() override;
  void draw(Compositor& compositor) override;
  void update(float dt_sec, Controller const& controller) override;

private:
  float m_defaultspeed;
  std::string m_music;
  SurfacePtr m_background;
  std::unique_ptr<TextScroller> m_text_scroller;

private:
  TextScrollerScreen(TextScrollerScreen const&) = delete;
  TextScrollerScreen& operator=(TextScrollerScreen const&) = delete;
};

#endif

/* EOF */
