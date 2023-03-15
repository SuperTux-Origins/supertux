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

#include "supertux/menu/main_menu.hpp"

#include <physfs.h>

#include "audio/sound_manager.hpp"
#include "gui/dialog.hpp"
#include "gui/menu_item.hpp"
#include "physfs/util.hpp"
#include "supertux/fadetoblack.hpp"
#include "supertux/game_manager.hpp"
#include "supertux/globals.hpp"
#include "supertux/levelset.hpp"
#include "supertux/menu/menu_storage.hpp"
#include "supertux/menu/sorted_contrib_menu.hpp"
#include "supertux/screen_manager.hpp"
#include "util/file_system.hpp"
#include "util/log.hpp"
#include "video/video_system.hpp"
#include "video/viewport.hpp"

#if defined(_WIN32)
  #include <windows.h>
  #include <shellapi.h>
#else
  #include <cstdlib>
#endif

#ifdef __EMSCRIPTEN__
#endif

MainMenu::MainMenu()
{
  set_center_pos(static_cast<float>(SCREEN_WIDTH) / 2.0f,
                 static_cast<float>(SCREEN_HEIGHT) / 2.0f + 35.0f);

  add_entry(MNID_STARTGAME, _("Start Game"));
  add_entry(MNID_CONTRIB, _("Contrib Levels"));
#ifdef __EMSCRIPTEN__
  add_entry(MNID_MANAGEASSETS, _("Manage Assets"));
#endif
  add_submenu(_("Options"), MenuStorage::OPTIONS_MENU);
  add_entry(MNID_CREDITS, _("Credits"));
#ifndef REMOVE_QUIT_BUTTON
  add_entry(MNID_QUITMAINMENU, _("Quit"));
#endif
}

void
MainMenu::on_window_resize()
{
  set_center_pos(static_cast<float>(SCREEN_WIDTH) / 2.0f,
                 static_cast<float>(SCREEN_HEIGHT) / 2.0f + 35.0f);
}

void
MainMenu::menu_action(MenuItem& item)
{
  switch (item.get_id())
  {

    case MNID_STARTGAME:
      {
        std::unique_ptr<World> world = World::from_directory("levels/world1");
        GameManager::current()->start_worldmap(*world);
      }
      break;

    case MNID_CONTRIB:
      {
        std::vector<std::string> level_worlds;
        std::unique_ptr<char*, decltype(&PHYSFS_freeList)>
          files(PHYSFS_enumerateFiles("levels"),
                PHYSFS_freeList);
        for (char const* const* filename = files.get(); *filename != nullptr; ++filename)
        {
          std::string filepath = FileSystem::join("levels", *filename);
          if (physfsutil::is_directory(filepath))
          {
            level_worlds.push_back(filepath);
          }
        }

        std::vector<std::unique_ptr<World> > contrib_worlds;

        for (std::vector<std::string>::const_iterator it = level_worlds.begin(); it != level_worlds.end(); ++it)
        {
          try
          {
            auto levelset =
              std::unique_ptr<Levelset>(new Levelset(*it, /* recursively = */ true));
            if (levelset->get_num_levels() == 0)
              continue;

            std::unique_ptr<World> world = World::from_directory(*it);
            if (!world->hide_from_contribs())
            {
              if (world->is_levelset() || world->is_worldmap())
              {
                contrib_worlds.push_back(std::move(world));
              }
              else
              {
                log_warning("unknown World type");
              }
            }
          }
          catch(std::exception& e)
          {
            log_info("Couldn't parse levelset info for '{}': {}", *it, e.what());
          }
        }

        auto contrib_menu = std::make_unique<SortedContribMenu>(contrib_worlds, "official", _("Contrib Levels"),
                                                                _("How is this possible? There are no Official Contrib Levels!"));
        MenuManager::instance().push_menu(std::move(contrib_menu));
      }
      break;

    case MNID_CREDITS:
      {
        // Credits Level
        SoundManager::current()->stop_music(0.2f);
        std::unique_ptr<World> world = World::from_directory("levels/misc");
        GameManager::current()->start_level(*world, "credits.stlv");
      }
      break;

    case MNID_QUITMAINMENU:
      MenuManager::instance().clear_menu_stack();
      ScreenManager::current()->quit(std::unique_ptr<ScreenFade>(new FadeToBlack(FadeToBlack::FADEOUT, 0.25f)));
      SoundManager::current()->stop_music(0.25);
      break;
  }
}

/* EOF */
