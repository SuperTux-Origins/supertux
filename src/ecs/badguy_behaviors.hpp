//  SuperTux
//  Copyright (C) 2026 Ingo Ruhnke <grumbel@gmail.com>
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
#ifndef HEADER_SUPERTUX_ECS_BADGUY_BEHAVIORS_HPP
#define HEADER_SUPERTUX_ECS_BADGUY_BEHAVIORS_HPP

#include "ecs/badguy_behavior.hpp"
#include "ecs/badguy_components.hpp"

template<> BadGuyBehavior const& behavior_of<Walker>();
template<> BadGuyBehavior const& behavior_of<Floater>();
template<> BadGuyBehavior const& behavior_of<Patrol>();
template<> BadGuyBehavior const& behavior_of<SquishReaction>();
template<> BadGuyBehavior const& behavior_of<Jumper>();
template<> BadGuyBehavior const& behavior_of<Bouncer>();
template<> BadGuyBehavior const& behavior_of<Circler>();
template<> BadGuyBehavior const& behavior_of<Flyer>();
template<> BadGuyBehavior const& behavior_of<ElementalFade>();
template<> BadGuyBehavior const& behavior_of<LoopingSound>();
template<> BadGuyBehavior const& behavior_of<BombCarrier>();
template<> BadGuyBehavior const& behavior_of<Fuse>();
template<> BadGuyBehavior const& behavior_of<Stalactite>();
template<> BadGuyBehavior const& behavior_of<IceBlock>();
template<> BadGuyBehavior const& behavior_of<Boarder>();
template<> BadGuyBehavior const& behavior_of<Sleeper>();
template<> BadGuyBehavior const& behavior_of<BulletShy>();
template<> BadGuyBehavior const& behavior_of<Firecracker>();
template<> BadGuyBehavior const& behavior_of<Snail>();
template<> BadGuyBehavior const& behavior_of<Snowman>();
template<> BadGuyBehavior const& behavior_of<MrTree>();
template<> BadGuyBehavior const& behavior_of<Stumpy>();

namespace stalactite {

constexpr float SHAKE_TIME = .8f;

} // namespace stalactite

namespace walker {

/** Walk towards target_velocity; the movement for this frame must
    already have been applied (BadGuy::active_update). */
void walk(ArchetypeBadguy& self, Walker& walker, float target_velocity, float acceleration);

void turn_around(ArchetypeBadguy& self, Walker& walker);

void initialize(ArchetypeBadguy& self, Walker const& walker);
void collision_solid(ArchetypeBadguy& self, Walker& walker, CollisionHit const& hit);
HitResponse collision_badguy(ArchetypeBadguy& self, Walker& walker, BadGuy& other, CollisionHit const& hit);

} // namespace walker

#endif

/* EOF */
