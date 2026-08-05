// SPDX-FileCopyrightText: 2004 Tobias Glaesser <tobi.web@gmx.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "tile.h"
#include "scene.h"
#include "assert.h"

TileManager* TileManager::instance_  = 0;
std::set<TileGroup>* TileManager::tilegroups_  = 0;

Tile::Tile()
{
}

Tile::~Tile()
{
  for(std::vector<Surface*>::iterator i = images.begin(); i != images.end();
      ++i) {
    delete *i;
  }
  for(std::vector<Surface*>::iterator i = editor_images.begin();
      i != editor_images.end(); ++i) {
    delete *i;                                                                
  }
}

//---------------------------------------------------------------------------

TileManager::TileManager()
{
  std::string filename = datadir + "/images/tilesets/supertux.stgt";
  load_tileset(filename);
}

TileManager::~TileManager()
{
  for(std::vector<Tile*>::iterator i = tiles.begin(); i != tiles.end(); ++i) {
    delete *i;                                                                  
  }
}

void TileManager::load_tileset(std::string filename, bool replace)
{
  if(replace && filename == current_tileset)
    return;

  if(replace)
    {
      for(std::vector<Tile*>::iterator i = tiles.begin(); i != tiles.end(); ++i) {
        delete *i;
      }
      tiles.clear();
    }

  lisp_object_t* root_obj = lisp_read_from_file(filename);

  if (!root_obj)
    st_abort("Couldn't load file", filename);

  if (!lisp_expect_symbol_root(root_obj, "supertux-tiles"))
    {
      lisp_free(root_obj);
      st_abort("Not a supertux-tiles file", filename);
    }

  {
      lisp_object_t* cur = lisp_cdr(root_obj);
      int tileset_id = 0;

      while(!lisp_nil_p(cur))
        {
          lisp_object_t* element = lisp_car(cur);
          const char* el_sym = 0;

          if (!lisp_element_symbol(element, &el_sym))
            {
              cur = lisp_cdr(cur);
              continue;
            }

          if (strcmp(el_sym, "tile") == 0)
            {
              Tile* tile = new Tile;
              tile->id      = -1;
              tile->solid   = false;
              tile->brick   = false;
              tile->ice     = false;
              tile->water   = false;
              tile->fullbox = false;
              tile->distro  = false;
              tile->goal    = false;
              tile->data    = 0;
              tile->next_tile  = 0;
              tile->anim_speed = 25;

              LispReader reader(lisp_cdr(element));
              if (!reader.read_int("id", &tile->id) || tile->id < 0)
                {
                  delete tile;
                  fprintf(stderr,
                          "\nError: tileset entry is missing a valid (id N) field.\n"
                          "  File: %s\n"
                          "  Each (tile ...) form must include a non-negative integer id.\n"
                          "  Example: (tile (id 10) (images \"block.png\") (solid #t))\n\n",
                          filename.c_str());
                  lisp_free(root_obj);
                  st_abort("Invalid tile definition (missing or bad id)", filename);
                }
              reader.read_bool("solid",     &tile->solid);
              reader.read_bool("brick",     &tile->brick);
              reader.read_bool("ice",       &tile->ice);
              reader.read_bool("water",     &tile->water);
              reader.read_bool("fullbox",   &tile->fullbox);
              reader.read_bool("distro",    &tile->distro);
              reader.read_bool("goal",      &tile->goal);
              reader.read_int("data",       &tile->data);
              reader.read_int("anim-speed", &tile->anim_speed);
              if (tile->anim_speed <= 0)
                tile->anim_speed = 25;
              reader.read_int("next-tile",  &tile->next_tile);
              reader.read_string_vector("images",  &tile->filenames);
              reader.read_string_vector("editor-images", &tile->editor_filenames);

              for(std::vector<std::string>::iterator it = tile->
                  filenames.begin();
                  it != tile->filenames.end();
                  ++it)
                {
                  tile->images.push_back(new Surface(
                               datadir +  "/images/tilesets/" + (*it),
                               USE_ALPHA));
                }
              for(std::vector<std::string>::iterator it = tile->editor_filenames.begin();
                  it != tile->editor_filenames.end();
                  ++it)
                {
                  tile->editor_images.push_back(new Surface(
                               datadir + "/images/tilesets/" + (*it),
                               USE_ALPHA));
                }

              if (tile->id + tileset_id >= int(tiles.size()))
                tiles.resize(tile->id + tileset_id + 1, 0);

              /* Nested loads may overwrite the same id; free the previous tile. */
              if (tiles[tile->id + tileset_id] != 0
                  && tiles[tile->id + tileset_id] != tile)
                delete tiles[tile->id + tileset_id];

              tiles[tile->id + tileset_id] = tile;
            }
          else if (strcmp(el_sym, "tileset") == 0)
            {
              LispReader reader(lisp_cdr(element));
              std::string child_file;
              reader.read_string("file",  &child_file);
              child_file = datadir + "/images/tilesets/" + child_file;
              /* Merge nested tileset without clearing tiles already parsed. */
              load_tileset(child_file, false);
            }
          else if (strcmp(el_sym, "tilegroup") == 0)
            {
              TileGroup new_;
              LispReader reader(lisp_cdr(element));
              reader.read_string("name",  &new_.name);
              reader.read_int_vector("tiles", &new_.tiles);
              if(!tilegroups_)
                tilegroups_ = new std::set<TileGroup>;
              tilegroups_->insert(new_);
            }
          else if (strcmp(el_sym, "properties") == 0)
            {
              LispReader reader(lisp_cdr(element));
              reader.read_int("id",  &tileset_id);
              tileset_id *= 1000;
            }
          else
            {
              puts("Unhandled symbol");
            }

          cur = lisp_cdr(cur);
        }
  }

  lisp_free(root_obj);
  if(replace)
    current_tileset = filename;
}

void
Tile::draw(float x, float y, unsigned int c, Uint8 alpha)
{
  if (c != 0)
    {
      Tile* ptile = TileManager::instance()->get(c);
      if(ptile)
        {
          int aspeed = ptile->anim_speed > 0 ? ptile->anim_speed : 25;
          if(ptile->images.size() > 1)
            {
              ptile->images[( ((global_frame_counter*25) / aspeed) % (ptile->images.size()))]->draw(x,y, alpha);
            }
          else if (ptile->images.size() == 1)
            {
              ptile->images[0]->draw(x,y, alpha);
            }
        }
    }
}

void
Tile::draw_stretched(float x, float y, int w, int h, unsigned int c, Uint8 alpha)
{
  if (c != 0)
    {
      Tile* ptile = TileManager::instance()->get(c);
      if(ptile)
        {
          int aspeed = ptile->anim_speed > 0 ? ptile->anim_speed : 25;
          if(ptile->images.size() > 1)
            {
              ptile->images[( ((global_frame_counter*25) / aspeed) % (ptile->images.size()))]->draw_stretched(x,y,w,h, alpha);
            }
          else if (ptile->images.size() == 1)
            {
              ptile->images[0]->draw_stretched(x,y, w, h, alpha);
            }
        }
    }
}

// EOF //

