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

#ifndef HEADER_SUPERTUX_GUI_ITEM_TOGGLE_HPP
#define HEADER_SUPERTUX_GUI_ITEM_TOGGLE_HPP

#include <functional>

#include "gui/menu_item.hpp"

class ItemToggle final : public MenuItem
{
public:
  ItemToggle(std::string const& text, bool* toggled, int id = -1, bool center_text = false);
  ItemToggle(std::string const& text,
             std::function<bool()> get_func,
             std::function<void(bool)> set_func,
             int id = -1,
             bool center_text = false);

  /** Draws the menu item. */
  void draw(DrawingContext&, Vector const& pos, int menu_width, bool active) override;

  /** Returns the minimum width of the menu item. */
  int get_width() const override;

  /** Processes the menu action. */
  void process_action(MenuAction const& action) override;

private:
  const bool m_center_text;
  std::function<bool()> m_get_func;
  std::function<void(bool)> m_set_func;

private:
  ItemToggle(ItemToggle const&) = delete;
  ItemToggle& operator=(ItemToggle const&) = delete;
};

#endif

/* EOF */
