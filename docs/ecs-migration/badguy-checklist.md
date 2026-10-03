# Badguy migration checklist

Source of truth for Phase 4 of `docs/ecs-migration/plan.md`. Check an item once
its C++ subclass is deleted and its behavior is fully expressed as an archetype
+ components, verified against the golden-master replay harness (see plan
§Phase 0.3).

Starter clustering (from plan §4.1) — **verify each against its real `.cpp`
before trusting the cluster**, names are a triage hint, not ground truth:

## Ground walkers / patrollers
- [x] snowball — `data/archetypes/snowball.archetype`
- [x] bouncing_snowball — `data/archetypes/bouncingsnowball.archetype`
- [x] mriceblock — `data/archetypes/mriceblock.archetype`
- [x] spiky — `data/archetypes/spiky.archetype`
- [x] sspiky — `data/archetypes/sspiky.archetype`
- [x] snail — `data/archetypes/snail.archetype`
- [x] crystallo — `data/archetypes/crystallo.archetype` (shatter reaction not covered by golden suite yet)
- [ ] rcrystallo
- [ ] scrystallo
- [x] viciousivy (+ alias poisonivy) — `data/archetypes/viciousivy.archetype`
- [x] walkingleaf — `data/archetypes/walkingleaf.archetype`
- [ ] walking_candle
- [x] igel — `data/archetypes/igel.archetype`
- [ ] toad
- [x] mrtree — `data/archetypes/mrtree.archetype`
- [x] stumpy — `data/archetypes/stumpy.archetype`
- [ ] mole

## Jumpers
- [x] jumpy — `data/archetypes/jumpy.archetype`
- [x] skullyhop — `data/archetypes/skullyhop.archetype`
- [x] mrbomb — `data/archetypes/mrbomb.archetype`
- [ ] kamikazesnowball

## Flyers
- [x] flyingsnowball — `data/archetypes/flyingsnowball.archetype`
- [x] zeekling — `data/archetypes/zeekling.archetype`
- [x] spidermite — `data/archetypes/spidermite.archetype`
- [ ] owl
- [x] captainsnowball — `data/archetypes/captainsnowball.archetype`

## Stationary / turret / trap
- [ ] angrystone
- [x] darttrap — `data/archetypes/darttrap.archetype`
- [x] dart — `data/archetypes/dart.archetype`
- [ ] dispenser  — spawns other archetypes at runtime; sequence AFTER Phase 3's archetype registry lands, not earlier
- [ ] totem
- [x] stalactite — `data/archetypes/stalactite.archetype`
- [~] yeti_stalactite — shell subclass of ArchetypeBadguy with the Stalactite behavior (yeti needs is_hanging/start_shaking)
- [ ] crusher

## Fire/ice elemental & status-effect-carrying
- [x] flame — `data/archetypes/flame.archetype`
- [x] iceflame — `data/archetypes/iceflame.archetype` (not exercised by golden suite)
- [x] ghostflame — `data/archetypes/ghostflame.archetype`
- [ ] livefire
- [ ] willowisp
- [ ] ghoul
- [ ] treewillowisp
- [ ] kugelblitz

## Explosive
- [x] bomb — `data/archetypes/bomb.archetype`
- [ ] goldbomb
- [x] short_fuse — `data/archetypes/short_fuse.archetype` (lightly covered by golden suite: 2 runs; the earlier failed targeted probes were caused by stale-binary/new-data mismatch, worth retrying)
- [ ] haywire

## Aquatic
- [ ] fish_swimming
- [x] fish_jumping — `data/archetypes/fish-jumping.archetype`
- [ ] fish_chasing
- [ ] fish_harmless

## Rolling / physical projectile-like
- [ ] mole_rock
- [x] smartball — `data/archetypes/smartball.archetype`
- [x] smartblock — `data/archetypes/smartblock.archetype`

## Plants / scenery-enemy
- [ ] plant
- [ ] root
- [x] snowman — `data/archetypes/snowman.archetype`

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
