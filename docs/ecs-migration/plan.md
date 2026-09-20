# SuperTux-Origins: OOP → ECS / Data-Driven Migration Plan

**Repo:** `git@github.com:SuperTux-Origins/supertux.git`
**Audience:** an LLM (e.g. Claude Code) working through this repo commit-by-commit, plus the human maintainer reviewing PRs.
**Scope note:** This version of the plan is grounded in the actual `src/` tree (provided as a tarball) — every class name, file path, and line count below is verified against real source, not inferred from upstream knowledge. Phase 0's audit task is correspondingly lighter than a from-scratch audit would be (see §0.5 below for what's still worth double-checking, e.g. build files and `data/` weren't in the tarball).

---

## 0. What the actual source tells us

The uploaded tarball contains `src/` (no `data/`, `CMakeLists.txt`, `tests/`, or `AGENTS.md` — those weren't in the archive; Phase 0 should still fetch those from the real repo). Top-level `src/` layout:

```
audio/  badguy/  collision/  control/  gui/  math/  object/  physfs/
port/   scripting/  sprite/  squirrel/  supertux/  trigger/  util/  video/  worldmap/
```

Notable, confirmed facts that change the plan for the better:

- **No `editor/` directory** — confirms the README's claim that the in-game editor was removed. But it wasn't fully excised: `GameObject` still declares dead virtual hooks `editor_delete()`, `editor_select()`, `editor_deselect()`, `editor_update()`, `has_settings()`, `is_saveable()`, and there are ~44 files with stray `editor` references (comments, dead branches, a `Ghoul` FIXME about not overriding a `get_settings()` that no longer exists anywhere). **The property-reflection system the editor used to use for generic "edit any object's fields" UI (`ObjectSettings`/`get_settings()`) has been deleted along with the editor.** This matters: it means Phase 3's data-driven archetype/override mechanism has to be built from scratch — there is no existing generic reflection layer to repurpose, contrary to what a "the editor already does this" assumption would suggest.
- **`GameObject` already has a rudimentary component system.** `supertux/game_object.hpp` defines an (currently-empty-interface) `GameObjectComponent` base class, and `GameObject` holds `std::vector<std::unique_ptr<GameObjectComponent>>` with `add_component()`/`get_component<T>()` (via `dynamic_cast`) /`remove_component()`. This is genuine, if minimal, prior art for composition — it's *heterogeneous OOP composition per-instance*, not a data-oriented registry systems can iterate over, but it means "components" as a concept and a bit of plumbing already exist in this codebase's vocabulary. The ECS migration should either grow into this abstraction or consciously replace it — see Open Decision #6.
- **Object identity already goes through a handle, not a raw pointer, almost everywhere.** `util/uid.hpp` defines `UID` (an opaque `uint32_t` handle with a "magic" generation-check byte packed in), and `util/typed_uid.hpp` defines `TypedUID<T>`, which behaves like a `T*` (`operator->`, `operator*`, `operator bool`) but is actually backed by `Sector::get().get_object_by_uid<T>(uid)` under the hood — i.e. **this codebase already implements the "store an entity handle, resolve to a live object each use, tolerate the referent having been destroyed" pattern that a from-scratch ECS migration normally has to introduce as brand-new plumbing.** `GameObjectManager` (the `Sector`'s object store) maintains `m_objects_by_uid`, `m_objects_by_name`, and `m_objects_by_type_index` maps alongside the master `std::vector<std::unique_ptr<GameObject>>`. This is very good news for risk: the single riskiest class of migration bug (dangling references when converting "owns a pointer" to "owns an ID") is largely *already solved* here. The ECS migration's `entt::entity` can, in most call sites, be a fairly mechanical swap-in for `UID`/`TypedUID<T>`, not a new concept the rest of the code has to learn.
- **Squirrel bindings already store a `UID`, not a pointer.** `squirrel/exposed_object.hpp`'s `ExposedObject<S, T>::expose()` constructs the scripting-facing wrapper as `std::make_unique<T>(m_parent->get_uid())` — so Phase 5 (scripting bridge) is smaller than a typical ECS migration's equivalent phase: the lifetime-safety problem is already solved, what's left is de-duplicating the ~71 files in `scripting/` (one hand-written binding class per exposed `GameObject` subclass) into a smaller number of component-driven bindings.
- **Physics and rendering state are already extracted into plain, non-virtual member objects.** `supertux/physic.hpp`'s `Physic` class is exactly `{ax, ay, vx, vy, gravity_enabled_flag, gravity_modifier}` plus `get_movement(dt)` — no virtual methods, no inheritance, trivially copyable into an ECS `PhysicsBody` component pretty much as-is. Likewise `Sprite` (in `sprite/`) and `collision/collision_object.hpp`'s `CollisionObject` are discrete, non-polymorphic-in-the-relevant-sense member objects already. These three are the lowest-risk, highest-confidence first targets for Phase 1.
- **The class-per-enemy problem is real and precisely as large as expected.** `badguy/` contains **59 concrete enemy subclasses** (full list captured in `docs/ecs-migration/badguy-checklist.md`, seeded below) plus the `BadGuy` base (`badguy.cpp` is 932 lines) and a shared `walking_badguy.hpp`/`.cpp` intermediate base already used by several ground-patrol enemies — i.e. there's already an informal "shared behavior base class" pattern for at least the walker cluster, which is a good sign that clustering by behavior (§4.1) will find natural groupings quickly.
- **`Player` is exactly the monolith expected, and larger than typical.** `object/player.cpp` is **2,603 lines**; `object/player.hpp` is 388 lines. This is the single largest, riskiest file in the codebase and should be treated with the most caution in Phase 2 — do not attempt this in one PR.
- **Object creation-by-name is a single, small, centralized map**, not scattered `if/else` chains: `supertux/object_factory.hpp`'s `ObjectFactory` holds `std::map<std::string, std::function<std::unique_ptr<GameObject>(ReaderMapping const&)>>`, populated via `add_factory<ConcreteClass>(name)`, and `GameObjectFactory` (in `supertux/game_object_factory.hpp/.cpp`) is the concrete singleton used at load time. This is actually a *clean* seam — Phase 3's archetype registry can slot in as an alternative/extension to this exact map without fighting scattered dispatch logic.
- **`object/` (non-badguy `GameObject` subclasses) is large too** — roughly 100+ classes covering blocks, platforms, particle systems, triggers, decorations, lights, the player, path-following helpers, etc. Many of these (e.g. `path_walker`, `path_object`, `platform`, `pneumatic_platform`, `bicycle_platform`) already look like they'd cleanly decompose into a `PathFollower` component + system rather than needing bespoke treatment.
- **`gui/`** is the in-game menu system (main menu, options, dialogs) — unrelated to gameplay entities, out of scope for this migration.
- **`port/` contains only `emscripten.hpp`** — worth flagging to the maintainer given the README says "abandonment of most ports," since a WASM target has real implications for which ECS library and threading approach are safe to use (Phase 0.6 below). **Update: now confirmed to matter — see §0.1a.**

### 0.1a. Conflict: README says "abandon most ports," AGENTS.md + CMakeLists.txt say otherwise — resolve before Phase 0

Two documents from the same repo disagree, and this isn't cosmetic:

- The public **README** (fetched earlier from GitHub) states the fork's goal includes "abandonment of most ports," focusing on Linux/NixOS.
- The repo's own **`AGENTS.md`** (provided directly) describes an active porting project targeting **five** platforms: Desktop Linux (OpenGL 3.3 + GLEW), WebAssembly/Emscripten (WebGL/GLES2), Android (NDK, GLES2), R36S/ArkOS handhelds (aarch64, GLES2, a hybrid toolchain against an older sysroot), and Windows (MinGW cross-build) — explicitly modeled on the `Pingus`/`Windstille` projects' `mk/`/`nix/` packaging patterns.
- The provided **`CMakeLists.txt`** proves this isn't aspirational: it already contains live, non-trivial logic for all five — `if(EMSCRIPTEN)` blocks setting `FULL_ES2` and exported runtime functions, `if(ANDROID OR DEFINED ANDROID)` forcing GLES2, a `SUPERTUX_R36S` option with its own ABI-shim source file (`mk/r36s/dl_find_object_stub.c`, for linking GCC 15 headers against an older sysroot), and `if(WIN32) / if(MINGW)` sections. This is a real, in-progress, substantially-built-out effort, not a stub.
- Corroborating evidence already seen in the source: `util/uid.hpp` has a comment explicitly working around an NDK libc++ r26 `std::format` limitation — i.e. Android is a toolchain someone has actually been building against recently, not a someday-goal.

**This needs a maintainer decision before Phase 0 starts**, because it changes the ECS migration's verification surface substantially: "does it build and play the same" becomes a five-platform question, not a one-platform question, if the porting effort is the current priority. See Open Decision #8.

**Revised core diagnosis (now evidence-based, not inferred):** the "spaghetti" is narrower and more tractable than a generic OOP-to-ECS rewrite would suggest. The state that needs to be data (physics, collision, sprite/animation) is *already* extracted into clean, non-polymorphic member objects. The reference-safety problem that usually makes these migrations dangerous is *already solved* via `UID`/`TypedUID`. What's actually spaghetti is narrower and specific: **(a)** 59 enemy behaviors expressed as 59 C++ classes instead of composable data, **(b)** a 2,600-line `Player` class doing too many jobs in one file, **(c)** name→concrete-class factory dispatch instead of name→data-archetype dispatch, and **(d)** ~71 hand-written per-class scripting bindings instead of a handful of component-driven ones. That's a real, substantial rewrite — but a bounded and well-scoped one, not a "replace the whole engine" one.

### 0.1b. Build system facts from the actual `CMakeLists.txt`

- **C++20**, not C++17 (`set(CMAKE_CXX_STANDARD 20)`, required, no GNU extensions). Good news for the ECS work — EnTT supports C++20 cleanly, and concepts/ranges are available if useful for system definitions — but it raises the bar on toolchain compatibility across the five platforms in §0.1a (older/cross sysroots need to actually support C++20, which the existing NDK-libc++ workaround in `uid.hpp` suggests is already a live source of friction).
- Depends on `tinycmmc` (`find_package(tinycmmc CONFIG)`), confirming `AGENTS.md`'s note that the project is moving toward `tinycmmc` and away from some legacy Boost usage. Any new CMake wiring for EnTT (Phase 0.4) should follow the existing `tinycmmc`/`mk/cmake` module conventions already in use, not invent a new pattern.
- Version is sourced from a single top-level `VERSION` file, with Nix flakes appending a dev-build suffix — consistent with `AGENTS.md`'s "Top-level `VERSION` is the only source of truth."
- Platform branches are extensive and specific: Emscripten gets its own SDL2_image stub (`mk/emscripten/sdl_image_stub.c`) and an explicit `EXPORTED_FUNCTIONS` list for JS interop (audio pause/resume, download progress, config save) — the web build has real JS-boundary surface, not just a recompile target. R36S has its own ABI-shim translation unit (`mk/r36s/dl_find_object_stub.c`) bridging a newer GCC against an older sysroot. This confirms the porting effort in §0.1a is deep enough that "build the ECS code on Linux and assume the rest follows" is not a safe assumption for anything touching `video/`, `audio/`, or platform-conditional headers — which, unfortunately, includes exactly the `Sprite`/`Renderable` component work in Phase 1.

---

## 1. Guiding principles for the whole migration

1. **Strangler-fig, not big-bang.** The game must build and be playable after every merged phase. Never let `master` sit in a half-migrated, non-compiling state for more than one PR's lifetime.
2. **Data first, structure second, cleanup third.** For each subsystem: (a) introduce the ECS component(s) and a system that reads/writes them, (b) migrate the existing OOP classes to be thin adapters that populate those components instead of doing per-frame logic themselves, (c) once nothing references the old virtual methods, delete the old class.
3. **One migration axis at a time.** Don't try to convert physics, rendering, and AI simultaneously. Land physics as components fully before starting on badguy AI.
4. **Content changes are a forcing function, not a side effect.** Every enemy/object converted to data-driven form should be *validated* by actually deleting the C++ subclass and proving the game still plays identically (or documenting the intended behavior change).
5. **No parallel "new engine" tree.** Do not build ECS-SuperTux in a subdirectory and swap it in later — that always rots. Migrate the live `src/` tree incrementally.
6. **Golden-master tests are the safety net.** Before touching gameplay-affecting code, build a deterministic replay harness (Phase 0) so every subsequent phase can be checked by diffing recorded entity trajectories, not just "does it compile."
7. **Prefer composition data that a designer could plausibly hand-edit.** The end state should let someone add a new enemy by writing a data file (archetype + component overrides) referencing existing systems, not by writing C++.

---

## 2. Target architecture

### 2.1 ECS core

- Adopt **EnTT** (header-only, MIT-licensed, C++17, extremely widely used for exactly this kind of migration, minimal build-system impact — drops into `external/` alongside the existing vendored Squirrel). Do not write a bespoke ECS; that's a multi-month distraction from the actual goal. (Check WASM/emscripten compatibility explicitly per Open Decision below, since `port/emscripten.hpp` suggests that target may still matter — EnTT is generally emscripten-friendly, but verify with a throwaway build before committing.)
- `entt::registry` per `Sector` (one registry per active level/worldmap "room"), owned where `GameObjectManager`'s `m_gameobjects` vector currently lives.
- Entities are `entt::entity` handles. **This is a smaller conceptual leap here than in most codebases**, because the code already thinks in terms of opaque handles resolved through a registry: `UID` + `TypedUID<T>` (`util/uid.hpp`, `util/typed_uid.hpp`) already implement "store a handle, resolve via `Sector::get().get_object_by_uid<T>()`, tolerate the referent being gone" exactly as `entt::entity` + `registry.valid()`/`registry.try_get<T>()` would. The pragmatic migration path is: keep `UID` as the *externally-visible* handle type used by scripting and cross-object references (minimal call-site churn), and internally maintain a `UID → entt::entity` map (or store the `entt::entity` value inside `UID`'s 32 bits directly, since both are already small opaque integers — worth prototyping both and picking whichever needs fewer call-site changes in Phase 0.1). Do **not** force a mechanical rename of every `UID`/`TypedUID<T>` use to `entt::entity` as a first step; that's churn without benefit. Only touch a given reference site when its owning subsystem is actually being migrated in that phase.

### 2.2 Core components (Phase 1 target)

| Component | Fields | Replaces |
|---|---|---|
| `Transform` | position (Vector2f), previous position (for interpolation), rotation if needed | `GameObject`/`MovingObject` position bookkeeping |
| `Hitbox` | width/height or polygon, offset | `MovingObject` bounding box |
| `PhysicsBody` | velocity, acceleration, gravity scale, "on ground" flag | existing `Physic` class (mostly a rename/relocate, low risk) |
| `Collider` | collision group/mask, solid/one-way/trigger flag | `CollisionObject` |
| `Sprite` | sprite resource handle, current action/animation, flip, layer/z-order | existing `Sprite` class (again, mostly relocate) |
| `Renderable` | draw layer, visibility flag, opacity/color mod | scattered ad-hoc fields across `GameObject` subclasses |
| `Health` | hit points, invulnerability timer | scattered per-badguy fields |
| `Tag`/`Name` | string identifier for scripting lookup | `GameObject::get_name()` / `GameObjectManager::m_objects_by_name` |
| `ScriptExposed` | marks entity as reachable from Squirrel, stores its exposed-name table | current per-class `ExposedObject<S,T>` binding (`squirrel/exposed_object.hpp`) — note this already stores a `UID`, not a pointer, so this row is a de-duplication task more than a safety fix |

Note on `GameObjectComponent` (`supertux/game_object_component.hpp`): this existing (currently empty-interface) type is *not* the same thing as an ECS component even though the name suggests it — it's a heterogeneous, per-instance, `dynamic_cast`-queried composition slot owned by one `GameObject`, not a data-oriented array a system iterates over. Recommendation (confirm with maintainer, see Open Decisions): retire `GameObjectComponent` in favor of real EnTT components as each subsystem migrates, rather than trying to make EnTT components inherit from it — bolting a registry-based system onto an OOP composition interface would just create a second, confusing meaning for "component" in the same codebase.

### 2.3 Behavior/AI components (Phase 4 target — replaces the `BadGuy` subclass explosion)

Instead of one C++ class per enemy, an enemy becomes an **entity archetype**: a named bag of components assembled from data. Behavior is expressed via small, reusable components + systems, e.g.:

- `Walker { speed, turn_on_edge: bool, turn_on_wall: bool }` — generic ground-patrol system
- `Flyer { pattern: enum/curve-ref, amplitude, period }`
- `Jumper { interval, height }`
- `Stompable { on_stomp: enum(Kill|Flatten|Bounce|Ignore), points }`
- `ContactDamage { amount, knockback }`
- `Projectile { speed, lifetime, on_hit: enum }`
- `StateMachine { current_state, states: data table of {on_enter, on_update, transitions} }` for enemies with genuinely bespoke multi-phase behavior (bosses) that can't be captured by small composable components alone.
- `Dies { death_anim, sound, spawns_on_death: [archetype refs] }`

A "Jumpy" enemy becomes: `Transform + Hitbox + PhysicsBody + Collider + Sprite + Health(1) + Jumper + Stompable(Kill) + ContactDamage(1)` — zero bespoke C++.
A boss with truly unique multi-phase logic keeps a `StateMachine` component whose states are still data, with the *transition conditions* as small composable predicates rather than a hand-written subclass.

**Rule of thumb for the migration:** if two or more existing `BadGuy` subclasses differ only in constants (speed, sprite, points, hitbox), they collapse into *one* archetype definition with different data. If behavior genuinely differs in control flow, it becomes a new small component + system, reusable by future enemies — not a new class.

### 2.4 Data-driven definitions

- Introduce an `archetypes/` data directory (format: reuse the project's existing s-expression parser to minimize new dependencies, or move to a simpler format — see Open Decisions — but keep it text, diffable, and hand-editable).
- An archetype file declares: archetype name, list of components + default field values, optional parent archetype (for "SnowBall" vs. "FastSnowBall" sharing a base).
- Level files (`.stl`) keep spawning objects by name, but the name now resolves through an **archetype registry** (name → component list) instead of a **class registry** (name → `new SomeBadguySubclass()`). Per-instance overrides in the level file (position, direction, custom speed, etc.) apply as component field overrides on top of the archetype.
- This is the single change that turns "add a new enemy" from "write C++, recompile, ship a binary" into "write a data file."

### 2.5 Systems (replace virtual `update()`/`draw()`)

Systems are plain functions operating on component views, run in a fixed, explicit order each frame from a central `GameLoop`/`Sector::update()`:

1. `InputSystem` (player only)
2. `AIBehaviorSystem`(s) — one per behavior component type, or a small dispatch table (`Walker`, `Flyer`, `Jumper`, `StateMachine`, ...)
3. `PhysicsIntegrationSystem` — apply gravity/acceleration to velocity, velocity to position
4. `CollisionDetectionSystem` — broad + narrow phase against tilemap and other colliders
5. `CollisionResponseSystem` — resolve overlaps, fire `Stompable`/`ContactDamage`/trigger events
6. `HealthSystem` — apply damage events, handle death → spawn death effects/drops, mark entity for destruction
7. `AnimationSystem` — pick sprite action from state (falling/walking/etc.)
8. `CameraSystem`
9. `ScriptBridgeSystem` — flush any script-queued mutations, expose newly created entities to Squirrel
10. `RenderSystem` — draw pass, sorted by layer/z
11. `CleanupSystem` — actually destroy entities marked dead, at a single well-defined point per frame (never mid-iteration)

Systems should be free functions or small structs taking `(entt::registry&, float dt)` — not classes with inheritance, not singletons beyond what's already unavoidable (audio, resource caches).

### 2.6 Scripting bridge

- Squirrel scripts currently call typed methods on specific `GameObject` subclass instances. Under ECS, a script reference should resolve to an `entt::entity` + registry, with exposed script methods implemented as free functions that read/write components (e.g. `door.set_open(true)` becomes a function that flips a `DoorState` component field on the entity the script handle wraps).
- Keep the **existing Squirrel API surface stable where the fork still uses it** (level scripts already written for this fork shouldn't need edits) — this is a binding-layer change, not a scripting-language or level-content change, except where content is intentionally being cut per the fork's stated goals.
- Migrate binding registration from "one hand-written class per exposed object type" to a small number of generic bindings driven by which components an entity has (e.g. any entity with a `DoorState` component automatically gets `open()`/`close()` exposed).

---

## 3. Phased execution plan

Each phase below is written as a set of concrete, independently reviewable tickets. Sizes assume an LLM agent with repo access doing the work in a branch-per-ticket workflow, human review at each PR.

### Phase 0 — Ground truth & safety net (do not skip)

Much of the class-inventory work a "Phase 0 audit" normally does has already been done for you in §0 above, from the actual `src/` tree. What's left is the parts that weren't in the source-only tarball, plus the non-negotiable safety net:

- **0.1 Fill the gaps.** Pull `CMakeLists.txt` (or `mk/`/`flake.nix` build definitions), `data/` (to know which archetypes/levels actually ship post-content-cuts — needed for Phase 3.4 and Phase 4's clustering priority), `tests/` (any existing tests to preserve), and `AGENTS.md` (repo-specific agent conventions — fold into this plan, flag conflicts to the human) from the real repo. Update `docs/ecs-migration/audit.md` (create it) with these plus a copy of the confirmed findings from §0 of this plan, so the audit doc — not this plan file — becomes the living source of truth as the migration proceeds.
- **0.2 Build & run baseline — on every platform actually in scope.** Given §0.1a/§0.1b, this is no longer a single-platform check. Confirm `nix develop && cmake && make` on desktop Linux, and confirm (or get the maintainer's explicit sign-off to skip, per Open Decision #8) build success on Emscripten, Android NDK, R36S/ArkOS, and MinGW cross-compile — using whatever recipe `PORTING.md`/`PORTS.md`/the `nix/` flake outputs already define, per `AGENTS.md`'s "don't invent new solutions" instruction. Record exact toolchain versions for all five.
- **0.3 Golden-master replay harness.** Build a minimal deterministic test mode: fixed timestep, fixed input script (a recorded sequence of button presses) played against a fixed level, dumping per-frame entity positions/states (player + all badguys, keyed by `UID` so the dumps remain comparable across the migration) to a file. Run this against a handful of representative levels (one per world/theme still present after the content cuts, per `data/`) *before any refactor* and commit the baseline dumps under `tests/golden/`. Primary target for this harness is desktop Linux (fastest iteration); treat it as the gate for every phase. Cross-platform parity (does the WASM/Android/R36S build produce the same golden-master output) is a valuable but secondary check — run it at phase boundaries (end of Phase 1, end of Phase 4, end of Phase 6), not on every single task, given there's no CI to automate this and it must be done manually or via local scripts per `AGENTS.md`.
- **0.4 Add EnTT.** Vendor into `external/entt/` (alongside the existing vendored `squirrel/`/`tinycmmc` external deps, following the same integration pattern, i.e. wire through `mk/cmake` modules rather than a bespoke `find_package`). Add a trivial "hello world" unit test (create registry, create entity, add/query a dummy component) under `tests/` to prove the build integration works before it's load-bearing. **Given §0.1a, this must include a real build check on Emscripten, Android NDK, and ideally R36S's older sysroot before EnTT is considered safe to build on** — EnTT is header-only, C++20-compatible, and has no OpenGL/platform dependency of its own, so it should be fine everywhere, but "should be fine" is exactly the kind of assumption `uid.hpp`'s NDK-libc++ workaround shows has already bitten this project once. Don't skip this check to save time.
- **0.5 Local pre-merge check script.** Since GitHub CI was removed and the actual workflow is git-bundle-based (see §6a), add a single script (`tools/check.sh` or similar) that builds Linux + runs unit tests + runs the golden-master harness, so an agent has one command to self-verify before assembling a bundle. Cross-platform builds (0.2) don't need to be in this fast inner-loop script — call them out as a separate, periodic check.
- **0.6 Decide the `UID` ↔ `entt::entity` bridging strategy** (see §2.1's note) before Phase 1 starts — this is a one-way-door-ish decision that every later phase depends on, so it deserves an explicit, written decision in the audit doc, not an implicit choice made mid-Phase-1.
- **0.7 Resolve the README-vs-AGENTS.md scope question (§0.1a) with the maintainer before committing to a build-verification scope for the rest of this plan** — this determines how much of 0.2/0.4's multi-platform checking is actually required per-task vs. per-phase-boundary vs. skippable for now.

### Phase 1 — Core movement/physics/collision as components (no gameplay change)

Goal: every `MovingObject` gets a **shadow** `Transform`/`PhysicsBody`/`Collider`/`Hitbox` component set that mirrors its existing C++ member state, systems compute the same math, but the legacy virtual `update()` still runs and is still authoritative — components are write-only mirrors, checked against golden-master dumps for exact equality. This "shadow mode" step exists purely to prove the ECS math matches the legacy math before anything depends on it.

- 1.1 Add the components from §2.2.
- 1.2 Add `PhysicsIntegrationSystem` that reads from the *legacy* `Physic` object each frame and writes into the shadow `PhysicsBody`/`Transform` (i.e., a one-way sync, not yet driving anything).
- 1.3 Extend the golden-master harness to also assert shadow-component values equal legacy values every frame; fix any drift.
- 1.4 Flip authority: make `PhysicsIntegrationSystem` compute motion itself; legacy `Physic`/position fields become the mirrored (read-only, deprecated) side. Re-run golden-master; must be byte-identical or intentionally-explained.
- 1.5 Repeat 1.1–1.4 for collision (`Collider`/`Hitbox` + `CollisionDetectionSystem`/`CollisionResponseSystem`), then rendering (`Sprite`/`Renderable` + `RenderSystem`), then camera.
- 1.6 Once physics/collision/render/camera are ECS-authoritative for all entity types, delete the now-dead legacy fields/methods from `GameObject`/`MovingObject`/`MovingSprite`. `GameObject` should shrink to essentially nothing (maybe just "has an entity id" bookkeeping) by the end of this phase.

Exit criteria: game plays identically per golden-master; `GameObject`/`MovingObject`/`MovingSprite` no longer contain physics/collision/render logic, only entity-id plumbing.

### Phase 2 — Player entity componentization

The `Player` class is large and gameplay-critical (feel matters a lot here), so it's its own phase, done *after* Phase 1's physics/collision/render are solid, since Player depends on all three.

- 2.1 Introduce `PlayerInput`, `PlayerPowerupState`, `PlayerMovementState` (walk/run/jump/duck/climb/swim-if-kept) components.
- 2.2 Extract input reading into `InputSystem` writing `PlayerInput`, decoupled from movement logic.
- 2.3 Extract the movement state machine into a `PlayerMovementSystem` operating on `PlayerMovementState` + `PhysicsBody` + `PlayerInput`. This is the riskiest single system for "feel" regressions — keep the golden-master harness running frame-perfect input sequences through jump-arc-critical moments (short hop vs. full jump, coyote time if present, etc.) and treat any deviation as a bug, not an acceptable side effect, unless the human explicitly signs off on a feel change.
- 2.4 Extract powerup logic (grow/shrink, fire/ice ability, invincibility timer) into `PlayerPowerupState` + a small `PowerupSystem`.
- 2.5 Delete the monolithic `Player` class's logic once nothing calls into it; keep only whatever thin "the player entity" bookkeeping the rest of the engine needs (e.g. "which entity is the local player" lookup).

Exit criteria: golden-master input-replay tests pass frame-identical for all recorded player trajectories; `Player.cpp` is gone or reduced to a tiny factory function that assembles the player archetype.

### Phase 3 — Data-driven entity/archetype loading

- 3.1 Design the archetype file format (see §2.4 and Open Decisions below) and write the archetype registry loader.
- 3.2 Change the level-file object factory: instead of `if (name == X) return new XClass()`, look up `name` in the archetype registry and spawn an entity with the archetype's components, then apply the level file's per-instance property overrides onto those components (this replaces each class's bespoke "parse my construction properties" code with a generic "set component field by name" mechanism — reflection via a small per-component field table, not runtime RTTI magic).
- 3.3 Port **non-badguy** `GameObject` subclasses first (blocks, coins, doors, platforms, triggers, decorations) since they're generally simpler than badguys and will validate the archetype/override mechanism before tackling AI. For each: write its archetype, spawn via the new path, delete the old class, check golden-master.
- 3.4 Confirm every remaining object in `data/` levels post-content-cuts has a corresponding archetype; anything unreferenced can be dropped from the codebase per the fork's stated cruft-cutting goal (flag these to the human rather than silently deleting content).

Exit criteria: no `GameObject` subclass remains for non-badguy, non-player, non-boss content; all such content is archetype + component data.

### Phase 4 — Badguy/AI migration (the big one)

This is where most of the "spaghetti" actually lives, so it gets the most structure:

- 4.1 Cluster the confirmed **59 badguy subclasses** by behavior similarity. `badguy/walking_badguy.hpp`/`.cpp` already exists as a shared intermediate base for at least some ground-patrol enemies — start there, since it's evidence of a natural cluster the original authors already half-recognized. A name-based starter guess (verify each against its actual `.cpp` before trusting it — names lie sometimes) to seed `docs/ecs-migration/badguy-clustering.md`:

  - **Ground walkers/patrollers** (likely built on or parallel to `walking_badguy`): `snowball`, `bouncing_snowball`, `mriceblock`, `spiky`, `sspiky`, `snail`, `crystallo`, `rcrystallo`, `scrystallo`, `viciousivy`, `walkingleaf`, `walking_candle`, `igel`, `toad`, `mrtree`, `stumpy`, `mole`
  - **Jumpers**: `jumpy`, `skullyhop`, `mrbomb` (bomb-drop variant of jump?), `kamikazesnowball`
  - **Flyers** (patterns/curves): `flyingsnowball`, `zeekling`, `spidermite`, `owl`, `captainsnowball`
  - **Stationary / turret / trap**: `angrystone`, `darttrap`, `dart`, `dispenser` (spawns other badguys — special-case, see below), `totem`, `stalactite`, `yeti_stalactite`, `crusher`
  - **Fire/ice elemental & status-effect-carrying**: `flame`, `iceflame`, `ghostflame`, `livefire`, `willowisp`, `ghoul`, `treewillowisp`, `kugelblitz`
  - **Explosive**: `bomb`, `mrbomb`, `goldbomb`, `short_fuse`, `haywire`
  - **Aquatic**: `fish_swimming`, `fish_jumping`, `fish_chasing`, `fish_harmless`
  - **Rolling/physical projectile-like**: `mole_rock`, `smartball`, `smartblock`
  - **Plants/scenery-enemy**: `plant`, `root`, `ghosttree` (likely a boss, verify), `snowman`
  - **Bosses / unique multi-phase** (candidates for the Phase 4.4 "named boss-script" exception — verify by size/complexity of the real `.cpp`, not just by name): `yeti`, `ghosttree`, `totem` (if it's the boss variant, not the stationary trap variant — check), `owl` (if boss)
  - **Structural/special-case, not really "AI" at all**: `dispenser` (a spawner of other archetypes — model as a `Spawner` component referencing archetype names, not as a behavior cluster of its own)

  This is a **starting hypothesis for triage order, not a final taxonomy** — Phase 4.1's actual deliverable is the verified version of this table after reading each `.cpp`.
- 4.2 For each cluster, design the minimal set of composable behavior components (§2.3) that covers every member, and implement the corresponding system(s).
- 4.3 Migrate cluster by cluster, smallest/simplest first (build confidence and reusable components before tackling bosses):
  - For each badguy: write its archetype using the cluster's components + custom data values, spawn it via Phase 3's archetype path, run golden-master, delete the C++ subclass.
  - Track a checklist (`docs/ecs-migration/badguy-checklist.md`) of every badguy: `[ ] migrated`, so an LLM resuming this work later has an explicit, resumable task list instead of re-deriving state from the diff.
- 4.4 Bosses last: give each boss a `StateMachine` component with data-defined states/transitions where possible; where a boss needs genuinely unique one-off code, isolate it as a small, named "boss script" function registered by name rather than a full subclass — this is an acceptable, explicitly-scoped exception to "no bespoke classes," not a backdoor to keep the old pattern everywhere.
- 4.5 Delete `BadGuy` base class once no subclass remains (its useful bits — e.g. shared "flat/dead" handling — should already have become a `Stompable`/`Health` system behavior in step 4.2).

Exit criteria: zero `BadGuy` subclasses remain (except explicitly-approved boss-script exceptions, tracked and justified); all enemies are archetype + component data; adding a new basic enemy requires writing a data file, not C++.

### Phase 5 — Scripting bridge rework

- 5.1 Replace per-class Squirrel bindings with the component-driven generic bindings described in §2.6.
- 5.2 Re-run every level's `init-script`/trigger scripts (via golden-master harness extended to execute scripts, not just physics) to confirm scripted behavior (doors opening, cutscene-lite triggers the fork kept, etc.) is unchanged.
- 5.3 Remove the old `ExposedObject<T>`-per-class machinery once unused.

### Phase 6 — Cleanup & hardening

- 6.1 Delete dead code: empty `GameObject`/`MovingObject`/`MovingSprite` shells if nothing still needs them as an entity-id wrapper; remove now-unused headers.
- 6.2 Re-run full golden-master suite + manual playtest pass on every remaining shipped level.
- 6.3 Performance pass: profile before/after (frame time, allocations); EnTT's cache-friendly iteration should generally *improve* perf over virtual-dispatch-per-object, but verify rather than assume, especially around the collision broad-phase.
- 6.4 Update `AGENTS.md`/contributor docs to describe the new architecture and the "how to add a new enemy" data-driven workflow, replacing any now-stale "subclass BadGuy" instructions.
- 6.5 Write a short migration retrospective (`docs/ecs-migration/retro.md`) noting any deliberately-approved gameplay/feel changes from §2–4, for the historical record.

---

## 3a. Seed checklist file

Create `docs/ecs-migration/badguy-checklist.md` at the start of Phase 4 with all 59 confirmed entries, e.g.:

```markdown
# Badguy migration checklist

- [ ] angrystone
- [ ] bomb
- [ ] bouncing_snowball
- [ ] captainsnowball
- [ ] crusher
- [ ] crystallo
- [ ] dart
- [ ] darttrap
- [ ] dispenser
- [ ] fish_chasing
- [ ] fish_harmless
- [ ] fish_jumping
- [ ] fish_swimming
- [ ] flame
- [ ] flyingsnowball
- [ ] ghostflame
- [ ] ghosttree
- [ ] ghoul
- [ ] goldbomb
- [ ] haywire
- [ ] iceflame
- [ ] igel
- [ ] jumpy
- [ ] kamikazesnowball
- [ ] kugelblitz
- [ ] livefire
- [ ] mole
- [ ] mole_rock
- [ ] mrbomb
- [ ] mriceblock
- [ ] mrtree
- [ ] owl
- [ ] plant
- [ ] rcrystallo
- [ ] root
- [ ] scrystallo
- [ ] short_fuse
- [ ] skullyhop
- [ ] skydive
- [ ] smartball
- [ ] smartblock
- [ ] snail
- [ ] snowball
- [ ] snowman
- [ ] spidermite
- [ ] spiky
- [ ] sspiky
- [ ] stalactite
- [ ] stumpy
- [ ] toad
- [ ] totem
- [ ] treewillowisp
- [ ] viciousivy
- [ ] walking_candle
- [ ] walkingleaf
- [ ] willowisp
- [ ] yeti
- [ ] yeti_stalactite
- [ ] zeekling
```

Also track the shared base `walking_badguy` itself (mark it "retire once all dependents migrate" rather than "migrate," since it should dissolve into the `Walker` component/system, not become an archetype itself).

## 4. Cross-cutting risk register

| Risk | Mitigation |
|---|---|
| Player "feel" regression (jump arc, acceleration curves) during Phase 2 | Frame-exact golden-master replays specifically covering jump timing edge cases; treat any diff as a defect by default |
| Save-game format tied to old class layout | Audit save/serialization code in Phase 0.1; if save data references object types by C++ class name, add a compatibility mapping table (old class name → new archetype name) rather than breaking old saves silently |
| Level `.stl` files reference object names the archetype registry doesn't yet cover mid-migration | Keep the legacy `if/else` factory as a fallback path *until* Phase 3/4 fully replace it for a given object; never let a level fail to load because of migration order |
| Scripting behavior drift (Phase 5) | Golden-master harness must execute scripts, not just physics, before Phase 5 is considered done |
| LLM agent losing track of a multi-week migration's state across sessions | Explicit checklists (`badguy-checklist.md`, phase exit criteria above) are the source of truth for "what's left," not conversation memory |
| Scope creep: re-adding editor/mod/translation generality "because ECS makes it easy" | Out of scope per the fork's stated goals; flag to human if a design decision seems to require it, don't just add it |
| Boss fights needing genuinely bespoke logic | Explicitly scoped exception in Phase 4.4 — named boss-script functions, tracked and justified, not a silent reversion to subclassing |
| Dead `editor_*`/`is_saveable`/`has_settings` virtual hooks on `GameObject` create confusion about whether editor support is coming back | Confirm with maintainer these are pure dead code (no known caller found in `src/`); if confirmed, delete them in Phase 1 as part of shrinking `GameObject`, rather than carrying them through the whole migration |
| `Dispenser` badguy spawns other badguys by name at runtime — a structural dependency on Phase 3's archetype registry, not just Phase 4's behavior components | Sequence `dispenser`'s migration to happen *after* Phase 3 lands the archetype registry, even though behaviorally it might look simple enough to do earlier |
| `UID`/`entt::entity` dual-handle confusion if bridging (§2.1) isn't done cleanly | Single written decision in Phase 0.6, referenced by every later phase; don't let individual bundles each invent their own bridging convention |
| ECS migration and the active multi-platform porting effort (§0.1a) are two large, concurrent initiatives touching overlapping files (`video/`, `audio/`, build system, platform-conditional headers) | Get explicit sequencing guidance from the maintainer (Open Decision #8) before Phase 1 touches `Sprite`/`Renderable`/rendering — don't let this plan's phases silently race with in-flight porting work on the same files |
| A component/system introduced in Phase 1+ compiles and passes golden-master on desktop Linux but silently breaks (or isn't even attempted) on Emscripten/Android/R36S | Phase 0.4's EnTT cross-platform build check, plus periodic (not per-task) cross-platform golden-master runs at phase boundaries per 0.3 |
| Git-bundle-based workflow (no PRs, no CI) means an LLM agent's work isn't automatically visible to reviewers between bundles | Keep bundles small and task-scoped per `AGENTS.md`'s own stated preference ("small, task-focused commits; one large bundle at the end of a work unit"), and keep the checklist files (badguy checklist, phase exit criteria) as the resumable source of truth, since there's no PR thread to reconstruct state from |

---

## 5. Open decisions for the human maintainer (don't guess on these — ask)

1. **Archetype file format:** reuse the existing `ReaderMapping`/Lisp-like s-expression parser (`util/reader*.hpp`) that `.stl` files already use (least new code, consistent with existing tooling) vs. adopt something like TOML/JSON (more tooling-friendly, easier for future non-C++ tools). Recommend the existing reader format for consistency unless there's a reason to standardize.
2. **How much boss bespoke-ness is acceptable:** where exactly to draw the "named boss-script function" line in Phase 4.4 — `yeti`, `ghosttree`, and possibly `totem`/`owl` are candidates; confirm which are actually bosses vs. just larger regular enemies once their `.cpp` files are read.
3. **Save-game compatibility requirement:** does this fork need to preserve saves across the migration, or is "wipe saves once" acceptable given it's a from-scratch-focused fork? (`supertux/savegame.hpp/.cpp` should be checked for whether it serializes by class name/type — if so, it needs the same old-name → new-archetype-name mapping treatment as level files.)
4. **Content scope:** Phase 3.4 will likely surface objects/badguys that exist in code but aren't used by any remaining level after the fork's content cuts (needs `data/` from Phase 0.1 to confirm) — confirm whether to delete these outright or archive them.
5. **Target timeline/parallelism:** whether phases must be strictly sequential (single agent/branch) or whether Phase 1's sub-steps (physics/collision/render/camera) can be parallelized across multiple agents/branches once the shadow-mode pattern is proven on the first one.
6. **Fate of `GameObjectComponent`** (`supertux/game_object_component.hpp`): retire it in favor of EnTT components as each subsystem migrates (recommended, see §2.2 note), or find a specific reason to keep it for some other purpose? Worth a quick grep for its actual current usages (only the interface was inspected here, not who instantiates it) before deciding.
7. **`port/emscripten.hpp`:** given §0.1a now confirms WASM is a real, actively-maintained target (not dead code), no further decision needed here — resolved. Retained for history.
8. **README vs. `AGENTS.md`/`CMakeLists.txt` scope conflict (§0.1a) — the big one.** Is the multi-platform porting effort (Linux/WASM/Android/R36S/Windows) the current active priority, with the README simply stale, or is the README the current intent and `AGENTS.md`/the CMake platform branches are the stale/aspirational side? This determines: (a) how much cross-platform build verification Phase 0 actually needs to do per-task vs. skip, (b) whether ECS Phase 1's rendering-component work needs to be coordinated with or sequenced after any in-flight porting work on `video/`, and (c) whether it's safe to treat "Linux golden-master passes" as sufficient proof of correctness for most phases, or whether WASM/Android/R36S parity is load-bearing throughout. Do not guess on this — it changes the cost of nearly every phase.
9. **Commit attribution convention.** `AGENTS.md` specifies commit author `Ingo Ruhnke <grumbel@gmail.com>` with a `Co-authored-by: Grok <grok@x.ai>` trailer — written, evidently, for a different AI agent's workflow. If Claude (or another agent) executes some or all of this plan, confirm with the maintainer whether that trailer convention still applies as-is, should be adjusted, or was specific to prior work and doesn't generalize.

---

## 6. How to hand this to an LLM agent

### 6a. Match the repo's actual delivery process, not a generic PR workflow

`AGENTS.md` specifies a concrete, non-default process that this plan's execution needs to follow rather than a generic "one PR per ticket" assumption:

- No GitHub CI, no PR review loop — work is delivered as **stacked git bundles**, sequentially numbered (`supertux-001-…`, `supertux-002-…`, never reusing a number), using `HEAD` as the ref.
- Prefer small, task-focused commits inside a bundle; **one bundle per logical unit of work** — which maps naturally onto this plan's numbered tasks (one bundle per `N.M` task, or per small cluster of closely-related tasks, e.g. one Phase-4 badguy cluster).
- Commit author and trailer conventions are specified explicitly in `AGENTS.md` — apply them as written unless the maintainer says otherwise for whichever agent is actually doing the work (see Open Decision #9).
- `PORTING.md`/`PORTS.md`/`TODO.md` are living documents the process expects to be kept current — any ECS-migration task that touches build files, platform-conditional code, or discovers a porting-relevant quirk (e.g. an R36S/Android/Emscripten build failure caused by a new component) should update `PORTING.md`, not just fix it silently, per `AGENTS.md`'s explicit working practice #1.
- Nix-specific hygiene called out in `AGENTS.md` applies to any Phase 0.4 EnTT vendoring work: the Nix store is read-only, so fix permissions after copying files out of it; double-check Nix string quoting/escaping in any flake changes.

### 6b. Recommended per-session prompt shape

Once this plan is checked into the repo (e.g. `docs/ecs-migration/plan.md`):

> "We're on Phase `<N>`, task `<N.M>` of `docs/ecs-migration/plan.md`. Read `docs/ecs-migration/audit.md`, `badguy-checklist.md`, and `AGENTS.md` for current state and process conventions. Implement task `<N.M>` only, run `tools/check.sh`, produce the next numbered git bundle per `AGENTS.md`'s conventions, and stop for review before starting the next task."

Keeping tasks this granular (roughly one bundle per numbered task above) is what makes this tractable for an LLM to "crunch on" without losing the thread across a rewrite this size, and it matches the repo's existing bundle-based review rhythm rather than fighting it.
