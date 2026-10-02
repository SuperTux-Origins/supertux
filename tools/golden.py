#!/usr/bin/env python3
# SuperTux golden-master regression harness
# Copyright (C) 2026 Ingo Ruhnke <grumbel@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

"""Golden-master regression harness.

Plays demos headless (--renderer null --fast-forward --dump-state) and
compares the per-frame object state against recorded baselines in
tests/golden/dumps/. Small positional drift is tolerated; see
docs/ecs-migration/kickoff.md for the policy.

  tools/golden.py record              re-record all baselines
  tools/golden.py check               run the suite, compare to baselines
  tools/golden.py compare A B         compare two dump files
  tools/golden.py demo PATTERN N OUT  write a synthetic demo file

Environment: SUPERTUX_BIN (default build/supertux-origins).
"""

import argparse
import concurrent.futures
import lzma
import os
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
GOLDEN_DIR = ROOT / "tests" / "golden"
SUITE_FILE = GOLDEN_DIR / "suite.txt"
DUMP_DIR = GOLDEN_DIR / "dumps"
DEMO_DIR = GOLDEN_DIR / "demos"

# Buttons in demo order: left right up down jump action
PATTERNS = {
    # Tux stands still; only objects near the spawn point get activated.
    "idle": lambda i: (0, 0, 0, 0, 0, 0),
    # Walk right, jump for 12 of every 40 frames.
    "hop": lambda i: (0, 1, 0, 0, 1 if i % 40 < 12 else 0, 0),
    # Run right, jump for 15 of every 50 frames.
    "run": lambda i: (0, 1, 0, 0, 1 if i % 50 < 15 else 0, 1),
}


def write_demo(pattern, frames, path, seed=1):
    func = PATTERNS[pattern]
    with open(path, "wb") as out:
        out.write(b"random_seed=%10d\0" % seed)
        for i in range(frames):
            out.write(bytes(func(i)))


def read_suite():
    entries = []
    for line in SUITE_FILE.read_text().splitlines():
        line = line.split("#", 1)[0].strip()
        if not line:
            continue
        fields = line.split()
        level, pattern, frames = fields[:3]
        spawn = fields[3] if len(fields) > 3 else None
        name = "{}-{}-{}".format(Path(level).parent.name, Path(level).stem, Path(pattern).stem)
        if spawn:
            name += "-at" + spawn.replace(",", "x")
        entries.append((name, level, pattern, int(frames), spawn))
    return entries


def run_one(binary, level, pattern, frames, spawn, dump_path, workdir):
    if pattern.endswith(".demo"):
        demo = DEMO_DIR / pattern
    else:
        demo = Path(workdir) / (Path(dump_path).name + ".demo")
        write_demo(pattern, frames, demo)
    userdir = Path(workdir) / (Path(dump_path).name + ".user")
    userdir.mkdir()
    env = dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy")
    cmd = [str(binary),
           "--userdir", str(userdir),
           "--datadir", str(ROOT / "data"),
           "--renderer", "null",
           "--disable-sound", "--disable-music",
           "--fast-forward",
           "--max-frames", str(frames + 60),  # safety net, demo-end comes first
           "--play-demo", str(demo),
           "--dump-state", str(dump_path)]
    if spawn:
        cmd += ["--spawn-pos", spawn]
    cmd.append(str(ROOT / level))
    proc = subprocess.run(cmd, env=env, cwd=workdir,
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                          timeout=600)
    if proc.returncode != 0:
        tail = proc.stdout.decode(errors="replace").splitlines()[-20:]
        raise RuntimeError("{} failed ({}):\n{}".format(level, proc.returncode, "\n".join(tail)))


def run_suite(binary, outdir, jobs, only=None):
    entries = [e for e in read_suite() if not only or any(o in e[0] for o in only)]
    results = {}
    with tempfile.TemporaryDirectory(prefix="golden-") as workdir, \
         concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
        futures = {}
        for name, level, pattern, frames, spawn in entries:
            dump = Path(workdir) / (name + ".dump")
            futures[pool.submit(run_one, binary, level, pattern, frames, spawn, dump, workdir)] = (name, dump)
        for fut in concurrent.futures.as_completed(futures):
            name, dump = futures[fut]
            fut.result()
            data = dump.read_bytes()
            results[name] = data
            if outdir:
                with lzma.open(Path(outdir) / (name + ".dump.xz"), "wb", preset=9) as out:
                    out.write(data)
    return results


def open_dump(path):
    path = Path(path)
    if path.suffix == ".xz":
        return lzma.open(path, "rt")
    return open(path)


def parse_dump(lines):
    """Yield (frame, state) per frame; state maps uid -> (type, values).
    The final item is ('end', reason, frame)."""
    state = {}
    frame = None
    for line in lines:
        line = line.rstrip("\n")
        if not line or line.startswith("#"):
            continue
        if line.startswith("frame "):
            if frame is not None:
                yield frame, state
            frame = int(line.split()[1])
        elif line.startswith("- "):
            state.pop(int(line.split()[1]), None)
        elif line.startswith("end "):
            if frame is not None:
                yield frame, state
                frame = None
            _, reason, count = line.split()
            yield "end", reason, int(count)
        else:
            parts = line.split()
            state[int(parts[0])] = (parts[1], tuple(float(v) for v in parts[2:]))
    if frame is not None:
        yield frame, state
        yield "end", "truncated", frame + 1


