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

#include "supertux/menu/menu_storage.hpp"

#include <assert.h>

#include "supertux/menu/cheat_menu.hpp"
#include "supertux/menu/debug_menu.hpp"
#include "supertux/menu/custom_menu_menu.hpp"
#include "supertux/menu/game_menu.hpp"
#include "supertux/menu/joystick_menu.hpp"
#include "supertux/menu/keyboard_menu.hpp"
#include "supertux/menu/main_menu.hpp"
#include "supertux/menu/multiplayer_menu.hpp"
#include "supertux/menu/multiplayer_players_menu.hpp"
#include "supertux/menu/options_menu.hpp"
#include "supertux/menu/web_asset_menu.hpp"
#include "supertux/menu/worldmap_menu.hpp"
#include "supertux/menu/worldmap_cheat_menu.hpp"
#include "util/log.hpp"

MenuStorage* MenuStorage::s_instance = nullptr;

MenuStorage&
MenuStorage::instance()
{
  assert(s_instance);
  return *s_instance;
}

MenuStorage::MenuStorage()
{
  assert(!s_instance);
  s_instance = this;
}

MenuStorage::~MenuStorage()
{
  s_instance = nullptr;
}

std::unique_ptr<Menu>
MenuStorage::create(MenuId menu_id)
{
  switch (menu_id)
  {
    case MAIN_MENU:
      return std::make_unique<MainMenu>();

    case OPTIONS_MENU:
      return std::unique_ptr<Menu>(new OptionsMenu(true));

    case INGAME_OPTIONS_MENU:
      return std::unique_ptr<Menu>(new OptionsMenu(false));

    case KEYBOARD_MENU:
      return std::unique_ptr<Menu>(new KeyboardMenu(*InputManager::current()));

    case JOYSTICK_MENU:
      return std::unique_ptr<Menu>(new JoystickMenu(*InputManager::current()));

    case WORLDMAP_MENU:
      return std::make_unique<WorldmapMenu>();

    case WORLDMAP_CHEAT_MENU:
      return std::make_unique<WorldmapCheatMenu>();

    case WORLDMAP_LEVEL_SELECT_MENU:
      return std::make_unique<WorldmapLevelSelectMenu>();

    case GAME_MENU:
      return std::make_unique<GameMenu>();

    case CHEAT_MENU:
      return std::make_unique<CheatMenu>();

    case DEBUG_MENU:
      return std::make_unique<DebugMenu>();

    case ASSET_MENU:
      return std::make_unique<WebAssetMenu>();

    case CUSTOM_MENU_MENU:
      return std::make_unique<CustomMenuMenu>();

    case MULTIPLAYER_MENU:
      return std::make_unique<MultiplayerMenu>();

    case MULTIPLAYER_PLAYERS_MENU:
      return std::make_unique<MultiplayerPlayersMenu>();

    case NO_MENU:
      return std::unique_ptr<Menu>();

    default:
      log_warning << "unknown MenuId provided: " << menu_id << std::endl;
      assert(false);
      return std::unique_ptr<Menu>();
  }
}

/* EOF */
