// SPDX-FileCopyrightText: 2004 Ingo Ruhnke <grumbel@gmx.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef HEADER_SPRITE_HXX
#define HEADER_SPRITE_HXX

#include <string>
#include <vector>
#include "lispreader.h"
#include "texture.h"

class Sprite
{
 private:
  std::string name;

  int x_hotspot;
  int y_hotspot;

  /** Frames per second */
  float fps;

  /** Number of seconds that a frame is displayed until it is switched
      to the next frame */
  float frame_delay;

  float time;

  std::vector<Surface*> surfaces;

  void init_defaults();
 public:
  /** cur has to be a pointer to data in the form of ((x-hotspot 5)
      (y-hotspot 10) ...) */
  Sprite(lisp_object_t* cur);
  ~Sprite();
  
  void reset();

  /** Update the sprite and process to the next frame */
  void update(float delta);
  void draw(float x, float y);
  void draw_part(float sx, float sy, float x, float y, float w, float h);
  int get_current_frame() const;

  std::string get_name() const { return name; } 
  int get_width() const;
  int get_height() const;
};

#endif

/* Local Variables: */
/* mode:c++ */
/* End: */
