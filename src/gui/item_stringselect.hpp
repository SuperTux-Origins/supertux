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

#ifndef HEADER_SUPERTUX_GUI_ITEM_STRINGSELECT_HPP
#define HEADER_SUPERTUX_GUI_ITEM_STRINGSELECT_HPP

#include <functional>

#include "gui/menu_item.hpp"

class ItemStringSelect final : public MenuItem
{
public:
  ItemStringSelect(std::string const& text, std::vector<std::string> items, int* selected, int id = -1);

  /** Draws the menu item. */
  void draw(DrawingContext&, Vector const& pos, int menu_width, bool active) override;

  /** Returns the minimum width of the menu item. */
  int get_width() const override;

  /** Processes the menu action. */
  void process_action(MenuAction const& action) override;

  bool changes_width() const override {
    return false;
  }

  void set_callback(std::function<void(int)> const& callback) {
    m_callback = callback;
  }

private:
  float calculate_width() const;

private:
  std::vector<std::string> m_items; // list of values for a STRINGSELECT item
  int* m_selected; // currently selected item
  std::function<void(int)> m_callback;
  float m_width;

private:
  ItemStringSelect(ItemStringSelect const&) = delete;
  ItemStringSelect& operator=(ItemStringSelect const&) = delete;
};

#endif

/* EOF */
