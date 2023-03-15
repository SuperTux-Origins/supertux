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

#include "supertux/menu/cheat_menu.hpp"

#include "gui/menu_item.hpp"
#include "object/player.hpp"
#include "supertux/game_session.hpp"
#include "supertux/sector.hpp"

CheatMenu::CheatMenu()
{
  auto const& players = Sector::get().get_players();

  add_label(_("Cheats"));
  add_hl();
  add_entry(MNID_GROW, _("Bonus: Grow"));
  add_entry(MNID_FIRE, _("Bonus: Fire"));
  add_entry(MNID_ICE, _("Bonus: Ice"));
  add_entry(MNID_AIR, _("Bonus: Air"));
  add_entry(MNID_EARTH, _("Bonus: Earth"));
  add_entry(MNID_STAR, _("Bonus: Star"));
  add_entry(MNID_SHRINK, _("Shrink Tux"));
  add_entry(MNID_KILL, _("Kill Tux"));
  add_entry(MNID_FINISH, _("Finish Level"));

  if (players.size() == 1)
  {
    add_entry(MNID_GHOST, players[0]->get_ghost_mode() ?
              _("Leave Ghost Mode") : _("Activate Ghost Mode"));
  }
  else
  {
    // In multiplayer, different players may have different ghost states
    add_entry(MNID_GHOST, _("Activate Ghost Mode"));
    add_entry(MNID_UNGHOST, _("Leave Ghost Mode"));
  }
  add_hl();
  add_back(_("Back"));
}

void
CheatMenu::menu_action(MenuItem& item)
{
  if (!Sector::current()) return;

  auto const& players = Sector::get().get_players();
  Player* single_player = (players.size() == 1) ? players[0] : nullptr;

  for (Player* const player_ptr : Sector::get().get_players()) {
    Player& player = *player_ptr;

    switch (item.get_id())
    {
      case MNID_GROW:
        player.set_bonus(GROWUP_BONUS);
        break;

      case MNID_FIRE:
        player.set_bonus(FIRE_BONUS);
        player.get_status().max_fire_bullets[player.get_id()] = 5;
        break;

      case MNID_ICE:
        player.set_bonus(ICE_BONUS);
        player.get_status().max_ice_bullets[player.get_id()] = 5;
        break;

      case MNID_AIR:
        player.set_bonus(AIR_BONUS);
        player.get_status().max_air_time[player.get_id()] = 5;
        break;

      case MNID_EARTH:
        player.set_bonus(EARTH_BONUS);
        player.get_status().max_earth_time[player.get_id()] = 5;
        break;

      case MNID_STAR:
        player.make_invincible();
        break;

      case MNID_SHRINK:
        player.kill(false);
        break;

      case MNID_KILL:
        player.kill(true);
        break;

      case MNID_GHOST:
      case MNID_UNGHOST:
        player.set_ghost_mode(!single_player->get_ghost_mode());
        break;

      case MNID_FINISH:
        if (GameSession::current()) {
          GameSession::current()->finish(true);
        }
        break;

      default:
        break;
    }
  }
}

/* EOF */
