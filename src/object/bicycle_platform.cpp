//  SuperTux - BicyclePlatform
//  Copyright (C) 2007 Christoph Sommer <christoph.sommer@2007.expires.deltadevelopment.de>
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

#include "object/bicycle_platform.hpp"


#include "math/util.hpp"
#include "object/player.hpp"
#include "object/portable.hpp"
#include "supertux/debug.hpp"
#include "supertux/sector.hpp"
#include "util/reader_mapping.hpp"

BicyclePlatformChild::BicyclePlatformChild(ReaderMapping const& reader, float angle_offset, BicyclePlatform& parent) :
  MovingSprite(reader, "images/objects/platforms/small.sprite", LAYER_OBJECTS, COLGROUP_STATIC),
  m_parent(parent),
  m_angle_offset(angle_offset),
  m_momentum(),
  m_contacts()
{
}

void
BicyclePlatformChild::update(float dt_sec)
{
  float angle = m_parent.m_angle + m_angle_offset;
  angle = math::positive_fmodf(angle, math::TAU);

  Vector dest = m_parent.m_center + Vector(cosf(angle), sinf(angle)) * m_parent.m_radius - (m_col.m_bbox.get_size().as_vector() * 0.5f);
  Vector movement = dest - get_pos();
  m_col.set_movement(movement);
  m_col.propagate_movement(movement);
}

HitResponse
BicyclePlatformChild::collision(GameObject& other, CollisionHit const& )
{
  const float gravity = Sector::get().get_gravity();

  // somehow the hit parameter does not get filled in, so to determine (hit.top == true) we do this:
  auto mo = dynamic_cast<MovingObject*>(&other);
  if (!mo) return FORCE_MOVE;
  if ((mo->get_bbox().get_bottom()) > (m_col.m_bbox.get_top() + 2)) return FORCE_MOVE;

  auto pl = dynamic_cast<Player*>(mo);
  if (pl) {
    if (pl->is_big()) m_momentum += m_parent.m_momentum_change_rate * gravity;
    auto po = pl->get_grabbed_object();
    auto pomo = dynamic_cast<MovingObject*>(po);
    if (m_contacts.insert(pomo).second) {
      m_momentum += m_parent.m_momentum_change_rate * gravity;
    }
  }

  if (m_contacts.insert(&other).second) {
    m_momentum += m_parent.m_momentum_change_rate * Sector::get().get_gravity();
  }

  return FORCE_MOVE;
}

BicyclePlatform::BicyclePlatform(ReaderMapping const& reader) :
  GameObject(reader),
  m_center(0.0f, 0.0f),
  m_radius(128),
  m_angle(0),
  m_angular_speed(0.0f),
  m_momentum_change_rate(0.1f),
  m_children(),
  m_walker(),
  m_platforms(2)
{
  reader.read("x", m_center.x);
  reader.read("y", m_center.y);
  m_radius = reader.get("radius", 128.0f);
  m_momentum_change_rate = reader.get("momentum-change-rate", 0.1f);

  reader.read("platforms", m_platforms);
  m_platforms = std::max(1, m_platforms);

  for (int i = 0; i < m_platforms; ++i) {
    const float offset = static_cast<float>(i) * (math::TAU / static_cast<float>(m_platforms));
    m_children.push_back(&d_sector->add<BicyclePlatformChild>(reader, offset, *this));
  }

  std::string path_ref;
  if (reader.read("path-ref", path_ref))
  {
    d_sector->request_name_resolve(path_ref, [this](UID uid){
        if (!uid) {
          log_fatal("no path-ref entry for BicyclePlatform");
        } else {
          m_walker.reset(new PathWalker(uid, true));
        }
      });
  }
}

BicyclePlatform::~BicyclePlatform()
{
}

void
BicyclePlatform::draw(DrawingContext& context)
{
  if (g_debug.show_collision_rects) {
    context.color().draw_filled_rect(Rectf::from_center(m_center, Sizef(16, 16)), Color::MAGENTA, LAYER_OBJECTS);
  }
}

void
BicyclePlatform::update(float dt_sec)
{
  float total_angular_momentum = 0.0f;
  for (auto const& child : m_children)
  {
    const float child_angle = m_angle + child->m_angle_offset;
    const float angular_momentum = cosf(child_angle) * child->m_momentum;
    total_angular_momentum += angular_momentum;
    child->m_momentum = 0.0f;
    child->m_contacts.clear();
  }

  m_angular_speed += (total_angular_momentum * dt_sec) * math::PI;
  m_angular_speed *= 1.0f - dt_sec * 0.2f;
  m_angle += m_angular_speed * dt_sec;
  m_angle = math::positive_fmodf(m_angle, math::TAU);

  m_angular_speed = std::min(std::max(m_angular_speed, -128.0f * math::PI * dt_sec),
                             128.0f * math::PI * dt_sec);

  if (m_walker)
  {
    m_walker->update(std::max(0.0f, dt_sec * m_angular_speed * 0.1f));
    m_center = m_walker->get_pos(Sizef(), {});
  }
  else
  {
    m_center += Vector(m_angular_speed, 0) * dt_sec * 32.0f;
  }
}

/* EOF */
