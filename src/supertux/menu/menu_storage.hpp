//  SuperTux
//  Copyright (C) 2009 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_SUPERTUX_SUPERTUX_MENU_MENU_STORAGE_HPP
#define HEADER_SUPERTUX_SUPERTUX_MENU_MENU_STORAGE_HPP

#include <memory>

class JoystickMenu;
class KeyboardMenu;
class Menu;
class OptionsMenu;

class MenuStorage final
{
private:
  static MenuStorage* s_instance;
public:
  static MenuStorage& instance();

public:
  enum MenuId {
    NO_MENU,
    MAIN_MENU,
    OPTIONS_MENU,
    INGAME_OPTIONS_MENU,
    ADDON_MENU,
    KEYBOARD_MENU,
    JOYSTICK_MENU,
    WORLDMAP_MENU,
    WORLDMAP_CHEAT_MENU,
    WORLDMAP_LEVEL_SELECT_MENU,
    GAME_MENU,
    CHEAT_MENU,
    DEBUG_MENU,
    EDITOR_LEVELSET_SELECT_MENU,
    EDITOR_NEW_LEVELSET_MENU,
    EDITOR_LEVEL_SELECT_MENU,
    EDITOR_MENU,
    EDITOR_TILEGROUP_MENU,
    EDITOR_OBJECTGROUP_MENU,
    EDITOR_SECTORS_MENU,
    EDITOR_SECTOR_MENU,
    EDITOR_LEVEL_MENU,
    EDITOR_LEVELSET_MENU,
    PARTICLE_EDITOR_MENU,
    PARTICLE_EDITOR_SAVE_AS,
    PARTICLE_EDITOR_OPEN,
    ASSET_MENU,
    MULTIPLAYER_MENU,
    MULTIPLAYER_PLAYERS_MENU
  };

public:
  MenuStorage();
  ~MenuStorage();

  std::unique_ptr<Menu> create(MenuId menu_id);

private:
  MenuStorage(MenuStorage const&) = delete;
  MenuStorage& operator=(MenuStorage const&) = delete;
};

#endif

/* EOF */