def compare(base_lines, new_lines, tolerance):
    """Return (verdict, report_lines). verdict: identical|close|diverged"""
    report = []
    base_iter = parse_dump(base_lines)
    new_iter = parse_dump(new_lines)

    first_divergence = None
    max_dev = (0.0, None, None, None)
    deviating = {}  # uid -> type
    membership_frames = 0
    any_diff = False
    base_end = new_end = None

    while True:
        b = next(base_iter, None)
        n = next(new_iter, None)
        if b is not None and b[0] == "end":
            base_end = b
        if n is not None and n[0] == "end":
            new_end = n
        if base_end or new_end or b is None or n is None:
            # drain to find end markers
            while base_end is None and (b := next(base_iter, None)) is not None:
                if b[0] == "end":
                    base_end = b
            while new_end is None and (n := next(new_iter, None)) is not None:
                if n[0] == "end":
                    new_end = n
            break

        frame, bstate = b
        _, nstate = n
        if bstate.keys() != nstate.keys():
            membership_frames += 1
            any_diff = True
        for uid in bstate.keys() & nstate.keys():
            bv = bstate[uid][1]
            nv = nstate[uid][1]
            if bv == nv:
                continue
            any_diff = True
            dev = max((abs(x - y) for x, y in zip(bv, nv)), default=0.0)
            if len(bv) != len(nv):
                dev = float("inf")
            if dev > max_dev[0]:
                max_dev = (dev, frame, uid, bstate[uid][0])
            if dev > tolerance:
                deviating[uid] = bstate[uid][0]
                if first_divergence is None:
                    first_divergence = frame

    end_differs = (base_end[1:] != new_end[1:]) if base_end and new_end else True
    if end_differs:
        any_diff = True
        report.append("run end differs: baseline {} at {}, new {} at {}".format(
            base_end[1] if base_end else "?", base_end[2] if base_end else "?",
            new_end[1] if new_end else "?", new_end[2] if new_end else "?"))
    if membership_frames:
        report.append("object set differs in {} frames (spawn/removal timing)".format(membership_frames))
    if max_dev[1] is not None:
        report.append("max deviation {:.3f} at frame {} (uid {} {})".format(*max_dev))
    if deviating:
        by_type = {}
        for t in deviating.values():
            by_type[t] = by_type.get(t, 0) + 1
        report.append("first frame over tolerance: {}".format(first_divergence))
        report.append("objects over tolerance: " + ", ".join(
            "{}x {}".format(c, t) for t, c in sorted(by_type.items(), key=lambda x: -x[1])))

    if not any_diff:
        return "identical", report
    if end_differs or deviating:
        return "diverged", report
    return "close", report


def binary_path(args):
    path = Path(args.binary or os.environ.get("SUPERTUX_BIN", ROOT / "build" / "supertux-origins"))
    if not path.exists():
        sys.exit("supertux binary not found: {} (set SUPERTUX_BIN or --binary)".format(path))
    return path.resolve()


def cmd_record(args):
    DUMP_DIR.mkdir(parents=True, exist_ok=True)
    results = run_suite(binary_path(args), DUMP_DIR, args.jobs, args.only)
    print("recorded {} baselines in {}".format(len(results), DUMP_DIR.relative_to(ROOT)))


def cmd_check(args):
    results = run_suite(binary_path(args), args.keep, args.jobs, args.only)
    counts = {"identical": 0, "close": 0, "diverged": 0, "missing": 0}
    for name in sorted(results):
        baseline = DUMP_DIR / (name + ".dump.xz")
        if not baseline.exists():
            counts["missing"] += 1
            print("MISSING   {}".format(name))
            continue
        with open_dump(baseline) as base:
            verdict, report = compare(base, results[name].decode().splitlines(), args.tolerance)
        counts[verdict] += 1
        if verdict != "identical" or args.verbose:
            print("{:9} {}".format(verdict.upper(), name))
            for line in report:
                print("            " + line)
    print("identical: {identical}  close: {close}  diverged: {diverged}  missing: {missing}".format(**counts))
    return 1 if counts["diverged"] or counts["missing"] else 0


def cmd_compare(args):
    with open_dump(args.a) as a, open_dump(args.b) as b:
        verdict, report = compare(a, b, args.tolerance)
    print(verdict.upper())
    for line in report:
        print("  " + line)
    return 1 if verdict == "diverged" else 0


def cmd_demo(args):
    write_demo(args.pattern, args.frames, args.output)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)

    for name in ("record", "check"):
        p = sub.add_parser(name)
        p.add_argument("--binary", help="supertux-origins executable")
        p.add_argument("-j", "--jobs", type=int, default=os.cpu_count())
        p.add_argument("--only", nargs="*", help="only run suite entries containing one of these strings")
        if name == "check":
            p.add_argument("--tolerance", type=float, default=1.0, help="max deviation in pixels (default 1.0)")
            p.add_argument("--keep", help="also write the new dumps to this directory")
            p.add_argument("-v", "--verbose", action="store_true")

    p = sub.add_parser("compare")
    p.add_argument("a")
    p.add_argument("b")
    p.add_argument("--tolerance", type=float, default=1.0)

    p = sub.add_parser("demo")
    p.add_argument("pattern", choices=sorted(PATTERNS))
    p.add_argument("frames", type=int)
    p.add_argument("output")

    args = parser.parse_args()
    return {"record": cmd_record, "check": cmd_check,
            "compare": cmd_compare, "demo": cmd_demo}[args.command](args) or 0


if __name__ == "__main__":
    sys.exit(main())
