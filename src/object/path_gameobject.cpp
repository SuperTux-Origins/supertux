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

#include "object/path_gameobject.hpp"


#include "gui/menu_manager.hpp"
#include "object/path_object.hpp"
#include "sprite/sprite.hpp"
#include "sprite/sprite_manager.hpp"
#include "supertux/debug.hpp"
#include "supertux/sector.hpp"
#include "util/log.hpp"
#include "util/reader_mapping.hpp"
#include "util/unique_name.hpp"

namespace {

PathStyle PathStyle_from_string(std::string const& text)
{
  if (text == "none") {
    return PathStyle::NONE;
  } else if (text == "solid") {
    return PathStyle::SOLID;
  } else {
    log_warning("unknown PathStyle: {}", text);
    return PathStyle::NONE;
  }
}

} // namespace

PathGameObject::PathGameObject() :
  m_path(new Path),
  m_style(PathStyle::NONE),
  m_edge_sprite(),
  m_node_sprite()
{
  m_name = make_unique_name("path", this);
}

PathGameObject::PathGameObject(Vector const& pos) :
  m_path(new Path(pos)),
  m_style(PathStyle::NONE),
  m_edge_sprite(),
  m_node_sprite()
{
  m_name = make_unique_name("path", this);
}

PathGameObject::PathGameObject(ReaderMapping const& mapping, bool backward_compatibility_hack) :
  GameObject(mapping),
  m_path(new Path),
  m_style(PathStyle::NONE),
  m_edge_sprite(),
  m_node_sprite()
{
  if (backward_compatibility_hack)
  {
    m_path->read(mapping);
  }
  else
  {
    ReaderMapping path_mapping;
    if (mapping.read("path", path_mapping))
    {
      m_path->read(path_mapping);
    }
  }

  mapping.read("style", m_style, PathStyle_from_string);

  if (m_style == PathStyle::SOLID)
  {
    m_edge_sprite = SpriteManager::current()->create("images/objects/path/edge.sprite");
    m_node_sprite = SpriteManager::current()->create("images/objects/path/node.sprite");
  }

  if (m_name.empty()) {
    set_name(make_unique_name("path", this));
  }
}

PathGameObject::~PathGameObject()
{
}

void
PathGameObject::update(float dt_sec)
{
  check_references();
}

void
PathGameObject::draw(DrawingContext& context)
{
  if (m_style == PathStyle::SOLID)
  {
    std::optional<Vector> previous_node;

    for (auto const& node : m_path->get_nodes())
    {
      if (previous_node)
      {
        const Vector p1 = *previous_node;
        const Vector p2 = node.position;
        const Vector diff = (p2 - p1);
        const float length = glm::length(diff);
        const Vector unit = glm::normalize(diff);
        float dot_distance = 16.0f;

        // Recalculate the dot distance to evenly spread across the
        // whole edge
        dot_distance = length / floorf(length / dot_distance);

        for (float i = dot_distance; i < length; i += dot_distance) // NOLINT
        {
          Vector dot_pos = p1 + unit * i;
          m_edge_sprite->draw(context.color(), Vector(dot_pos), LAYER_OBJECTS - 1);
        }
      }

      m_node_sprite->draw(context.color(), node.position, LAYER_OBJECTS - 1);

      previous_node = node.position;
    }
  }

  if (g_debug.show_collision_rects)
  {
    const Color node_color = Color::BLUE;
    const Color edge_color = Color::MAGENTA;

    std::optional<Vector> previous_node;
    for (auto const& node : m_path->get_nodes())
    {
      if (previous_node)
      {
        context.color().draw_line(*previous_node, node.position, edge_color, LAYER_OBJECTS - 2);
      }

      context.color().draw_filled_rect(Rectf::from_center(node.position, Sizef(16.0f, 16.0f)),
                                       node_color, LAYER_OBJECTS - 1);

      previous_node = node.position;
    }
  }
}

void
PathGameObject::remove_me()
{
  auto const& path_objects = Sector::get().get_objects_by_type<PathObject>();

  for (auto const& path_obj : path_objects)
  {
    if (path_obj.get_path_gameobject() == this)
    {
      log_warning("Attempt to delete path {} while bound to object", get_name());
      return;
    }
  }

  GameObject::remove_me();
}

void
PathGameObject::copy_into(PathGameObject& other)
{
  other.get_path().m_nodes = get_path().m_nodes;
}

void
PathGameObject::check_references()
{
  if (!Sector::current())
    return;

  // Object settings menu might hold references to paths
  if (MenuManager::instance().is_active())
    return;

  auto const& path_objects = Sector::get().get_objects_by_type<PathObject>();

  for (auto const& path_obj : path_objects)
    if (path_obj.get_path_gameobject() == this)
      return;

  remove_me();
}

/* EOF */
