// SPDX-FileCopyrightText: 2004 Ingo Ruhnke <grumbel@gmx.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef HEADER_SPRITE_MANAGER_HXX
#define HEADER_SPRITE_MANAGER_HXX

#include <map>
#include "sprite.h"

class SpriteManager
{
 private:
  typedef std::map<std::string, Sprite*> Sprites;
  Sprites sprites;
 public:
  SpriteManager(const std::string& filename);
  ~SpriteManager();
  
  void    load_resfile(const std::string& filename);
  /** loads a sprite.
   * WARNING: You must not delete the returned object.
   */
  Sprite* load(const std::string& name);
};

#endif

/* Local Variables: */
/* mode:c++ */
/* End: */
