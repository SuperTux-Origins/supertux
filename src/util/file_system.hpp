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

#ifndef HEADER_SUPERTUX_UTIL_FILE_SYSTEM_HPP
#define HEADER_SUPERTUX_UTIL_FILE_SYSTEM_HPP

#include <string>

namespace FileSystem {

/** Returns true if the given path is a directory */
bool is_directory(std::string const& path);

/** Return true if the given file exists */
bool exists(std::string const& path);

/** Create the given directory */
void mkdir(std::string const& directory);

/** Copy file from one directory to another */
void copy(std::string const& source_path, std::string const& target_path);

/** returns the path of the directory the file is in */
std::string dirname(std::string const& filename);

/** returns the name of the file */
std::string basename(std::string const& filename);

/** Return a path to 'filename' that is relative to 'basedir', e.g.
    reldir("/levels/juser/level1.stl", "/levels") -> "juser/level1.stl" */
std::string relpath(std::string const& filename, std::string const& basedir);

/** Return the extension of a file */
std::string extension(std::string const& filename);

/** remove everything starting from and including the last dot */
std::string strip_extension(std::string const& filename);

/** normalize filename so that "blup/bla/blo/../../bar" will become
    "blup/bar" */
std::string normalize(std::string const& filename);

/** join two filenames join("foo", "bar") -> "foo/bar" */
std::string join(std::string const& lhs, std::string const& rhs);

/** Remove a file
    @return true when successfully removed, false otherwise */
 bool remove(std::string const& path);

/** Opens a file path with the user's preferred app for that file.
 * @param path path to open
 */
 void open_path(std::string const& path);

/** Opens an URL in the user's preferred browser.
 * @param url URL to open
 */
 void open_url(std::string const& url);

} // namespace FileSystem

#endif

/* EOF */
