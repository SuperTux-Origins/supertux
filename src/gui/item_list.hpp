//  SuperTux
//  Copyright (C) 2022 mrkubax10
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

#ifndef HEADER_SUPERTUX_GUI_ITEM_LIST_HPP
#define HEADER_SUPERTUX_GUI_ITEM_LIST_HPP

#include "gui/menu_item.hpp"

#include <vector>
#include <string>

class ItemList final : public MenuItem
{
public:
  ItemList(std::string const& text, std::vector<std::string> const& items, std::string* value_ptr, int id = -1);

  virtual void process_action(MenuAction const& action) override;

private:
  std::vector<std::string> const& m_items;
  std::string* m_value_ptr;

private:
  ItemList(ItemList const&) = delete;
  ItemList& operator=(ItemList const&) = delete;
};

#endif
/* EOF */
