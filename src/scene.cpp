// SPDX-FileCopyrightText: 2003 Tobias Glaesser <tobi.web@gmx.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <stdlib.h>
#include "scene.h"
#include "defines.h"

PlayerStatus player_status;

PlayerStatus::PlayerStatus()
  : score(0),
    distros(0),
    lives(START_LIVES),
    bonus(NO_BONUS),
    score_multiplier(1)
{
}

void PlayerStatus::reset()
{
  score = 0;
  distros = 0;
  lives = START_LIVES;
  bonus = NO_BONUS;
  score_multiplier = 1;
}

std::string bonus_to_string(PlayerStatus::BonusType b)
{
  switch (b)
    {
    case PlayerStatus::NO_BONUS:
      return "none";
    case PlayerStatus::GROWUP_BONUS:
      return "growup";
    case PlayerStatus::FLOWER_BONUS:
      return "iceflower";
    default:
      return "none";
    }
}

PlayerStatus::BonusType string_to_bonus(const std::string& str)
{
  if (str == "none")
    return PlayerStatus::NO_BONUS;
  else if (str == "growup")
    return PlayerStatus::GROWUP_BONUS;
  else if (str == "iceflower")
    return PlayerStatus::FLOWER_BONUS;
  else
    return PlayerStatus::NO_BONUS;
}

// FIXME: Move this into a view class
float scroll_x;

unsigned int global_frame_counter;

// EOF //

