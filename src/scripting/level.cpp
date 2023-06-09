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

#include "scripting/level.hpp"

#include "supertux/game_session.hpp"

namespace scripting {

void
Level_finish(bool win)
{
  if (GameSession::current() == nullptr)
    return;

  GameSession::current()->finish(win);
}

void
Level_spawn(std::string const& sector, std::string const& spawnpoint)
{
  if (GameSession::current() == nullptr)
    return;

  GameSession::current()->respawn(sector, spawnpoint);
}

void
Level_toggle_pause()
{
  if (GameSession::current() == nullptr)
    return;
  GameSession::current()->toggle_pause();
}

void
Level_edit(bool edit_mode)
{
  if (GameSession::current() == nullptr) return;
  GameSession::current()->set_editmode(edit_mode);
}

} // namespace scripting

/* EOF */
