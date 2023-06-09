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

#include "gui/item_action.hpp"

ItemAction::ItemAction(std::string const& text, int id, std::function<void()> callback) :
  MenuItem(text, id),
  m_callback(std::move(callback))
{
}

void
ItemAction::process_action(MenuAction const& action)
{
  if (action == MenuAction::HIT) {
    if (m_callback) {
      m_callback();
    }
  }
}

/* EOF */
