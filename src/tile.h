// SPDX-FileCopyrightText: 2004 Tobias Glaesser <tobi.web@gmx.de>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TILE_H
#define TILE_H

#include <set>
#include <map>
#include <vector>
#include "texture.h"
#include "globals.h"
#include "lispreader.h"
#include "setup.h"

/**
Tile Class
*/
class Tile
{
public:
  Tile();
  ~Tile();

  int id;

  std::vector<Surface*> images;
  std::vector<Surface*> editor_images;
  
  std::vector<std::string>  filenames;
  std::vector<std::string> editor_filenames;
  
  /** solid tile that is indestructable by Tux */
  bool solid;

  /** a brick that can be destroyed by jumping under it */
  bool brick;

  /** FIXME: ? */
  bool ice;

  /** water */
  bool water;

  /** Bonusbox, content is stored in \a data */
  bool fullbox;

  /** Tile is a distro/coin */
  bool distro;

  /** the level should be finished when touching a goaltile.
   * if data is 0 then the endsequence should be triggered, if data is 1
   * then we can finish the level instantly.
   */
  bool goal;

  /** General purpose data attached to a tile (content of a box, type of coin) */
  int data;

  /** Id of the tile that is going to replace this tile once it has
      been collected or jumped at */
  int next_tile;

  int anim_speed;
  
  /** Draw a tile on the screen: */
  static void draw(float x, float y, unsigned int c, Uint8 alpha = 255);
  static void draw_stretched(float x, float y, int w, int h, unsigned int c, Uint8 alpha = 255);
};

struct TileGroup
{
  friend bool operator<(const TileGroup& lhs, const TileGroup& rhs)
  { return lhs.name < rhs.name; };
  friend bool operator>(const TileGroup& lhs, const TileGroup& rhs)
  { return lhs.name > rhs.name; };

  std::string name;
  std::vector<int> tiles;
};

class TileManager
{
 private:
  TileManager();
  ~TileManager();
  
  std::vector<Tile*> tiles;
  static TileManager* instance_ ;
  static std::set<TileGroup>* tilegroups_;
  /** @param replace If true, free existing tiles first (top-level load).
      Nested (tileset (file …)) forms call with replace=false so parent
      tiles are not wiped mid-parse. */
  void load_tileset(std::string filename, bool replace = true);

  std::string current_tileset;
  
 public:
  static TileManager* instance() { return instance_ ? instance_ : instance_ = new TileManager(); }
  static void destroy_instance() { delete instance_; instance_ = 0; }
  
  static std::set<TileGroup>* tilegroups() { if(!instance_) { instance_ = new TileManager(); } return tilegroups_ ? tilegroups_ : tilegroups_ = new std::set<TileGroup>; }
  /** Returns the tile for id, or 0 if missing. Callers must null-check. */
  Tile* get(unsigned int id) {
    if (id < tiles.size())
      return tiles[id];
    return 0;
  }
};

#endif
