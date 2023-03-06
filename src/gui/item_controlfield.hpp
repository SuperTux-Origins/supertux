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

#ifndef HEADER_SUPERTUX_GUI_ITEM_CONTROLFIELD_HPP
#define HEADER_SUPERTUX_GUI_ITEM_CONTROLFIELD_HPP

#include "gui/menu_item.hpp"

class ItemControlField final : public MenuItem
{
public:
  ItemControlField(std::string const& text_, std::string const& input_, int id = -1);

  /** Draws the menu item. */
  virtual void draw(DrawingContext&, Vector const& pos, int menu_width, bool active) override;

  /** Returns the minimum width of the menu item. */
  virtual int get_width() const override;

  std::string input;

  void change_input(std::string const& input_) {
    input = input_;
  }

  virtual bool changes_width() const override {
    return true;
  }

private:
  ItemControlField(ItemControlField const&) = delete;
  ItemControlField& operator=(ItemControlField const&) = delete;
};

#endif

/* EOF */
