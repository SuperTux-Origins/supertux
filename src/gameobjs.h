// SPDX-FileCopyrightText: 2000 Bill Kendrick <bill@newbreedsoftware.com>
// SPDX-FileCopyrightText: 2004 Tobias Glaesser <tobi.web@gmx.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SUPERTUX_GAMEOBJS_H
#define SUPERTUX_GAMEOBJS_H

#include "type.h"
#include "texture.h"
#include "timer.h"
#include "scene.h"

/* Bounciness of distros: */
#define NO_BOUNCE 0
#define BOUNCE 1

class BouncyDistro : public GameObject
{
 public:
  
  void init(float x, float y);
  void action(double frame_ratio);
  void draw(); 
  std::string type() { return "BouncyDistro"; };
};

extern Surface* img_distro[4];

#define BOUNCY_BRICK_MAX_OFFSET 8
#define BOUNCY_BRICK_SPEED 0.9

class Tile;

class BrokenBrick : public GameObject
{
 public:
  Timer timer;
  Tile* tile;

  void init(Tile* tile, float x, float y, float xm, float ym);
  void action(double frame_ratio);
  void draw();
  std::string type() { return "BrokenBrick"; };
};

class BouncyBrick : public GameObject
{
 public:
  float offset;
  float offset_m;
  int shape;

  void init(float x, float y);
  void action(double frame_ratio);
  void draw();
  std::string type() { return "BouncyBrick"; };
};

class FloatingScore : public GameObject
{
 public:
  int value;
  Timer timer;
  
  void init(float x, float y, int s);
  void action(double frame_ratio);
  void draw();
  std::string type() { return "FloatingScore"; };
};

#endif 

/* Local Variables: */
/* mode:c++ */
/* End: */
