//  SuperTux - Particle spawn zone
//  Copyright (C) 2020 A. Semphris <semphris@protonmail.com>
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

#include "object/particle_zone.hpp"

#include "supertux/resources.hpp"
#include "util/reader_mapping.hpp"
#include "video/drawing_context.hpp"


ParticleZone::ParticleZone(const ReaderMapping& reader) :
  MovingObject(reader),
  //ExposedObject<ParticleZone, scripting::Wind>(this),
  m_enabled(),
  m_particle_name(),
  m_type()
{
  float w,h;
  reader.get("x", m_col.m_bbox.get_left(), 0.0f);
  reader.get("y", m_col.m_bbox.get_top(), 0.0f);
  reader.get("width", w, 32.0f);
  reader.get("height", h, 32.0f);
  m_col.m_bbox.set_size(w, h);

  reader.get("enabled", m_enabled, true);
  reader.get("particle-name", m_particle_name, "");

  std::string zone_type;
  if (reader.get("zone-type", zone_type))
  {
    if (zone_type == "destroyer")
    {
      m_type = ParticleZoneType::Destroyer;
    }
    else if (zone_type == "killer")
    {
      m_type = ParticleZoneType::Killer;
    }
    else if (zone_type == "life")
    {
      m_type = ParticleZoneType::Life;
    }
    else if (zone_type == "life-clear")
    {
      m_type = ParticleZoneType::LifeClear;
    }
    else
    {
      m_type = ParticleZoneType::Spawn;
    }
  }
  else
  {
    m_type = ParticleZoneType::Spawn;
  }

  set_group(COLGROUP_TOUCHABLE);
}

void
ParticleZone::update(float dt_sec)
{
  // This object doesn't manage creating particles :)
  // See `src/object/custom_particle_system.*pp` for that
}

void
ParticleZone::draw(DrawingContext& context)
{
}

HitResponse
ParticleZone::collision(GameObject& other, const CollisionHit& hit)
{
  return ABORT_MOVE;
}

/* EOF */
