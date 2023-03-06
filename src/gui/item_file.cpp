//  SuperTux
//  Copyright (C) 2016 Hume2 <teratux.mail@gmail.com>
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

#include "gui/item_file.hpp"

#include "gui/menu.hpp"
#include "gui/menu_filesystem.hpp"
#include "gui/menu_manager.hpp"

ItemFile::ItemFile(std::string const& text, std::string* filename,
                   std::vector<std::string> const& extensions,
                   std::string const& basedir,
                   bool path_relative_to_basedir,
                   int id) :
  MenuItem(text, id),
  m_filename(filename),
  m_extensions(extensions),
  m_basedir(basedir),
  m_path_relative_to_basedir(path_relative_to_basedir)
{
}

void
ItemFile::process_action(MenuAction const& action)
{
  if (action == MenuAction::HIT) {
    MenuManager::instance().push_menu(std::make_unique<FileSystemMenu>(m_filename, m_extensions, m_basedir, m_path_relative_to_basedir));
  }
}

/* EOF */
