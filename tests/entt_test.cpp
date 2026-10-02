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

// Smoke tests for the vendored EnTT, covering the features the ECS
// migration relies on (docs/ecs-migration/kickoff.md).

#include <gtest/gtest.h>

#include <entt/entity/registry.hpp>

namespace {

struct Position { float x; float y; };
struct Velocity { float x; float y; };

/** Component that must keep its address, like CollisionObject, which
    CollisionSystem stores as a raw pointer. */
struct Stable {
  static constexpr auto in_place_delete = true;
  int value;
};

struct Gravity { float value; };

} // namespace

TEST(EnTTTest, view_iterates_matching_entities)
{
  entt::registry registry;

  auto const moving = registry.create();
  registry.emplace<Position>(moving, 0.0f, 0.0f);
  registry.emplace<Velocity>(moving, 2.0f, 3.0f);

  auto const still = registry.create();
  registry.emplace<Position>(still, 5.0f, 5.0f);

  int count = 0;
  for (auto [entity, pos, vel] : registry.view<Position, Velocity>().each()) {
    pos.x += vel.x;
    pos.y += vel.y;
    count += 1;
  }

  EXPECT_EQ(1, count);
  EXPECT_FLOAT_EQ(2.0f, registry.get<Position>(moving).x);
  EXPECT_FLOAT_EQ(3.0f, registry.get<Position>(moving).y);
  EXPECT_FLOAT_EQ(5.0f, registry.get<Position>(still).x);
}

TEST(EnTTTest, destroyed_handle_is_invalid_after_recycling)
{
  entt::registry registry;

  auto const old_entity = registry.create();
  registry.destroy(old_entity);
  auto const new_entity = registry.create();

  // Same slot, new version: stale handles must not resolve (like UID magic)
  EXPECT_EQ(entt::to_entity(old_entity), entt::to_entity(new_entity));
  EXPECT_FALSE(registry.valid(old_entity));
  EXPECT_TRUE(registry.valid(new_entity));
}

TEST(EnTTTest, in_place_delete_keeps_addresses_stable)
{
  entt::registry registry;

  std::vector<entt::entity> entities;
  std::vector<Stable*> pointers;
  for (int i = 0; i < 64; ++i) {
    entities.push_back(registry.create());
    pointers.push_back(&registry.emplace<Stable>(entities.back(), i));
  }

  // Removing entities in the middle must not move the others
  for (int i = 0; i < 64; i += 2) {
    registry.destroy(entities[i]);
  }

  for (int i = 1; i < 64; i += 2) {
    EXPECT_EQ(pointers[i], &registry.get<Stable>(entities[i]));
    EXPECT_EQ(i, pointers[i]->value);
  }
}

TEST(EnTTTest, context_holds_singletons)
{
  entt::registry registry;
  registry.ctx().emplace<Gravity>(10.0f);

  EXPECT_FLOAT_EQ(10.0f, registry.ctx().get<Gravity>().value);
  EXPECT_EQ(nullptr, registry.ctx().find<Velocity>());
}

/* EOF */
