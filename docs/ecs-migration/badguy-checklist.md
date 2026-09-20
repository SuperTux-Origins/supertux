# Badguy migration checklist

Source of truth for Phase 4 of `docs/ecs-migration/plan.md`. Check an item once
its C++ subclass is deleted and its behavior is fully expressed as an archetype
+ components, verified against the golden-master replay harness (see plan
§Phase 0.3).

Starter clustering (from plan §4.1) — **verify each against its real `.cpp`
before trusting the cluster**, names are a triage hint, not ground truth:

## Ground walkers / patrollers
- [ ] snowball
- [ ] bouncing_snowball
- [ ] mriceblock
- [ ] spiky
- [ ] sspiky
- [ ] snail
- [ ] crystallo
- [ ] rcrystallo
- [ ] scrystallo
- [ ] viciousivy
- [ ] walkingleaf
- [ ] walking_candle
- [ ] igel
- [ ] toad
- [ ] mrtree
- [ ] stumpy
- [ ] mole

## Jumpers
- [ ] jumpy
- [ ] skullyhop
- [ ] mrbomb
- [ ] kamikazesnowball

## Flyers
- [ ] flyingsnowball
- [ ] zeekling
- [ ] spidermite
- [ ] owl
- [ ] captainsnowball

## Stationary / turret / trap
- [ ] angrystone
- [ ] darttrap
- [ ] dart
- [ ] dispenser  — spawns other archetypes at runtime; sequence AFTER Phase 3's archetype registry lands, not earlier
- [ ] totem
- [ ] stalactite
- [ ] yeti_stalactite
- [ ] crusher

## Fire/ice elemental & status-effect-carrying
- [ ] flame
- [ ] iceflame
- [ ] ghostflame
- [ ] livefire
- [ ] willowisp
- [ ] ghoul
- [ ] treewillowisp
- [ ] kugelblitz

## Explosive
- [ ] bomb
- [ ] goldbomb
- [ ] short_fuse
- [ ] haywire

## Aquatic
- [ ] fish_swimming
- [ ] fish_jumping
- [ ] fish_chasing
- [ ] fish_harmless

## Rolling / physical projectile-like
- [ ] mole_rock
- [ ] smartball
- [ ] smartblock

## Plants / scenery-enemy
- [ ] plant
- [ ] root
- [ ] snowman

## Bosses / unique multi-phase (candidate exceptions per plan §4.4 — confirm each is actually boss-scale before treating it as an exception)
- [ ] yeti
- [ ] ghosttree
- [ ] totem — ONLY if this is a distinct boss variant, not the stationary-trap `totem` above; check before double-counting
- [ ] owl — ONLY if this is a distinct boss variant, not the flyer `owl` above; check before double-counting

## Shared base to retire (not an archetype itself)
- [ ] walking_badguy — dissolve into the `Walker` component/system once all dependents above are migrated; do not port this class into an archetype of its own

---

**Total tracked: 59 concrete subclasses + 1 shared base (`walking_badguy`).**
If Phase 0.1's `data/` audit finds any of these unused by any shipped level,
flag to the maintainer per plan Open Decision #4 (delete vs. archive) rather
than migrating dead content.
