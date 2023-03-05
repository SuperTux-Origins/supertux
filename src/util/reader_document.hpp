//  SuperTux
//  Copyright (C) 2015 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_SUPERTUX_UTIL_READER_DOCUMENT_HPP
#define HEADER_SUPERTUX_UTIL_READER_DOCUMENT_HPP

#include <prio/reader_document.hpp>

#include "physfs/ifile_stream.hpp"

using prio::ReaderDocument;

inline
prio::ReaderDocument load_reader_document(std::string const& filename)
{
  IFileStream stream(filename);
  return prio::ReaderDocument::from_stream(prio::Format::SEXPR, stream, prio::ErrorHandler::THROW, filename);
}

#endif

/* EOF */
