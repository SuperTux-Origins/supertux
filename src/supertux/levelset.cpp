//  SuperTux
//  Copyright (C) 2014 Ingo Ruhnke <grumbel@gmail.com>
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

#include "supertux/levelset.hpp"

#include <algorithm>

#include <physfs.h>
#include <strut/numeric_less.hpp>

#include "physfs/util.hpp"
#include "util/file_system.hpp"
#include "util/log.hpp"

Levelset::Levelset(std::string const& basedir, bool recursively) :
  m_basedir(basedir),
  m_levels()
{
  walk_directory(m_basedir, recursively);
  std::sort(m_levels.begin(), m_levels.end(), strut::numeric_less);
}

int
Levelset::get_num_levels() const
{
  return static_cast<int>(m_levels.size());
}

std::string
Levelset::get_level_filename(int i) const
{
  return m_levels[i];
}

void
Levelset::walk_directory(std::string const& directory, bool recursively)
{
  bool is_basedir = (directory == m_basedir);
  char** files = PHYSFS_enumerateFiles(directory.c_str());
  if (!files)
  {
    log_warning("Couldn't read subset dir '{}'", directory);
    return;
  }

  for (char const* const* filename = files; *filename != nullptr; ++filename)
  {
    auto filepath = FileSystem::join(directory.c_str(), *filename);
    if (physfsutil::is_directory(filepath) && recursively)
    {
      walk_directory(filepath, true);
    }
    if (std::string_view(*filename).ends_with(".stlv"))
    {
      if (is_basedir)
      {
        m_levels.push_back(*filename);
      }
      else
      {
        // Replace basedir part of file path plus slash.
        filepath = filepath.replace(0, m_basedir.length() + 1, "");
        m_levels.push_back(filepath);
      }
    }
  }
  PHYSFS_freeList(files);
}

/* EOF */
