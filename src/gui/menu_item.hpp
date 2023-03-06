//  SuperTux
//  Copyright (C) 2006 Matthias Braun <matze@braunis.de>
//                2015 Hume2 <teratux.mail@gmail.com>
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

#ifndef HEADER_SUPERTUX_GUI_MENU_ITEM_HPP
#define HEADER_SUPERTUX_GUI_MENU_ITEM_HPP

#include "gui/menu.hpp"

class MenuItem
{
public:
  MenuItem(std::string const& text, int id = -1);
  virtual ~MenuItem();

  int get_id() const { return m_id; }

  void set_help(std::string const& help_text);
  std::string const& get_help() const { return m_help; }

  void set_text(std::string const& text) { m_text = text; }
  std::string const& get_text() const { return m_text; }

  /** Draws the menu item. */
  virtual void draw(DrawingContext&, Vector const& pos, int menu_width, bool active);

  /** Returns true when the menu item has no action and therefore can be skipped.
      Useful for labels and horizontal lines.*/
  virtual bool skippable() const {
    return false;
  }

  /** Returns the minimum width of the menu item. */
  virtual int get_width() const;

  /** Returns height of menu item. */
  virtual int get_height() const { return 24; }

  /** Processes the menu action. */
  virtual void process_action(MenuAction const& action) { }

  /** Processes the given event. */
  virtual void event(SDL_Event const& ev) { }

  virtual Color get_color() const;

  /** Returns true when the MenuManager shouldn't do anything else. */
  virtual bool no_other_action() const {
    return false;
  }

  /** Returns true when the width must be recalculated when an action is
      processed */
  virtual bool changes_width() const {
    return false;
  }

private:
  int m_id;
  std::string m_text;
  std::string m_help;

private:
  MenuItem(MenuItem const&) = delete;
  MenuItem& operator=(MenuItem const&) = delete;
};

#endif

/* EOF */
