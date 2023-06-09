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

#ifndef HEADER_SUPERTUX_PHYSFS_PHYSFS_SDL_HPP
#define HEADER_SUPERTUX_PHYSFS_PHYSFS_SDL_HPP

#include <SDL.h>
#include <string>

/** The returned SDL_RWops object must be freed with SDL_RWclose(),
    SDL library functions have a flag to perform that call
    automatically. Do not use 'delete' or 'free()' on it.

    See: https://wiki.libsdl.org/SDL_RWclose */
SDL_RWops* get_physfs_SDLRWops(std::string const& filename);
SDL_RWops* get_writable_physfs_SDLRWops(std::string const& filename);

#endif

/* EOF */
