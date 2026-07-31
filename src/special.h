// SPDX-FileCopyrightText: 2003 Tobias Glaesser <tobi.web@gmx.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SUPERTUX_SPECIAL_H
#define SUPERTUX_SPECIAL_H

#include "platform_config.h"
#include "type.h"
#include "texture.h"
#include "collision.h"
#include "player.h"
#include "physic.h"

/* Upgrade types: */

enum UpgradeKind {
  UPGRADE_GROWUP,
  UPGRADE_ICEFLOWER,
  UPGRADE_HERRING,
  UPGRADE_1UP
};

void load_special_gfx();
void free_special_gfx();

class Upgrade : public GameObject
{
public:
  UpgradeKind kind;
  Direction  dir;
  Physic physic;

  void init(float x, float y, Direction dir, UpgradeKind kind);
  void action(double frame_ratio);
  void draw();
  void collision(void* p_c_object, int c_object, CollisionType type);
  std::string type() { return "Upgrade"; };
  
  ~Upgrade() {};

private:
  /** removes the Upgrade from the global upgrade list. Note that after this
   * call the class doesn't exist anymore! So don't use any member variables
   * anymore then
   */
  void remove_me();

  void bump(Player* player);
};

class Bullet : public GameObject
{
 public:
  int life_count;
  base_type base;
  base_type old_base;
  
  void init(float x, float y, float xm, Direction dir);
  void action(double frame_ratio);
  void draw();
  void collision(int c_object);
  std::string type() { return "Bullet"; };

private:
  /** removes the Upgrade from the global upgrade list. Note that after this
   * call the class doesn't exist anymore! So don't use any member variables
   * anymore then
   */
  void remove_me();
};

#endif /*SUPERTUX_SPECIAL_H*/
