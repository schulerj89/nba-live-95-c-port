"""Golden per-frame hashes for gameplay JSONL traces.

Once a scene passes ROM-versus-port lockstep, freeze the port trace as a
golden hash file. Re-verifying is then cheap (no Mesen run, no full trace in
git) and a later change that perturbs any tracked field fails with the first
divergent frame. Diagnose failures with compare_gameplay_traces.py.

Usage:
    python tools/trace_hash.py --trace port.jsonl --write-golden tipoff.golden.json
    python tools/trace_hash.py --trace port.jsonl --golden tipoff.golden.json
"""

import argparse
import hashlib
import json
from pathlib import Path

from compare_gameplay_traces import flatten, load_rows

# Volatile fields that legitimately differ between otherwise identical runs.
IGNORED_ROOTS = {"source", "frame", "simulation_tick", "routine_hits"}


def frame_hash(row):
    items = []
    for path, value in sorted(flatten(row)):
        if path.split(".", 1)[0] in IGNORED_ROOTS:
            continue
        items.append(f"{path}={value!r}")
    digest = hashlib.sha256("\n".join(items).encode("utf-8"))
    return digest.hexdigest()[:16]


def build_hashes(rows):
    return {str(frame): frame_hash(rows[frame]) for frame in sorted(rows)}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--trace", required=True)
    parser.add_argument("--write-golden", help="freeze the trace's hashes here")
    parser.add_argument("--golden", help="verify the trace against this file")
    parser.add_argument("--max-mismatches", type=int, default=10)
    args = parser.parse_args()
    if bool(args.write_golden) == bool(args.golden):
        parser.error("pass exactly one of --write-golden or --golden")

    hashes = build_hashes(load_rows(args.trace))

    if args.write_golden:
        overall = hashlib.sha256(
            json.dumps(hashes, sort_keys=True).encode("utf-8")).hexdigest()[:16]
        Path(args.write_golden).write_text(json.dumps(
            {"trace": Path(args.trace).name, "frame_count": len(hashes),
             "overall": overall, "hashes": hashes}, indent=2) + "\n")
        print(f"[TRACE HASH] WROTE: {args.write_golden} "
              f"frames={len(hashes)} overall={overall}")
        return

    golden = json.loads(Path(args.golden).read_text())
    golden_hashes = golden["hashes"]
    mismatched, missing, extra = [], [], []
    for frame in sorted(golden_hashes, key=int):
        if frame not in hashes:
            missing.append(frame)
        elif hashes[frame] != golden_hashes[frame]:
            mismatched.append(frame)
    for frame in sorted(hashes, key=int):
        if frame not in golden_hashes:
            extra.append(frame)

    status = "PASS" if not (mismatched or missing or extra) else "FAIL"
    print(f"[TRACE HASH] {status}: frames={len(golden_hashes)} "
          f"mismatched={len(mismatched)} missing={len(missing)} "
          f"extra={len(extra)}")
    for frame in mismatched[:args.max_mismatches]:
        print(f"  scene_frame {frame}: golden={golden_hashes[frame]} "
              f"trace={hashes[frame]}")
    if mismatched:
        print(f"  first divergence at scene_frame {mismatched[0]}; diagnose "
              "field-level differences with compare_gameplay_traces.py")
    if status == "FAIL":
        raise SystemExit(1)


if __name__ == "__main__":
    main()
