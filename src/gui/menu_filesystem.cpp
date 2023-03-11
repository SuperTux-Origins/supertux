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

#include "gui/menu_filesystem.hpp"

#include <physfs.h>

#include "gui/menu_item.hpp"
#include "gui/menu_manager.hpp"
#include "physfs/util.hpp"
#include "util/file_system.hpp"
#include "util/gettext.hpp"
#include "util/log.hpp"

FileSystemMenu::FileSystemMenu(std::string* filename, std::vector<std::string> const& extensions,
                               std::string const& basedir, bool path_relative_to_basedir, std::function<void(std::string)> callback) :
  m_filename(filename),
  // when a basedir is given, 'filename' is relative to basedir, so
  // it's useless as a starting point
  m_directory(basedir.empty() ? (filename ? FileSystem::dirname(*filename) : "/") : basedir),
  m_extensions(extensions),
  m_basedir(basedir),
  m_directories(),
  m_files(),
  m_path_relative_to_basedir(path_relative_to_basedir),
  m_callback(std::move(callback))
{
  if (!PHYSFS_exists(m_directory.c_str())) {
    m_directory = "/"; //The filename is probably included in an old add-on.
  }

  refresh_items();
}

FileSystemMenu::~FileSystemMenu()
{
}

void
FileSystemMenu::refresh_items()
{
  m_items.clear();
  m_directories.clear();
  m_files.clear();
  m_directory = FileSystem::normalize(m_directory);

  add_label(m_directory);
  add_hl();

  int item_id = 0;

  // Do not allow leaving the data directory
  if (m_directory != "/") {
    m_directories.push_back("..");
  }

  char** dir_files = PHYSFS_enumerateFiles(m_directory.c_str());
  if (dir_files)
  {
    for (char const* const* file = dir_files; *file != nullptr; ++file)
    {
      std::string filepath = FileSystem::join(m_directory, *file);
      if (physfsutil::is_directory(filepath))
      {
        m_directories.push_back(*file);
      }
      else
      {
        if (has_right_suffix(*file))
        {
          m_files.push_back(*file);
        }
      }
    }
    PHYSFS_freeList(dir_files);
  }

  for (auto const& item : m_directories)
  {
    add_entry(item_id, "[" + std::string(item) + "]");
    item_id++;
  }

  for (auto const& item : m_files)
  {
    add_entry(item_id, item);
    item_id++;
  }

  add_hl();
  add_entry(-2, _("Open Directory"));
  add_hl();
  add_back(_("Cancel"));

  m_active_item = 2;

  // Re-center menu
  on_window_resize();
}

bool
FileSystemMenu::has_right_suffix(std::string const& file) const
{
  if (m_extensions.empty())
    return true;

  for (auto const& extension : m_extensions) {
    if (file.ends_with(extension))
    {
      return true;
    }
  }
  return false;
}

void
FileSystemMenu::menu_action(MenuItem& item)
{
  if (item.get_id() >= 0) {
    size_t id = item.get_id();
    if (id < m_directories.size()) {
      m_directory = FileSystem::join(m_directory, m_directories[id]);
      refresh_items();
    } else {
      id -= m_directories.size();
      if (id < m_files.size()) {
        std::string new_filename = FileSystem::join(m_directory, m_files[id]);

        if (!m_basedir.empty() && m_path_relative_to_basedir) {
          new_filename = FileSystem::relpath(new_filename, m_basedir);
        }

        if (m_filename)
          *m_filename = new_filename;

        if (m_callback)
          m_callback(new_filename);

        MenuManager::instance().pop_menu();
      } else {
        log_warning("Selected invalid file or directory");
      }
    }
  }
  else if (item.get_id() == -2)
  {
    FileSystem::open_path(FileSystem::join(PHYSFS_getRealDir(m_directory.c_str()), m_directory));
  }
}

/* EOF */
