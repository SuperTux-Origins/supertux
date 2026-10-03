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
#include "ecs/object_components.hpp"

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
template<> BadGuyBehavior const& behavior_of<JumpingFish>();
template<> BadGuyBehavior const& behavior_of<Hopper>();
template<> BadGuyBehavior const& behavior_of<Bobber>();
template<> BadGuyBehavior const& behavior_of<DartShooter>();
template<> BadGuyBehavior const& behavior_of<Projectile>();
template<> BadGuyBehavior const& behavior_of<Toad>();
template<> BadGuyBehavior const& behavior_of<Mole>();
template<> BadGuyBehavior const& behavior_of<Skydive>();
template<> BadGuyBehavior const& behavior_of<Owl>();
template<> BadGuyBehavior const& behavior_of<Ghoul>();
template<> BadGuyBehavior const& behavior_of<PathFollower>();
template<> BadGuyBehavior const& behavior_of<Dispenser>();
template<> BadGuyBehavior const& behavior_of<WillOWisp>();
template<> BadGuyBehavior const& behavior_of<Diver>();
template<> BadGuyBehavior const& behavior_of<Haywire>();
template<> BadGuyBehavior const& behavior_of<GoldBomb>();
template<> BadGuyBehavior const& behavior_of<LiveFire>();

namespace stalactite {

constexpr float SHAKE_TIME = .8f;

} // namespace stalactite

namespace dispenser {

/** A badguy spawned by the dispenser with this entity died */
void notify_dead(entt::entity dispenser);

/** Start or stop dispensing, also from scripts */
void activate(ArchetypeBadguy& self);
void deactivate(ArchetypeBadguy& self);

} // namespace dispenser

namespace willowisp {

/** Fade out and stop chasing the player */
void vanish(ArchetypeBadguy& self);

/** Script interface; see scripting::WillOWisp */
void goto_node(ArchetypeBadguy& self, int node_no);
void set_state(ArchetypeBadguy& self, std::string const& state);
void start_moving(ArchetypeBadguy& self);
void stop_moving(ArchetypeBadguy& self);

} // namespace willowisp

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
