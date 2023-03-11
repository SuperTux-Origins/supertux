//  SuperTux
//  Copyright (C) 2018 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_SUPERTUX_OBJECT_PATH_GAMEOBJECT_HPP
#define HEADER_SUPERTUX_OBJECT_PATH_GAMEOBJECT_HPP

#include "sprite/sprite_ptr.hpp"
#include "supertux/game_object.hpp"
#include "math/fwd.hpp"

class Path;

enum class PathStyle
{
  NONE,
  SOLID
};

class PathGameObject : public GameObject
{
public:
  PathGameObject();
  PathGameObject(Vector const& pos);
  PathGameObject(ReaderMapping const& mapping, bool backward_compatibility_hack=false);
  ~PathGameObject() override;

  void update(float dt_sec) override;
  void draw(DrawingContext& context) override;


  const std::string get_icon_path() const override {
    return "images/engine/editor/path.png";
  }

  void remove_me() override;

  void on_flip(float height) override;

  Path& get_path() { return *m_path; }

  void copy_into(PathGameObject& other);

private:
  /** Removes the object if the path is not referenced anywhere */
  void check_references();

private:
  std::unique_ptr<Path> m_path;
  PathStyle m_style;
  SpritePtr m_edge_sprite;
  SpritePtr m_node_sprite;

private:
  PathGameObject(PathGameObject const&) = delete;
  PathGameObject& operator=(PathGameObject const&) = delete;
};

#endif

/* EOF */
