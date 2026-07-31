// SPDX-FileCopyrightText: 2004 Ingo Ruhnke <grumbel@gmx.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <iostream>
#include "lispreader.h"
#include "sprite_manager.h"

SpriteManager::SpriteManager(const std::string& filename)
{
  load_resfile(filename);
}

SpriteManager::~SpriteManager()
{
  for(std::map<std::string, Sprite*>::iterator i = sprites.begin();
      i != sprites.end(); ++i) {
    delete i->second;
  }
}

void
SpriteManager::load_resfile(const std::string& filename)
{
  lisp_object_t* root_obj = lisp_read_from_file(filename);
  if (!root_obj)
    {
      std::cout << "SpriteManager: Couldn't load: " << filename << std::endl;
      return;
    }

  lisp_object_t* cur = root_obj;

  if (strcmp(lisp_symbol(lisp_car(cur)), "supertux-resources") != 0)
    return;
  cur = lisp_cdr(cur);

  while(cur)
    {
      lisp_object_t* el = lisp_car(cur);

      if (strcmp(lisp_symbol(lisp_car(el)), "sprite") == 0)
        {
          Sprite* sprite = new Sprite(lisp_cdr(el));

          Sprites::iterator i = sprites.find(sprite->get_name());
          if (i == sprites.end())
            {
              sprites[sprite->get_name()] = sprite;
            }
          else
            {
              delete i->second;
              i->second = sprite;
              std::cout << "Warning: dulpicate entry: '" << sprite->get_name() << "'" << std::endl;
            }
        }
      else
        {
          std::cout << "SpriteManager: Unknown tag" << std::endl;
        }

      cur = lisp_cdr(cur);
    }

  lisp_free(root_obj);
}

Sprite*
SpriteManager::load(const std::string& name)
{
  Sprites::iterator i = sprites.find(name);
  if (i != sprites.end())
    {
      return i->second;
    }
  else
    {
      std::cout << "SpriteManager: Sprite '" << name << "' not found" << std::endl;
      return 0;
    }
}

/* EOF */
