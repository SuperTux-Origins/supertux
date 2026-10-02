# ECS migration: how to start

Companion to `plan.md` (long-range target architecture) and `audit.md`
(verified facts). This file changes the plan's **ordering** based on the
audit, and lists the concrete first commits.

## What changes vs. plan.md

`plan.md` goes horizontally: shadow-mirror physics → collision → render
→ camera for *every* object (Phase 1), then Player (Phase 2), then
archetypes (Phase 3), then badguys (Phase 4). That spends a long time
syncing mirrored state with nothing to show for it, and it starts with
the most entangled code (collision, Player).

Instead: **go vertical**. Build the minimal ECS spine, then migrate one
complete cluster end-to-end (components + system + data archetype +
deleted C++ classes). That proves every layer (registry, archetype
loading, collision bridging, golden master) early, on a small surface.

Further adjustments from the audit:

1. **Golden master reuses demos.** Demo record/playback, the seeded RNG,
   the fixed timestep and the null renderer already exist. Add only a
   state dump and an exit-at-end-of-demo mode.
2. **Byte-identical is the default, not an absolute.** `GameObjectManager`
   updates in insertion order, and interactions (riding platforms,
   carrying, stomping) depend on it. Batching updates into
   per-component system passes reorders them. Rule: while a slice is
   being migrated, call the system function *per entity from the shell
   object's `update()`* (same order → identical dump); batching into a
   real system pass is a separate commit whose diff is reviewed and
   re-baselined explicitly.
3. **Collision stays where it is for now.** `CollisionSystem` works on
   stable `CollisionObject*` with listener callbacks. Do not move
   `CollisionObject` into the registry until late (it would need EnTT's
   `in_place_delete` pointer stability anyway). Replace the
   `dynamic_cast` partner dispatch gradually with component queries
   (`registry.all_of<Stompable>(e)`), which is where most of the
   coupling (208 casts) actually lives.
4. **Savegames are unaffected** (no class names are serialized), so plan
   Open Decision #3 is moot.
5. **Archetype format = the existing S-expression reader** (same as
   `.stlv`, `.sprite`), so there are no new parser deps on the five targets.

## Step 0 — Safety net (commits 1–3)

**0.a Golden-master harness**
- `--dump-state FILE`: every logical frame, write `frame uid type x y vx vy
  action` for each `MovingObject` (sorted by UID) plus the player state.
- `--play-demo` + `--renderer null` + exit when the demo input runs out
  (or after `--max-frames N`).
- `tools/golden.sh record|check`: run a fixed demo set, diff against
  `tests/golden/*.dump`.
- Record ~6 demos covering the walker cluster: `world1` levels heavy in
  snowball / spiky / mriceblock / walkingleaf, plus one level with
  platforms and one with bosses for broad regression coverage.
- First prove determinism: the same demo played twice must produce
  identical dumps. Fix any nondeterminism found (e.g. `graphicsRandom`
  leaking into gameplay, iteration over unordered containers) before
  going further.

**0.b EnTT dependency**
- `find_package(EnTT CONFIG)` with fallback to `external/entt`
  (header-only, same `EXISTS` pattern as the other externals);
  add `entt` to the Nix build inputs; add `tests/entt_test.cpp`.
- Build check on all five targets once (record any quirks in PORTING.md).

**0.c Dead-code removal** (shrinks the surface before touching it)
- Delete the `editor_*`, `has_settings`, `is_saveable` hooks on
  `GameObject` and their overrides.
- Fold `Portable` into a plain member/flag and delete
  `GameObjectComponent` (its only user), freeing the word "component"
  for ECS components.

## Step 1 — ECS spine (commits 4–5)

- `entt::registry` owned by `GameObjectManager` (so both `Sector` and
  `WorldMapSector` get one).
- Every `GameObject` gets an `entt::entity`, created in
  `flush_game_objects()` on add, destroyed on removal. The entity holds a
  `ObjectRef { GameObject* }` component back to the shell.
- **UID stays the external handle** (scripts, `TypedUID<T>`, level
  references). The `UID → entity` mapping lives in the manager. No
  call-site churn.
- `PhysicsBody` is the first real component: `Physic` moves into the
  registry for `MovingObject`s with physics; `get_physic()` returns a
  reference into the registry. `Physic` is a value type with no
  back-pointers, so it is safe to own in EnTT storage. Golden master must
  be identical.

## Step 2 — First vertical slice: ground walkers (commits 6–10)

Target: `snowball` (841 uses), `spiky` (451), `walkingleaf` (439),
`poisonivy` (388, factory alias of `ViciousIvy`), `crystallo`: thin
`WalkingBadguy` subclasses (`snowball.cpp` is 41 lines: sprite, speed,
squish reaction). `sspiky` (114 lines, sleeps until woken) is the
stretch goal. 22 classes derive from `WalkingBadguy`; the rest (mrbomb,
mriceblock, captainsnowball, …) carry extra state and come later.

1. Components: `Walker { speed, turn_at_ledge, max_drop_height }`,
   `Stompable { action, kill_mode }`, `SpriteRef { file, left/right actions }`.
2. `walker_system(registry, entity, dt)` — logic lifted from
   `WalkingBadguy::active_update`/`collision_solid`, called per entity
   from the shell (order preserved).
3. `BadGuy` collision hooks consult components (`Stompable`) before
   falling back to the virtual.
4. Archetype registry: `data/archetypes/*.arch` S-expressions, e.g.

   ```lisp
   (archetype
     (name "snowball")
     (base "badguy")
     (sprite "images/creatures/snowball/snowball.sprite")
     (walker (speed 80) (turn-at-ledge #f))
     (stompable (action "squished") (kill "squished")))
   ```

   `GameObjectFactory` checks the archetype registry first, falls back to
   the class map. One generic `ArchetypeBadguy` shell carries the
   components.
5. Per enemy: add the archetype, delete the C++ class, golden master
   identical, tick `badguy-checklist.md`.

Exit: 5–6 badguy classes deleted, levels unchanged, golden master
identical, all five targets build.

## After the slice

Re-evaluate against `plan.md` with real data. The likely continuation:
more badguy clusters (jumpers, flyers) → replace `dynamic_cast<Player>`
dispatch with components → non-badguy objects (blocks, platforms via a
`PathFollower`) → Player decomposition last, once the collision bridge
has matured → scripting bindings per component.

## Decisions for the maintainer

1. Vertical slice first (this doc) vs. plan.md's horizontal Phase 1?
2. EnTT vs. a small hand-rolled registry? (Recommend EnTT: in nixpkgs,
   header-only, widely used.)
3. Gameplay must stay byte-identical through the migration, or are
   reviewed one-frame ordering diffs acceptable once systems are batched?
4. Objects unused in shipped levels (see audit): migrate, or delete
   up front to shrink the work?
