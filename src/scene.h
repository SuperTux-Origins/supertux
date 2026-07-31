// SPDX-FileCopyrightText: 2003 Tobias Glaesser <tobi.web@gmx.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SUPERTUX_SCENE_H
#define SUPERTUX_SCENE_H

#include "texture.h"
#include "timer.h"

#define FRAME_RATE 10 // 100 Frames per second (10ms)

// Player stats
struct PlayerStatus
{
  int  score;
  int  distros;
  int  lives;
  enum BonusType { NO_BONUS, GROWUP_BONUS, FLOWER_BONUS };
  BonusType bonus;

  int  score_multiplier;

  PlayerStatus();

  void reset();
};

std::string bonus_to_string(PlayerStatus::BonusType b);
PlayerStatus::BonusType string_to_bonus(const std::string& str);

extern PlayerStatus player_status;

extern float scroll_x;
extern unsigned int global_frame_counter;

#endif /*SUPERTUX_SCENE_H*/
