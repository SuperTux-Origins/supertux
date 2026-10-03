# Golden-master regression suite

Plays every entry of `suite.txt` headless and compares the per-frame state
of all `MovingObject`s against the baselines in `dumps/`.

```bash
export SUPERTUX_BIN=build/supertux-origins
tools/golden.py check            # compare against baselines (~30 s)
tools/golden.py check --only world1-welcome --keep /tmp/new
tools/golden.py compare dumps/a.dump.xz /tmp/new/a.dump.xz
tools/golden.py record           # re-baseline after an accepted change
```

Verdicts per run:

- **IDENTICAL**: byte-identical state.
- **CLOSE**: differences, but every object stays within `--tolerance`
  pixels (default 1.0) and the run ends the same way.
- **DIVERGED**: an object drifted beyond tolerance, or the run ended
  differently (death/finish/frame). Review it; if the change is intended,
  re-record. Small gameplay differences are acceptable during the ECS
  migration (see `docs/ecs-migration/kickoff.md`), but they must be
  reviewed, not ignored.

## How a run works

`supertux-origins --renderer null --fast-forward --play-demo DEMO
--dump-state DUMP LEVEL` runs one logical step per loop iteration, skips
the level intro, and quits at the first death, at the level end, when the
demo input runs out, or at `--max-frames`. The dump is delta-encoded:
`frame N`, then `UID TYPE X Y W H [VX VY] g:GROUP [a:ACTION]` for changed
objects (collision group and sprite action compare exactly) and
`- UID` for removed ones, then `end REASON FRAMES`.

## Inputs

Synthetic patterns (`idle`, `hop`, `run`) only cover the start of a
level: `hop` runs die after about 7 s. For deeper coverage, record real
demos and add them to `suite.txt`:

```bash
supertux-origins --record-demo tests/golden/demos/NAME.demo data/levels/world1/LEVEL.stlv
```

Play without dying: on death the recorder restarts the demo file with
a new seed.

## Recording baselines for a migration

Baselines must come from the code *before* a change. Record targeted
runs with the old build before migrating, or build the old commit in a
git worktree and record there. Always run a binary with the `data/` of
its own commit: an older binary rejects newer `data/archetypes` (unknown
components), which breaks level loading and looks like Tux dying at
frame 201.
