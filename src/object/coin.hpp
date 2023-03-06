//  SuperTux
//  Copyright (C) 2006 Matthias Braun <matze@braunis.de>
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

#ifndef HEADER_SUPERTUX_OBJECT_COIN_HPP
#define HEADER_SUPERTUX_OBJECT_COIN_HPP

#include "object/path_object.hpp"
#include "object/moving_sprite.hpp"
#include "supertux/physic.hpp"

class Path;
class PathWalker;
class TileMap;

class Coin : public MovingSprite,
             public PathObject
{

friend class HeavyCoin;

public:
  Coin(Vector const& pos);
  Coin(ReaderMapping const& reader);
  virtual void finish_construction() override;

  virtual HitResponse collision(GameObject& other, CollisionHit const& hit) override;

  virtual void update(float dt_sec) override;
  static std::string class_name() { return "coin"; }
  virtual std::string get_class_name() const override { return class_name(); }
  static std::string display_name() { return _("Coin"); }
  virtual std::string get_display_name() const override { return display_name(); }

  virtual void move_to(Vector const& pos) override;

  virtual void on_flip(float height) override;

  void collect();

private:
  Vector m_offset;
  bool m_from_tilemap;
  bool m_add_path;
  Physic m_physic;
  std::string m_collect_script;

  int m_starting_node;

private:
  Coin(Coin const&) = delete;
  Coin& operator=(Coin const&) = delete;
};

class HeavyCoin final : public Coin
{
public:
  HeavyCoin(Vector const& pos, Vector const& init_velocity);
  HeavyCoin(ReaderMapping const& reader);

  virtual void update(float dt_sec) override;
  virtual void collision_solid(CollisionHit const& hit) override;

  static std::string class_name() { return "heavycoin"; }
  virtual std::string get_class_name() const override { return class_name(); }
  static std::string display_name() { return _("Heavy Coin"); }
  virtual std::string get_display_name() const override { return display_name(); }

  virtual void on_flip(float height) override;

private:
  Physic m_physic;
  CollisionHit m_last_hit;

private:
  HeavyCoin(HeavyCoin const&) = delete;
  HeavyCoin& operator=(HeavyCoin const&) = delete;
};

#endif

/* EOF */
