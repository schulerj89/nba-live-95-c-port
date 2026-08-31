"""Run bounded, test-only live scenarios through the production game caller.

This helper never patches the executable, ROM, assets, or trace.  It starts at
the existing headless Tipoff test entry and records that direct-entry boundary
in its report.  Scenarios without an honest production route are refused.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SUPPORTED = ("cpu-pass", "clock-expiry-q1", "pause-resume")
REFUSED = {
    "overtime": "No CLI period/score seed reaches overtime through the production lifecycle.",
    "free-throw": "No deterministic live foul-to-free-throw route exists; current probes seed typed component state.",
    "substitution": "No live foul-out/substitution route exists; the current runtime probe mutates initialized state.",
    "human-control": "Tipoff initialization deliberately installs the neutral CPU allocation pending complete human actions.",
}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def git_text(*args: str) -> str:
    run = subprocess.run(
        ["git", *args], cwd=ROOT, text=True, capture_output=True, check=False
    )
    return run.stdout.strip() if run.returncode == 0 else "unavailable"


def read_rows(path: Path) -> list[dict]:
    with path.open("r", encoding="utf-8") as source:
        return [json.loads(line) for line in source if line.strip()]


def require(condition: bool, detail: str) -> None:
    if not condition:
        raise RuntimeError(detail)


def pass_observations(rows: list[dict]) -> dict:
    start_index = next(
        (
            index
            for index, row in enumerate(rows)
            if row["possession"]["pass_actor_raw"] >= 0
            and row["possession"]["pass_receiver_raw"] >= 0
            and row["possession"]["pass_active_raw"] != 0
        ),
        None,
    )
    require(start_index is not None, "no pass began within the bounded run")
    start = rows[start_index]
    passer = start["possession"]["pass_actor_raw"]
    receiver = start["possession"]["pass_receiver_raw"]
    release_index = next(
        (
            index
            for index in range(start_index + 1, len(rows))
            if rows[index]["possession"]["pass_actor_raw"] == passer
            and rows[index]["possession"]["pass_receiver_raw"] == receiver
            and rows[index]["possession"]["pass_active_raw"] == 0
            and rows[index]["possession"]["actor"] == -1
            and rows[index]["ball"]["state"] == 3
        ),
        None,
    )
    require(release_index is not None, "pass did not release within the bounded run")
    catch_index = next(
        (
            index
            for index in range(release_index + 1, len(rows))
            if rows[index]["possession"]["actor"] == receiver
            and rows[index]["ball"]["owner"] == receiver
            and rows[index]["ball"]["state"] == 4
            and rows[index]["possession"]["pass_actor_raw"] == -1
            and rows[index]["possession"]["pass_receiver_raw"] == -1
        ),
        None,
    )
    require(catch_index is not None, "receiver did not acquire the pass within the bounded run")
    return {
        "passer": passer,
        "receiver": receiver,
        "begin_frame": start["frame"],
        "release_frame": rows[release_index]["frame"],
        "catch_frame": rows[catch_index]["frame"],
    }


def clock_observations(rows: list[dict]) -> dict:
    expiry_index = next(
        (
            index
            for index, row in enumerate(rows)
            if row["match"]["period_raw_0926"] == 1
            and row["match"]["match_clock_raw_0928"] == 0
        ),
        None,
    )
    require(expiry_index is not None, "Q1 did not expire within the bounded run")
    restart_index = next(
        (
            index
            for index in range(expiry_index + 1, len(rows))
            if rows[index]["match"]["period_raw_0926"] == 1
            and rows[index]["match"]["match_clock_raw_0928"] > 0
            and rows[index]["match"]["live_state_raw"] == 0x82
        ),
        None,
    )
    require(restart_index is not None, "Q2 restart was not published within the bounded run")
    restart_clock = rows[restart_index]["match"]["match_clock_raw_0928"]
    live_index = next(
        (
            index
            for index in range(restart_index + 1, len(rows))
            if rows[index]["match"]["period_raw_0926"] == 1
            and rows[index]["match"]["match_clock_raw_0928"] < restart_clock
            and rows[index]["match"]["live_state_raw"] < 0x80
            and rows[index]["possession"]["actor"] >= 0
        ),
        None,
    )
    require(live_index is not None, "Q2 did not return to live possession within the bounded run")
    return {
        "expiry_frame": rows[expiry_index]["frame"],
        "restart_frame": rows[restart_index]["frame"],
        "restart_clock": restart_clock,
        "live_frame": rows[live_index]["frame"],
    }


def pause_observations(rows: list[dict]) -> dict:
    require(len(rows) == 330, "pause scenario trace row count changed")
    before, entered, frozen, resumed = rows[249], rows[250], rows[314], rows[315]
    require(before["simulation_tick"] == 250, "unexpected pre-pause simulation tick")
    require(entered["input"]["pressed"] == 8, "Start was not observed at the outer caller")
    require(entered["match"]["live_state_raw"] == 0x80, "Start did not enter pause")
    require(
        frozen["simulation_tick"] == before["simulation_tick"]
        and frozen["match"]["match_clock_raw_0928"]
        == before["match"]["match_clock_raw_0928"],
        "pause did not freeze gameplay and clock",
    )
    require(
        resumed["simulation_tick"] == before["simulation_tick"] + 1
        and resumed["match"]["match_clock_raw_0928"]
        == before["match"]["match_clock_raw_0928"] - 1
        and resumed["match"]["live_state_raw"] != 0x80,
        "pause did not restore the production update",
    )
    return {
        "start_frame": entered["frame"],
        "frozen_simulation_tick": entered["simulation_tick"],
        "frozen_clock": entered["match"]["match_clock_raw_0928"],
        "resume_frame": resumed["frame"],
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--scenario", required=True, choices=SUPPORTED + tuple(REFUSED))
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--rom", required=True, type=Path)
    parser.add_argument("--pack", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()

    if os.environ.get("NBA95_PRODUCTION_BUILD"):
        parser.error("quick_live_scenario.py refuses production-build execution")
    if args.scenario in REFUSED:
        parser.error(f"{args.scenario} is intentionally refused: {REFUSED[args.scenario]}")
    manifest = ROOT / "nba95_sources.txt"
    if manifest.exists() and "quick_live_scenario" in manifest.read_text(encoding="utf-8"):
        parser.error("test-only scenario helper must not be present in nba95_sources.txt")

    exe, rom, pack = (args.exe.resolve(), args.rom.resolve(), args.pack.resolve())
    for label, path in (("executable", exe), ("ROM", rom), ("asset pack", pack)):
        if not path.is_file():
            parser.error(f"{label} does not exist: {path}")
    output = args.output.resolve()
    if output.exists():
        parser.error(f"output must be new so stale evidence cannot mix: {output}")
    output.mkdir(parents=True)
    trace = output / "gameplay.jsonl"
    frames = {"cpu-pass": 390, "clock-expiry-q1": 2300, "pause-resume": 330}[args.scenario]
    command = [
        str(exe), "--headless", "--rom", str(rom), "--assets", str(pack),
        "--tipoff-only", "--frames", str(frames), "--gameplay-trace", str(trace),
    ]
    limitation = "Direct Tipoff test entry; production NbaGame input/tick/update callers."
    controlled_seed = None
    if args.scenario == "clock-expiry-q1":
        command[command.index("--frames"):command.index("--frames")] = ["--tipoff-clock", "2"]
        controlled_seed = {"match_clock_raw_0928": 2}
        limitation += " Clock=2 is an explicit C test seed, not a natural/native timing witness."
    elif args.scenario == "pause-resume":
        script = output / "input.txt"
        script.write_text(
            "250 0000\n1 1000\n1 0000\n1 0400\n1 0000\n1 0080\n70 0000\n",
            encoding="ascii",
        )
        command.extend(("--input-script", str(script)))
        controlled_seed = {"native_input_words": "Start, release, Down, release, A"}

    environment = {key: value for key, value in os.environ.items() if not key.startswith("NBA95")}
    run = subprocess.run(command, capture_output=True, env=environment)
    (output / "stdout.txt").write_bytes(run.stdout)
    (output / "stderr.txt").write_bytes(run.stderr)
    require(run.returncode == 0, f"scenario executable failed with exit {run.returncode}")
    rows = read_rows(trace)
    require(len(rows) == frames, f"expected {frames} telemetry rows, got {len(rows)}")
    if args.scenario == "cpu-pass":
        observations = pass_observations(rows)
    elif args.scenario == "clock-expiry-q1":
        observations = clock_observations(rows)
    else:
        observations = pause_observations(rows)

    report = {
        "schema": 1,
        "passed": True,
        "scenario": args.scenario,
        "test_only": True,
        "native_evidence": False,
        "normal_runtime_seeded": False,
        "production_manifest_member": False,
        "scope": limitation,
        "controlled_seed": controlled_seed,
        "source_commit": git_text("rev-parse", "HEAD"),
        "source_dirty_status": git_text("status", "--short"),
        "command": command,
        "identities": {
            "executable": {"path": str(exe), "sha256": sha256(exe)},
            "rom": {"path": str(rom), "sha256": sha256(rom)},
            "asset_pack": {"path": str(pack), "sha256": sha256(pack)},
            "trace": {"rows": len(rows), "sha256": sha256(trace)},
        },
        "observations": observations,
    }
    report_path = output / "report.json"
    report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"QUICK LIVE SCENARIO PASS: {args.scenario} -> {report_path}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as error:
        print(f"QUICK LIVE SCENARIO FAIL: {error}", file=sys.stderr)
        raise SystemExit(1)
