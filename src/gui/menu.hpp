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

#ifndef HEADER_SUPERTUX_GUI_MENU_HPP
#define HEADER_SUPERTUX_GUI_MENU_HPP

#include <functional>
#include <memory>
#include <SDL.h>

#include "gui/menu_action.hpp"
#include "math/vector.hpp"
#include "video/color.hpp"

class Controller;
class DrawingContext;
class ItemAction;
class ItemBack;
class ItemBadguySelect;
class ItemColor;
class ItemColorChannelRGBA;
class ItemColorChannelOKLab;
class ItemColorDisplay;
class ItemControlField;
class ItemFile;
class ItemFloatField;
class ItemGoTo;
class ItemHorizontalLine;
class ItemInactive;
class ItemIntField;
class ItemLabel;
class ItemPaths;
class ItemScript;
class ItemScriptLine;
class ItemList;
class ItemStringSelect;
class ItemTextField;
class ItemToggle;
class ItemStringArray;
class ItemImages;
class MenuItem;
class PathObject;

class Menu
{
public:
  Menu();
  virtual ~Menu();

  virtual void menu_action(MenuItem& item) = 0;

  /** Executed before the menu is exited
      @return true if it should perform the back action, false if it shouldn't */
  virtual bool on_back_action() { return true; }

  /** Perform actions to bring the menu up to date with configuration changes */
  virtual void refresh() {}

  virtual void on_window_resize();

  virtual void event(SDL_Event const& event);

  ItemHorizontalLine& add_hl();
  ItemLabel& add_label(std::string const& text);
  ItemAction& add_entry(int id, std::string const& text);
  ItemAction& add_entry(std::string const& text, std::function<void()> const& callback);
  ItemToggle& add_toggle(int id, std::string const& text, bool* toggled, bool center_text = false);
  ItemToggle& add_toggle(int id, std::string const& text,
                         std::function<bool()> const& get_func,
                         std::function<void(bool)> const& set_func,
                         bool center_text = false);
  ItemInactive& add_inactive(std::string const& text, bool default_color = false);
  ItemBack& add_back(std::string const& text, int id = -1);
  ItemGoTo& add_submenu(std::string const& text, int submenu, int id = -1);
  ItemControlField& add_controlfield(int id, std::string const& text, std::string const& mapping = "");
  ItemStringSelect& add_string_select(int id, std::string const& text, int* selected, std::vector<std::string> const& strings);
  ItemStringArray& add_string_array(std::string const& text, std::vector<std::string>& items, int id = -1);

  void process_input(Controller const& controller);

  /** Remove all entries from the menu */
  void clear();

  MenuItem& get_item(int index) { return *(m_items[index]); }

  MenuItem& get_item_by_id(int id);
  MenuItem const& get_item_by_id(int id) const;

  int get_active_item_id() const;
  void set_active_item(int id);

  void draw(DrawingContext& context);
  Vector get_center_pos() const { return m_pos; }
  void set_center_pos(float x, float y);

  float get_width() const;
  float get_height() const;

protected:
  /** returns true when the text is more important than action */
  virtual bool is_sensitive() const;

  MenuItem& add_item(std::unique_ptr<MenuItem> menu_item);
  MenuItem& add_item(std::unique_ptr<MenuItem> menu_item, int pos_);
  void delete_item(int pos_);

  /** Recalculates the width for this menu */
  void calculate_width();
  /** Recalculates the height for this menu */
  void calculate_height();

private:
  void process_action(MenuAction const& menuaction);
  void check_controlfield_change_event(SDL_Event const& event);
  void draw_item(DrawingContext& context, int index, float y_pos);

private:
  /** position of the menu (ie. center of the menu, not top/left) */
  Vector m_pos;

  /* input implementation variables */
  int m_delete_character;
  char m_mn_input_char;
  float m_menu_repeat_time;
  float m_menu_width;
  float m_menu_height;
  float m_menu_help_height;

public:
  std::vector<std::unique_ptr<MenuItem> > m_items;

private:
  int m_arrange_left;

protected:
  int m_active_item;

private:
  Menu(Menu const&) = delete;
  Menu& operator=(Menu const&) = delete;
};

#endif

/* EOF */
