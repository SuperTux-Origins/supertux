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

#ifndef HEADER_SUPERTUX_OBJECT_BICYCLE_PLATFORM_HPP
#define HEADER_SUPERTUX_OBJECT_BICYCLE_PLATFORM_HPP

#include "object/moving_sprite.hpp"
#include "object/path_walker.hpp"

#include <set>

class BicyclePlatform;

class BicyclePlatformChild : public MovingSprite
{
  friend class BicyclePlatform;

public:
  BicyclePlatformChild(ReaderMapping const& reader, float angle_offset, BicyclePlatform& parent);

  void update(float dt_sec) override;
  HitResponse collision(GameObject& other, CollisionHit const& hit) override;
  bool is_saveable() const override { return false; }

private:
  BicyclePlatform& m_parent;
  float m_angle_offset;
  float m_momentum; /** angular momentum in rad per second per second*/
  std::set<GameObject*> m_contacts; /**< objects that are currently pushing on the platform */

private:
  BicyclePlatformChild(BicyclePlatformChild const&) = delete;
  BicyclePlatformChild& operator=(BicyclePlatformChild const&) = delete;
};

/**
 * Used to construct a pair of bicycle platforms: If one is pushed down, the other one rises
 */
class BicyclePlatform final : public GameObject
{
  friend class BicyclePlatformChild;

public:
  BicyclePlatform(ReaderMapping const& reader);
  ~BicyclePlatform() override;

  void draw(DrawingContext& context) override;
  void update(float dt_sec) override;

private:
  Vector m_center; /**< pivot point */
  float m_radius; /**< radius of circle */

  float m_angle; /**< current angle */
  float m_angular_speed; /**< angular speed in rad per second */

  float m_momentum_change_rate; /** Change in momentum every step **/

  std::vector<BicyclePlatformChild*> m_children;
  std::unique_ptr<PathWalker> m_walker;
  int m_platforms;

private:
  BicyclePlatform(BicyclePlatform const&) = delete;
  BicyclePlatform& operator=(BicyclePlatform const&) = delete;
};

#endif

/* EOF */
