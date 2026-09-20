# ECS migration audit

Living document. Confirmed facts below came from inspecting the real `src/`
tree, `CMakeLists.txt`, and `AGENTS.md` directly (not from upstream-SuperTux
assumptions). Sections marked **TODO** are gaps Phase 0.1 still needs to fill
from the live repo.

## Confirmed: source layout

Top-level `src/`:
```
audio/  badguy/  collision/  control/  gui/  math/  object/  physfs/
port/   scripting/  sprite/  squirrel/  supertux/  trigger/  util/  video/  worldmap/
```

- No `editor/` directory — the in-game editor is gone, but `GameObject` still
  declares dead `editor_delete()`/`editor_select()`/`editor_deselect()`/
  `editor_update()`/`has_settings()`/`is_saveable()` virtuals with no live
  reflection system behind them (`get_settings()`/`ObjectSettings` do not
  exist anywhere in `src/`). Treat as dead code to remove in Phase 1, pending
  a maintainer nod (no caller found).
- `port/` contains only `emscripten.hpp` in the tarball, but the porting
  effort is real and much bigger than that one file — see "Platform scope"
  below.
- `gui/` is the in-game menu system, out of scope for this migration.

## Confirmed: object model

| Class | File | Notes |
|---|---|---|
| `GameObject` | `supertux/game_object.hpp` (186 lines) | Base for everything in a `Sector`. Already has `UID get_uid()`, and a rudimentary `GameObjectComponent` composition slot (`add_component`/`get_component<T>` via `dynamic_cast`) — not a real ECS, but prior art for the vocabulary. |
| `GameObjectManager` | `supertux/game_object_manager.hpp` (237 lines) | Owns `std::vector<std::unique_ptr<GameObject>>` plus `m_objects_by_uid`, `m_objects_by_name`, `m_objects_by_type_index` maps. This is the `Sector`'s object store and the natural home for a per-`Sector` `entt::registry`. |
| `MovingObject` | `supertux/moving_object.hpp` (113 lines) | Adds bounding box / collision participation. |
| `BadGuy` | `badguy/badguy.hpp` (286 lines) / `badguy/badguy.cpp` (932 lines) | Base for all 59 enemy subclasses (see `badguy-checklist.md`). Carries a real state machine (`STATE_INIT/INACTIVE/ACTIVE/SQUISHED/FALLING/BURNING/GEAR`) and shared physics/ignite/freeze/wind logic worth harvesting into components/systems rather than discarding. |
| `Player` | `object/player.hpp` (388 lines) / `object/player.cpp` (**2,603 lines**) | The single largest, riskiest file. Phase 2 target. |
| `Physic` | `supertux/physic.hpp` (89 lines) | Plain `{ax, ay, vx, vy, gravity_enabled_flag, gravity_modifier}` + `get_movement(dt)`. No virtuals. Near-direct copy into a `PhysicsBody` component. |
| `CollisionObject` | `collision/collision_object.hpp` (171 lines) | Already a discrete, non-polymorphic-in-the-relevant-sense member object. |
| `ObjectFactory` / `GameObjectFactory` | `supertux/object_factory.hpp` (92 lines) / `supertux/game_object_factory.hpp` (47 lines) | Single `std::map<std::string, FactoryFunction>` — name → lambda → `make_unique<ConcreteClass>(reader)`. Clean seam for Phase 3's archetype registry. |
| `UID` / `TypedUID<T>` | `util/uid.hpp` / `util/typed_uid.hpp` | Opaque handle resolved through `Sector::get().get_object_by_uid<T>()`. This already solves the "handle instead of raw pointer" problem an ECS migration normally introduces from scratch — bridge to `entt::entity` rather than replace wholesale. Has a documented NDK-libc++-r26 `std::format` workaround (evidence Android is a live build target). |
| `ExposedObject<S,T>` | `squirrel/exposed_object.hpp` | Squirrel bindings already store a `UID` (`m_parent->get_uid()`), not a raw pointer. ~71 files in `scripting/`, one per exposed class — Phase 5's de-duplication target. |

**Counts:** 59 concrete `BadGuy` subclasses (full list in `badguy-checklist.md`)
+ 1 shared intermediate base (`walking_badguy`). `object/` (non-badguy game
objects) has 100+ classes — blocks, platforms, particle systems, triggers,
decorations, lights, path-following helpers, etc.

## Confirmed: build system

- C++20 required (`CMakeLists.txt`), no GNU extensions.
- Depends on `tinycmmc` (`find_package(tinycmmc CONFIG)`); project is moving
  away from some legacy Boost usage toward it, per `AGENTS.md`.
- Version sourced from top-level `VERSION` file; Nix flake appends dev suffix.
- Platform branches in `CMakeLists.txt` are extensive: `EMSCRIPTEN` (own
  SDL2_image stub, `FULL_ES2`, explicit `EXPORTED_FUNCTIONS` JS interop list),
  `ANDROID` (forces GLES2), `SUPERTUX_R36S` (own ABI-shim TU for GCC-15-vs-old-
  sysroot), `WIN32`/`MINGW` (own linking section). This is a real, in-progress
  five-platform effort, not aspirational.

## CONFLICT requiring maintainer resolution (see plan Open Decision #8)

Public README says the fork's goal includes "abandonment of most ports,"
focus on Linux/NixOS. `AGENTS.md` + `CMakeLists.txt` describe and implement an
active five-platform port (Linux/OpenGL 3.3, WebAssembly/WebGL, Android/GLES2,
R36S-ArkOS/GLES2, Windows/MinGW), modeled on Pingus/Windstille packaging.
**Not yet resolved.** This determines how much cross-platform build
verification the rest of this migration actually needs per-task.

## TODO — fill from the live repo (not in the tarball provided so far)

- [ ] `data/` — or at minimum a `data/levels` listing, to know which
      archetypes/badguys are actually referenced by shipped levels
      post-content-cuts (needed for Phase 3.4 and to prioritize Phase 4
      clustering order).
- [ ] `tests/` — any existing tests to preserve/build on.
- [ ] `TODO.md` — current open tasks; check for overlap with this migration.
- [ ] `PORTING.md` — platform-specific quirks and lessons learned; `AGENTS.md`
      says read this before inventing new solutions, so read it before Phase
      0.4's EnTT cross-platform verification.
- [ ] `PORTS.md` — packaging map / flake outputs.
- [ ] `VERSION` — current version string.
- [ ] `nix/`, `mk/` — build recipe details for the five platforms, needed to
      actually run Phase 0.2's build-baseline task rather than guess at it.
- [ ] Confirm current usages (if any) of `GameObjectComponent` beyond the bare
      interface — only the interface itself was inspected so far (plan Open
      Decision #6).
- [ ] Confirm whether `savegame.hpp`/`.cpp` serializes by class name/type
      (affects save-compatibility risk, plan Open Decision #3).
- [ ] Read `AGENTS.md`'s referenced Pingus/Windstille `mk/`/`nix/` patterns
      before Phase 0.4 invents new EnTT build wiring.
