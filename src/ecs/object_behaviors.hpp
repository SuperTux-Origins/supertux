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
#ifndef HEADER_SUPERTUX_ECS_OBJECT_BEHAVIORS_HPP
#define HEADER_SUPERTUX_ECS_OBJECT_BEHAVIORS_HPP

#include "ecs/object_behavior.hpp"
#include "ecs/object_components.hpp"

#include <memory>

template<> ObjectBehavior const& object_behavior_of<UnstableTile>();
template<> ObjectBehavior const& object_behavior_of<WeakBlock>();
template<> ObjectBehavior const& object_behavior_of<MagicBlock>();
template<> ObjectBehavior const& object_behavior_of<ResetPoint>();

template<> ObjectBehavior const& object_behavior_of<Block>();
template<> ObjectBehavior const& object_behavior_of<BonusBlock>();
template<> ObjectBehavior const& object_behavior_of<Brick>();
template<> ObjectBehavior const& object_behavior_of<InvisibleBlock>();
template<> ObjectBehavior const& object_behavior_of<InfoBlock>();
template<> ObjectBehavior const& object_behavior_of<PathFollower>();
template<> ObjectBehavior const& object_behavior_of<Platform>();
template<> ObjectBehavior const& object_behavior_of<Hurting>();
template<> ObjectBehavior const& object_behavior_of<Coin>();
template<> ObjectBehavior const& object_behavior_of<HeavyCoin>();
template<> ObjectBehavior const& object_behavior_of<Rock>();
template<> ObjectBehavior const& object_behavior_of<Trampoline>();
template<> ObjectBehavior const& object_behavior_of<RustyTrampoline>();

class Crusher;
class GameObject;
class Player;

namespace block {

void start_bounce(ArchetypeObject& self, GameObject* hitter);

} // namespace block

namespace bonus_block {

void try_open(ArchetypeObject& self, Player* player);

} // namespace bonus_block

namespace brick {

void try_break(ArchetypeObject& self, Player* player);
void break_for_crusher(ArchetypeObject& self, Crusher& crusher);

} // namespace brick

class PortableObject;

namespace rock {

/** Accelerate the rock with the wind, up to end_speed */
void add_wind_velocity(ArchetypeObject& self, Vector const& velocity, Vector const& end_speed);

} // namespace rock

namespace trampoline {

/** Create a trampoline at runtime, e.g. from a bonus block */
std::unique_ptr<PortableObject> create(Vector const& pos, bool portable);

} // namespace trampoline

namespace coin {

void collect(ArchetypeObject& self);

/** Spawn a heavy coin with the given initial velocity */
void spawn_heavy(Vector const& pos, Vector const& velocity);

} // namespace coin

namespace path_follower {

/** The path object of the entity, or nullptr */
PathObject* get(GameObject const& self);

} // namespace path_follower

namespace platform {

void goto_node(ArchetypeObject& self, int node_no);
void start_moving(ArchetypeObject& self);
void stop_moving(ArchetypeObject& self);

} // namespace platform

namespace weak_block {

/** Set a weak block on fire (or melting), e.g. by an explosion */
void start_burning(ArchetypeObject& self);

} // namespace weak_block

#endif

/* EOF */
