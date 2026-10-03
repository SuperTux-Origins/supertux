//  SuperTux
//  Copyright (C) 2026 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_SUPERTUX_OBJECT_PLAYER_CONSTANTS_HPP
#define HEADER_SUPERTUX_OBJECT_PLAYER_CONSTANTS_HPP

#include <string>

/** Tux can swim in water tiles */
#define SWIMMING

/** Tuning of Tux' movement, shared by Player and the player systems */
namespace player_constants {

/* Times: */
inline constexpr float TUX_SAFE_TIME = 1.8f;
inline constexpr float TUX_INVINCIBLE_TIME = 14.0f;
inline constexpr float TUX_BACKFLIP_TIME = 2.1f; // minimum air time that backflip results in a loss of control

inline constexpr int IDLE_TIME[] = { 5000, 0, 2500, 0, 2500 };
inline constexpr int TIME_UNTIL_IDLE = 5000;
inline constexpr unsigned int IDLE_STAGE_COUNT = 5;

/** idle stages */
inline const std::string IDLE_STAGES[] =
{ "stand",
  "idle",
  "stand",
  "idle",
  "stand" };

/** acceleration in horizontal direction when walking
 * (all accelerations are in  pixel/s^2) */
inline constexpr float WALK_ACCELERATION_X = 300;
/** acceleration in horizontal direction when running */
inline constexpr float RUN_ACCELERATION_X = 400;
/** acceleration when skidding */
inline constexpr float SKID_XM = 200;
/** time of skidding in seconds */
inline constexpr float SKID_TIME = .3f;
/** maximum walk velocity (pixel/s) */
inline constexpr float MAX_WALK_XM = 230;
/** maximum run velocity (pixel/s) */
inline constexpr float MAX_RUN_XM = 320;
/** bonus run velocity addition (pixel/s) */
inline constexpr float BONUS_RUN_XM = 80;
/** maximum horizontal climb velocity */
inline constexpr float MAX_CLIMB_XM = 96;
/** maximum vertical climb velocity */
inline constexpr float MAX_CLIMB_YM = 128;
/** maximum vertical glide velocity */
inline constexpr float MAX_GLIDE_YM = 128;
/** sliding down walls velocity */
inline constexpr float MAX_WALLCLING_YM = 64;
/** instant velocity when tux starts to walk */
inline constexpr float WALK_SPEED = 100;
/** rate at which the boost decreases */
inline constexpr float BOOST_DECREASE_RATE = 500;
/** rate at which the speed decreases if going above maximum */
inline constexpr float OVERSPEED_DECELERATION = 100;

/** multiplied by WALK_ACCELERATION to give friction */
inline constexpr float NORMAL_FRICTION_MULTIPLIER = 1.5f;
/** multiplied by WALK_ACCELERATION to give friction */
inline constexpr float ICE_FRICTION_MULTIPLIER = 0.1f;
inline constexpr float ICE_ACCELERATION_MULTIPLIER = 0.25f;

/** time of the kick (kicking mriceblock) animation */
inline constexpr float KICK_TIME = .3f;

/** if Tux cannot unduck for this long, he will get hurt */
inline constexpr float UNDUCK_HURT_TIME = 0.25f;
/** gravity is higher after the jump key is released before
    the apex of the jump is reached */
inline constexpr float JUMP_EARLY_APEX_FACTOR = 3.0;

inline constexpr float JUMP_GRACE_TIME = 0.25f; /**< time before hitting the ground that the jump button may be pressed (and still trigger a jump) */
inline constexpr float COYOTE_TIME = 0.1f; /**< time between the moment leaving a platform without jumping and being able to jump anyways despite being in the air */

/* Tux's collision rectangle */
inline constexpr float TUX_WIDTH = 31.8f;
inline constexpr float RUNNING_TUX_WIDTH = 34;
inline constexpr float SMALL_TUX_HEIGHT = 30.8f;
inline constexpr float BIG_TUX_HEIGHT = 62.8f;
inline constexpr float DUCKED_TUX_HEIGHT = 31.8f;

/* Stone Tux variables */
inline constexpr float MAX_STONE_SPEED = 500.f;
inline constexpr float STONE_KEY_ACCELERATION = 200.f;
inline constexpr float STONE_DOWN_ACCELERATION = 300.f;
inline constexpr float STONE_UP_ACCELERATION = 400.f;

/* Swim variables */
inline constexpr float SWIM_SPEED = 300.f;
inline constexpr float SWIM_BOOST_SPEED = 600.f;
inline constexpr float SWIM_TO_BOOST_ACCEL = 15.f;
inline constexpr float TURN_MAGNITUDE = 0.15f;
inline constexpr float TURN_MAGNITUDE_BOOST = 0.2f;

/* Buttjump variables */

inline constexpr float BUTTJUMP_WAIT_TIME = 0.2f; // the length of time that the buttjump action is being played
inline constexpr float BUTTJUMP_SPEED = 800.f;


} // namespace player_constants

#endif

/* EOF */
