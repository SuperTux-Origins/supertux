//  SuperTux
//  Copyright (C) 2016 Hume2 <teratux.mail@gmail.com>
//                2022 Vankata453
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

#ifndef HEADER_SUPERTUX_GUI_ITEM_SCRIPT_LINE_HPP
#define HEADER_SUPERTUX_GUI_ITEM_SCRIPT_LINE_HPP

#include "gui/item_textfield.hpp"

class ItemScriptLine final : public ItemTextField
{
public:
  ItemScriptLine(std::string* input_, int id_ = -1);

  /** Draws the menu item. */
  virtual void draw(DrawingContext&, Vector const& pos, int menu_width, bool active) override;

  /** Returns the minimum width of the menu item. */
  virtual int get_width() const override;

  /** Processes the menu action. */
  virtual void process_action(MenuAction const& action) override;

  /** Processes the given event. */
  virtual void event(SDL_Event const& ev) override;

  /** Calls when the user wants to remove an invalid char. */
  virtual void invalid_remove() override;

  // Text manipulation and navigation functions

  virtual void paste() override;
  virtual void new_line();
  virtual void duplicate_line();

private:
  ItemScriptLine(ItemScriptLine const&) = delete;
  ItemScriptLine& operator=(ItemScriptLine const&) = delete;
};

#endif

/* EOF */
