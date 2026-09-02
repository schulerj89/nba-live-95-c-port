"""Capture the visible-defect checkpoint matrix from one production build.

The executable is never patched. This test-only orchestrator uses the port's
existing production scene callers, native input words, renderer, and gameplay
telemetry. It exports a compact visual-review gallery while temporary complete
frame sequences are discarded after the selected checkpoints are copied.
"""

from __future__ import annotations

import argparse
import gzip
import hashlib
import html
import json
import os
import shutil
import struct
import subprocess
import sys
import tempfile
import time
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path

from PIL import Image, ImageDraw


ROOT = Path(__file__).resolve().parents[1]
PROGRESS_PARTS = (".analysis", "progress-screenshots")
ROM_SHA256 = "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"
PACK_VERSION = 31
PLAYER_DRAW_RESOURCE = 287

FRONTEND_INPUT = """\
200 0000
1 0200
1 0000
1 1000
200 0000
1 0200
1 0000
1 1000
180 0000
1 1000
1 0000
1 1000
1 0000
1 0100
1 0000
1 0200
1 0000
1 1000
20 0000
"""

LINEUP_INPUT = """\
200 0000
1 1000
798 0000
1 0100
9 0000
1 0100
9 0000
1 0100
9 0000
1 0100
9 0000
1 0100
9 0000
1 0100
9 0000
1 0100
9 0000
1 0100
9 0000
1 0100
9 0000
"""


@dataclass(frozen=True)
class FrameRef:
    path: Path
    label: str
    group: str
    frame: int | None = None


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def git_text(*args: str) -> str:
    run = subprocess.run(
        ["git", *args], cwd=ROOT, capture_output=True, text=True, check=False
    )
    return run.stdout.strip() if run.returncode == 0 else "unavailable"


def clean_environment() -> dict[str, str]:
    return {key: value for key, value in os.environ.items()
            if not key.startswith("NBA95")}


def require(condition: bool, detail: str) -> None:
    if not condition:
        raise RuntimeError(detail)


def require_progress_output(path: Path) -> None:
    lowered = [part.lower() for part in path.resolve().parts]
    allowed = any(
        lowered[index:index + 2] == list(PROGRESS_PARTS)
        for index in range(len(lowered) - 1)
    )
    if not allowed:
        raise RuntimeError(
            "--output must be inside an ignored .analysis/progress-screenshots tree"
        )


def inspect_pack(path: Path) -> dict:
    raw = path.read_bytes()
    require(raw[:8] == b"NBA95PAK", "asset pack magic is not NBA95PAK")
    version, count = struct.unpack_from("<II", raw, 8)
    require(version == PACK_VERSION, f"expected asset pack v{PACK_VERSION}, got v{version}")
    require(16 + count * 24 <= len(raw), "asset pack directory is truncated")
    selected = None
    for index in range(count):
        ident, offset, size, width, height, flags = struct.unpack_from(
            "<6I", raw, 16 + index * 24
        )
        require(offset + size <= len(raw), f"asset {ident} exceeds pack bounds")
        if ident == PLAYER_DRAW_RESOURCE:
            payload = raw[offset:offset + size]
            selected = {
                "id": ident,
                "size": size,
                "width": width,
                "height": height,
                "flags": flags,
                "magic": payload[:8].decode("ascii", errors="replace"),
                "sha256": hashlib.sha256(payload).hexdigest(),
            }
    require(selected is not None, "player-draw resource 287 is absent")
    require(selected["magic"] == "NBPDRAW1", "resource 287 is not NBPDRAW1")
    return {"version": version, "resource_count": count,
            "player_draw_resource": selected}


class Harness:
    def __init__(self, exe: Path, rom: Path, pack: Path, output: Path) -> None:
        self.exe = exe.resolve()
        self.rom = rom.resolve()
        self.pack = pack.resolve()
        self.output = output.resolve()
        self.logs = self.output / "logs"
        self.frames = self.output / "frames"
        self.commands: list[dict] = []
        self.checks: dict[str, object] = {}
        self.contacts: list[FrameRef] = []
        self.started = time.monotonic()

    def command(self, name: str, argv: list[object], required: tuple[str, ...] = ()) -> str:
        command = [str(item) for item in argv]
        started = time.monotonic()
        flags = subprocess.CREATE_NO_WINDOW if os.name == "nt" else 0
        run = subprocess.run(
            command, cwd=ROOT, capture_output=True, env=clean_environment(),
            creationflags=flags, check=False
        )
        stdout_path = self.logs / f"{name}.stdout.txt"
        stderr_path = self.logs / f"{name}.stderr.txt"
        stdout_path.write_bytes(run.stdout)
        stderr_path.write_bytes(run.stderr)
        stdout = run.stdout.decode("utf-8", errors="replace")
        stderr = run.stderr.decode("utf-8", errors="replace")
        self.commands.append({
            "name": name,
            "argv": command,
            "exit_code": run.returncode,
            "elapsed_seconds": round(time.monotonic() - started, 3),
            "stdout": str(stdout_path.relative_to(self.output)).replace("\\", "/"),
            "stderr": str(stderr_path.relative_to(self.output)).replace("\\", "/"),
        })
        require(run.returncode == 0,
                f"{name} failed with exit {run.returncode}:\n{stdout}\n{stderr}")
        for text in required:
            require(text in stdout, f"{name} did not report required state: {text}")
        return stdout

    def exe_command(self, name: str, extra: list[object],
                    required: tuple[str, ...] = ()) -> str:
        return self.command(name, [
            self.exe, "--headless", "--rom", self.rom,
            "--assets", self.pack, *extra,
        ], required)

    def export_frames(self, raw: Path, group: str,
                      selected: list[int]) -> dict[int, FrameRef]:
        destination = self.frames / group
        destination.mkdir(parents=True, exist_ok=True)
        result = {}
        for frame in selected:
            source = raw / f"frame_{frame:04d}.bmp"
            require(source.is_file(), f"missing rendered {group} frame {frame}")
            target = destination / f"frame-{frame:04d}.png"
            with Image.open(source) as image:
                image.convert("RGB").save(target)
            result[frame] = FrameRef(target, f"{group} f{frame}", group, frame)
        return result

    def capture_frontend(self, temporary: Path) -> dict[int, FrameRef]:
        setup_raw = temporary / "setup-to-team-select"
        setup_raw.mkdir()
        self.exe_command(
            "setup-to-team-select-sequence", [
                "--setup-only", "--setup-main-row", 0, "--setup-main-confirm",
                "--frames", 300, "--dump-sequence-from", 160,
                "--dump-sequence-dir", setup_raw, "--debug-state",
            ], ("route=TEAM_SELECTION", "SCN:TEAM_SELECT",
                "Wrote 141 rendered sequence frames"),
        )
        setup_frames = [164, 166, 184, 185, 186, 193, 200, 205,
                        213, 214, 215, 277, 281, 282, 300]
        setup_refs = self.export_frames(
            setup_raw, "setup-to-team-select", setup_frames
        )
        setup_nonblack = {}
        for frame in (205, 213, 214, 277, 281, 282):
            with Image.open(setup_refs[frame].path) as image:
                setup_nonblack[frame] = sum(
                    pixel != (0, 0, 0) for pixel in image.convert("RGB").getdata()
                )
        require(setup_nonblack[205] > 0 and setup_nonblack[213] == 0 and
                setup_nonblack[214] == 0,
                "Setup artwork did not withdraw cleanly into forced black")
        require(setup_nonblack[277] == 0 and setup_nonblack[281] > 0 and
                setup_nonblack[282] > 0,
                "Team Select construction reveal boundary changed")
        self.make_contact(
            "setup-to-team-select-continuous-contact.png",
            [setup_refs[frame] for frame in setup_frames],
            "Continuous Game Setup exit and Team Select construction handoff",
            columns=5,
        )

        script = self.output / "frontend-neutral-and-skips.input"
        script.write_text(FRONTEND_INPUT, encoding="ascii")
        raw = temporary / "frontend"
        raw.mkdir()
        base = ["--team-only", "--input-script", script]
        self.exe_command(
            "frontend-neutral-probe", [*base, "--frames", 405, "--debug-state"],
            ("[PLAYER SETUP TEST] p1=NEUTRAL", "SCN:PLAYER_SETUP"),
        )
        self.exe_command(
            "frontend-sequence", [
                *base, "--frames", 620, "--dump-sequence-from", 1,
                "--dump-sequence-dir", raw, "--debug-state",
            ], ("SCN:TIPOFF", "Wrote 620 rendered sequence frames"),
        )
        frames = sorted(set(
            [1, 5, 10, 15, 20, 30, 40, 60, 80, 100, 120, 140, 160, 180, 200, 203]
            + [204, 210, 220, 228, 229, 230, 240, 250, 253, 254, 280, 298, 299,
               300, 319, 320, 321, 322, 350, 400, 403, 404, 405, 406, 500, 580, 586,
               587, 588, 589, 590, 591, 593, 594, 595, 596, 600, 620]
        ))
        refs = self.export_frames(raw, "frontend", frames)
        nonblack = {}
        for frame in (253, 254, 320, 321):
            with Image.open(refs[frame].path) as image:
                nonblack[frame] = sum(
                    pixel != (0, 0, 0) for pixel in image.convert("RGB").getdata()
                )
        require(nonblack[253] > 0 and nonblack[254] == 0,
                "full route did not enter forced black at frame 254")
        require(nonblack[320] == 0 and nonblack[321] > 0,
                "Player Setup reveal is not last-black 320 / first-pixel 321")
        tipoff_nonblack = {}
        tipoff_energy = {}
        for frame in (595, 596, 600):
            with Image.open(refs[frame].path) as image:
                pixels = list(image.convert("RGB").getdata())
                tipoff_nonblack[frame] = sum(pixel != (0, 0, 0) for pixel in pixels)
                tipoff_energy[frame] = sum(sum(pixel) for pixel in pixels)
        require(tipoff_nonblack[595] == 0 and tipoff_nonblack[596] > 0 and
                tipoff_energy[600] > tipoff_energy[596],
                "presentation skip did not enter Tipoff through its brightness ramp")
        self.make_contact(
            "team-select-entry-contact.png",
            [refs[frame] for frame in
             (1, 5, 10, 15, 20, 30, 40, 60, 80, 100, 120, 140, 160, 180, 200, 203)],
            "Team Select entry (partial native construction is a preserved original quirk)",
        )
        self.make_contact(
            "team-to-player-setup-contact.png",
            [refs[frame] for frame in
             (203, 204, 210, 220, 228, 229, 240, 250, 253, 254, 280, 298,
              299, 300, 319, 320, 321, 322, 350, 403, 405)],
            "Team Select layer exit and Player Setup destination reveal",
            columns=5,
        )
        self.make_contact(
            "neutral-and-presentation-skips-contact.png",
            [refs[frame] for frame in
             (400, 403, 404, 405, 406, 500, 580, 586, 587, 588, 589, 590,
              591, 593, 594, 595, 596, 620)],
            "Neutral CPU setup and Start skips through Matchup, Ratings, and Lineups",
            columns=6,
        )
        self.checks["frontend"] = {
            "setup_to_team_select_continuous": "captured from production scene handoff",
            "setup_to_team_select_boundary_nonblack_pixels": setup_nonblack,
            "team_select_entry": "captured",
            "team_select_exit": "captured",
            "forced_black_first_frame": 254,
            "forced_black_last_frame": 320,
            "forced_black_presented_frames": 67,
            "player_setup_first_reveal_frame": 321,
            "boundary_nonblack_pixels": nonblack,
            "skip_to_tipoff_nonblack_pixels": tipoff_nonblack,
            "skip_to_tipoff_rgb_energy": tipoff_energy,
            "neutral_cpu_vs_cpu": "state assertion passed",
            "presentation_skips_to_tipoff": "state assertion passed",
        }
        return refs

    def capture_lineup(self, temporary: Path) -> dict[int, FrameRef]:
        script = self.output / "all-lineup-cards.input"
        script.write_text(LINEUP_INPUT, encoding="ascii")
        raw = temporary / "lineup"
        raw.mkdir()
        self.exe_command(
            "lineup-sequence", [
                "--player-setup-only", "--input-script", script,
                "--frames", 1100, "--dump-sequence-from", 180,
                "--dump-sequence-dir", raw, "--debug-state",
            ], ("SCN:PLAYER_INTRO", "CARD:10/10", "ROM LOOP:$87:BE92"),
        )
        intro = [300, 350, 400, 500, 600, 680, 700, 720, 800, 900, 980]
        cards = [990, 1000, 1010, 1020, 1030, 1040, 1050, 1060, 1070, 1080]
        refs = self.export_frames(raw, "lineup", intro + cards)
        card_hashes = []
        for frame in cards:
            with Image.open(refs[frame].path) as image:
                card_hashes.append(hashlib.sha256(image.convert("RGB").tobytes()).hexdigest())
        require(len(set(card_hashes)) == 10,
                "lineup navigation did not produce ten distinct rendered cards")
        self.make_contact(
            "intro-plates-and-logo-contact.png",
            [refs[frame] for frame in intro],
            "Team presentation plates, variable logos, and Ratings court composition",
        )
        self.make_contact(
            "all-lineup-cards-and-text-contact.png",
            [refs[frame] for frame in cards],
            "All ten Starting Lineup cards (inspect jersey/name text, including 31 without bleed)",
            columns=5,
        )
        tipoff_raw = temporary / "lineup-to-tipoff"
        tipoff_raw.mkdir()
        self.exe_command(
            "unskipped-lineup-to-tipoff-sequence", [
                "--player-setup-only", "--player-setup-confirm",
                "--frames", 5340, "--dump-sequence-from", 5319,
                "--dump-sequence-dir", tipoff_raw, "--debug-state",
            ], ("SCN:TIPOFF", "Wrote 22 rendered sequence frames"),
        )
        tipoff_frames = [5319, 5321, 5322, 5323, 5324, 5330, 5340]
        tipoff_refs = self.export_frames(
            tipoff_raw, "lineup-to-tipoff", tipoff_frames
        )
        tipoff_nonblack = {}
        tipoff_energy = {}
        for frame in tipoff_frames:
            with Image.open(tipoff_refs[frame].path) as image:
                pixels = list(image.convert("RGB").getdata())
                tipoff_nonblack[frame] = sum(pixel != (0, 0, 0) for pixel in pixels)
                tipoff_energy[frame] = sum(sum(pixel) for pixel in pixels)
        require(tipoff_nonblack[5321] > 0 and tipoff_nonblack[5322] == 0 and
                tipoff_nonblack[5323] > 0 and
                tipoff_energy[5340] > tipoff_energy[5323],
                "unskipped lineup did not hand off through the Tipoff brightness ramp")
        self.make_contact(
            "unskipped-lineup-to-tipoff-contact.png",
            [tipoff_refs[frame] for frame in tipoff_frames],
            "Unskipped final lineup card into Tipoff and gameplay court",
            columns=4,
        )
        self.checks["lineup"] = {
            "card_count": 10,
            "distinct_rgb_hashes": len(set(card_hashes)),
            "final_state": "CARD:10/10",
            "unskipped_tipoff_first_frame": 5322,
            "unskipped_tipoff_nonblack_pixels": tipoff_nonblack,
            "unskipped_tipoff_rgb_energy": tipoff_energy,
        }
        return refs

    @staticmethod
    def read_trace(path: Path) -> list[dict]:
        with path.open("r", encoding="utf-8") as source:
            return [json.loads(line) for line in source if line.strip()]

    @staticmethod
    def pass_events(rows: list[dict]) -> list[dict]:
        events = []
        previous_key = None
        for index, row in enumerate(rows):
            possession = row["possession"]
            key = (possession["pass_actor_raw"], possession["pass_receiver_raw"])
            actors = row["actors"]
            valid = (0 <= key[0] < len(actors) and 0 <= key[1] < len(actors))
            if valid and key != previous_key:
                actor = actors[key[0]]
                event = {
                    "start": row["frame"],
                    "passer": key[0],
                    "receiver": key[1],
                    "draw_direction": actor["raw"]["draw_direction"],
                    "pass_direction_band": actor["raw"]["pass_direction_66"],
                    "live_state": row["match"]["live_state_raw"],
                    "inbound_transfer": row["match"]["inbound_transfer_raw"],
                    "attachment_xyz": [row["ball"][axis] for axis in ("x", "y", "z")],
                }
                for candidate in rows[index:index + 100]:
                    candidate_possession = candidate["possession"]
                    if ((candidate_possession["pass_actor_raw"],
                         candidate_possession["pass_receiver_raw"]) == key and
                            candidate["ball"]["owner"] == -1):
                        event["release"] = candidate["frame"]
                        event["release_xyz"] = [candidate["ball"][axis]
                                                for axis in ("x", "y", "z")]
                        break
                if "release" in event:
                    events.append(event)
            previous_key = key
        return events

    def capture_gameplay(self, temporary: Path) -> tuple[dict[int, FrameRef], list[dict]]:
        raw = temporary / "gameplay"
        raw.mkdir()
        trace = self.output / "gameplay.jsonl"
        self.exe_command(
            "gameplay-sequence", [
                "--tipoff-only", "--tipoff-clock", 43200,
                "--frames", 2440, "--gameplay-trace", trace,
                "--dump-sequence-from", 275, "--dump-sequence-dir", raw,
                "--debug-state",
            ], ("Wrote 2166 rendered sequence frames",),
        )
        rows = self.read_trace(trace)
        require(len(rows) == 2440, f"expected 2440 gameplay rows, got {len(rows)}")
        events = [event for event in self.pass_events(rows)
                  if event["start"] >= 277 and event["release"] <= 2440]
        require(len(events) >= 8, f"expected at least eight completed passes, got {len(events)}")

        direction_events = []
        seen = set()
        for event in events:
            direction = event["draw_direction"]
            if direction not in seen:
                direction_events.append(event)
                seen.add(direction)
        require(len(direction_events) >= 5,
                f"expected at least five observed draw directions, got {sorted(seen)}")
        for event in events:
            if len(direction_events) >= 8:
                break
            if event not in direction_events:
                direction_events.append(event)

        ordinary = [304, 306, 308, 312, 318, 320, 332, 342, 346, 350]
        inbound = [506, 526, 544, 546, 547, 548, 600, 673, 674, 675, 726,
                   752, 753, 770]
        directional = []
        for event in direction_events:
            start, release = event["start"], event["release"]
            windup = min(start + 6, max(start, release - 1))
            directional.extend((start - 2, start, windup, release))
        selected = sorted(set(ordinary + inbound + directional))
        refs = self.export_frames(raw, "gameplay", selected)
        self.make_contact(
            "ordinary-pass-contact.png",
            [refs[frame] for frame in ordinary],
            "Ordinary pass: before, attached windup, release, flight, receive, catch",
            columns=5,
        )
        self.make_contact(
            "inbound-through-release-contact.png",
            [refs[frame] for frame in inbound],
            "First inbound: install, ready hold, transfer, retry, and release",
        )
        direction_refs = []
        for event in direction_events:
            start, release = event["start"], event["release"]
            windup = min(start + 6, max(start, release - 1))
            phases = (("pre", start - 2), ("attach", start),
                      ("windup", windup), ("release", release))
            for phase, frame in phases:
                base = refs[frame]
                direction_refs.append(FrameRef(
                    base.path,
                    f"d{event['draw_direction']} {phase} f{frame}",
                    base.group,
                    frame,
                ))
        self.make_contact(
            "broad-direction-pass-contact.png", direction_refs,
            "Natural CPU passes across observed draw directions",
            columns=4,
        )

        by_frame = {row["frame"]: row for row in rows}
        ordinary_crops = []
        for frame in (306, 308, 312, 318, 320, 342, 346, 350):
            actor = 3 if frame <= 320 else 4
            ordinary_crops.append(self.actor_crop(
                refs[frame], by_frame[frame], actor,
                f"ordinary actor {actor} f{frame}",
            ))
        self.make_contact(
            "ordinary-pass-actor-crops.png", ordinary_crops,
            "Ordinary pass actor crops: attached pose, release, and receiver catch",
            columns=4,
        )

        inbound_crops = []
        for frame in (526, 544, 546, 547, 548, 600, 673, 674, 675, 726, 752, 753):
            inbound_crops.append(self.actor_crop(
                refs[frame], by_frame[frame], 7, f"inbound actor 7 f{frame}",
            ))
        self.make_contact(
            "inbound-actor-crops.png", inbound_crops,
            "Inbound carrier crops: arrival hand pose, ready hold, transfer, and release",
            columns=4,
        )

        direction_crops = []
        for event in direction_events:
            start, release = event["start"], event["release"]
            windup = min(start + 6, max(start, release - 1))
            for phase, frame in (("attach", start), ("windup", windup),
                                 ("release", release)):
                direction_crops.append(self.actor_crop(
                    refs[frame], by_frame[frame], event["passer"],
                    f"d{event['draw_direction']} {phase} f{frame}",
                ))
        self.make_contact(
            "broad-direction-pass-actor-crops.png", direction_crops,
            "Passer crops across natural CPU draw directions",
            columns=3,
        )
        self.checks["gameplay"] = {
            "trace_rows": len(rows),
            "completed_passes": len(events),
            "observed_draw_directions": sorted(seen),
            "direction_contact_events": direction_events,
            "direct_entry": "--tipoff-only production caller",
            "controlled_clock_seed": 43200,
            "actor_crop_rule": "104x112 around telemetry screen_x/screen_y, nearest-neighbor 2x",
        }
        return refs, rows

    def actor_crop(self, ref: FrameRef, row: dict, actor_id: int,
                   label: str) -> FrameRef:
        actor = row["actors"][actor_id]
        require(actor["visible"], f"actor {actor_id} is hidden at frame {row['frame']}")
        width, height = 104, 112
        left = actor["screen_x"] - width // 2
        top = actor["screen_y"] - 88
        with Image.open(ref.path) as image:
            source = image.convert("RGB")
            padded = Image.new("RGB", (width, height), (0, 0, 0))
            source_box = (
                max(0, left), max(0, top),
                min(source.width, left + width), min(source.height, top + height),
            )
            require(source_box[0] < source_box[2] and source_box[1] < source_box[3],
                    f"actor {actor_id} crop is outside frame {row['frame']}")
            region = source.crop(source_box)
            padded.paste(region, (source_box[0] - left, source_box[1] - top))
            scaled = padded.resize((width * 2, height * 2), Image.Resampling.NEAREST)
        destination = self.frames / "gameplay-crops"
        destination.mkdir(parents=True, exist_ok=True)
        safe_label = "".join(character if character.isalnum() else "-"
                             for character in label.lower()).strip("-")
        target = destination / f"{safe_label}.png"
        scaled.save(target)
        scaled.close()
        padded.close()
        region.close()
        source.close()
        return FrameRef(target, label, "gameplay-crops", row["frame"])

    def capture_player_lab(self, temporary: Path) -> list[FrameRef]:
        cases = [
            (3, 0, 0), (3, 1, 1), (3, 2, 2), (3, 3, 3),
            (18, 0, 4), (18, 1, 5), (18, 2, 6), (18, 3, 7),
            (8, 0, 0), (19, 1, 2), (23, 2, 4), (27, 3, 6),
        ]
        destination = self.frames / "player-lab"
        destination.mkdir(parents=True, exist_ok=True)
        refs = []
        source_kinds = set()
        for team, roster, direction in cases:
            raw = temporary / f"player-lab-t{team:02d}-r{roster:02d}-d{direction}.bmp"
            name = f"player-lab-t{team:02d}-r{roster:02d}-d{direction}"
            stdout = self.exe_command(name, [
                "--player-lab", "--player-team", team,
                "--player-roster", roster, "--player-animation", 0,
                "--player-direction", direction, "--frames", 1,
                "--dump-frame", raw,
            ], ("[PLAYER LAB]", "source=asset-pack"))
            source_kinds.add("asset-pack" if "source=asset-pack" in stdout else "other")
            target = destination / f"team-{team:02d}-roster-{roster:02d}-d{direction}.png"
            with Image.open(raw) as image:
                rgb = image.convert("RGB")
                require(any(pixel != (0, 0, 0) for pixel in rgb.getdata()),
                        f"blank Player Lab sample {name}")
                rgb.save(target)
            refs.append(FrameRef(target, f"team {team:02d} roster {roster:02d} d{direction}",
                                 "player-lab"))
        require(source_kinds == {"asset-pack"}, "Player Lab did not use packed player data")
        self.make_contact(
            "jersey-body-head-number-grid.png", refs,
            "Player Lab body, head, jersey, and number samples from NBPDRAW1",
        )
        self.checks["player_lab"] = {
            "sample_count": len(refs),
            "teams": sorted({case[0] for case in cases}),
            "directions": sorted({case[2] for case in cases}),
            "source": "asset-pack",
        }
        return refs

    def make_contact(self, filename: str, refs: list[FrameRef], title: str,
                     columns: int = 4) -> FrameRef:
        require(refs, f"contact sheet {filename} has no frames")
        images = [(ref, Image.open(ref.path).convert("RGB")) for ref in refs]
        width, height = images[0][1].size
        title_height, label_height = 30, 24
        rows = (len(images) + columns - 1) // columns
        sheet = Image.new(
            "RGB", (columns * width, title_height + rows * (height + label_height)),
            (24, 24, 24),
        )
        draw = ImageDraw.Draw(sheet)
        draw.text((8, 8), title, fill=(255, 255, 255))
        for index, (ref, image) in enumerate(images):
            x = (index % columns) * width
            y = title_height + (index // columns) * (height + label_height)
            draw.text((x + 5, y + 5), ref.label, fill=(255, 255, 255))
            sheet.paste(image, (x, y + label_height))
            image.close()
        target = self.output / filename
        sheet.save(target)
        sheet.close()
        contact = FrameRef(target, title, "contact")
        self.contacts.append(contact)
        return contact

    def make_court_contact(self, frontend: dict[int, FrameRef],
                           lineup: dict[int, FrameRef],
                           gameplay: dict[int, FrameRef]) -> None:
        refs = []
        for label, ref in (
            ("New York Matchup skip f586", frontend[586]),
            ("New York Ratings skip f588", frontend[588]),
            ("Orlando Matchup plate f500", lineup[500]),
            ("Orlando oval / Ratings f800", lineup[800]),
            ("Ordinary pass court f304", gameplay[304]),
            ("Inbound install court f526", gameplay[526]),
            ("Inbound transfer court f674", gameplay[674]),
            ("Inbound release court f752", gameplay[752]),
        ):
            refs.append(FrameRef(ref.path, label, ref.group, ref.frame))
        self.make_contact(
            "court-and-logo-contact.png", refs,
            "Court/logo checkpoints (Ratings overlap is a documented original quirk)",
            columns=4,
        )

    def regression(self, name: str, script: str, arguments: list[object]) -> None:
        self.command(name, [sys.executable, ROOT / "tools" / script, *arguments])

    def run_regressions(self, gameplay_trace: Path) -> None:
        common = ["--exe", self.exe, "--rom", self.rom, "--pack", self.pack]
        self.regression("test-setup-transition", "test_setup_transition.py", common)
        self.regression("test-team-select", "test_team_select.py", common)
        self.regression("test-frontend-route", "test_frontend_route.py", common)
        self.regression("test-player-setup", "test_player_setup.py", common)
        self.regression("test-player-intro", "test_player_intro.py", common)
        self.regression("test-player-intro-text", "test_player_intro_text.py", common)
        self.regression("test-court-assets", "test_court_assets.py",
                        ["--pack", self.pack])
        self.regression("test-court-logo-attribution", "test_court_logo_attribution.py",
                        ["--pack", self.pack, "--rom", self.rom])
        pose_output = self.output / "regressions" / "sprite-pose-source"
        self.regression("test-sprite-pose-runtime-source",
                        "test_sprite_pose_runtime_source.py",
                        ["--root", ROOT, "--output", pose_output])
        inbound_output = self.output / "regressions" / "consecutive-inbound"
        self.regression("test-consecutive-inbound-sequence",
                        "test_consecutive_inbound_sequence.py",
                        ["--trace", gameplay_trace, "--output", inbound_output])
        self.checks["regressions"] = [
            "setup_transition", "team_select", "frontend_route", "player_setup",
            "player_intro", "player_intro_text_all_145", "court_assets",
            "court_logo_attribution",
            "sprite_pose_runtime_source", "consecutive_inbound_sequence",
        ]

    def compress_trace(self, path: Path) -> Path:
        target = path.with_suffix(path.suffix + ".gz")
        with path.open("rb") as source, gzip.open(target, "wb", compresslevel=6) as sink:
            shutil.copyfileobj(source, sink, length=1024 * 1024)
        path.unlink()
        return target

    def artifacts(self) -> dict[str, dict]:
        result = {}
        for path in sorted(self.output.rglob("*")):
            if path.is_file() and path.name != "manifest.json":
                relative = str(path.relative_to(self.output)).replace("\\", "/")
                result[relative] = {"bytes": path.stat().st_size, "sha256": sha256(path)}
        return result

    def write_review(self, skip_regressions: bool) -> None:
        gate_status = (
            "capture gates passed; focused regressions were explicitly skipped"
            if skip_regressions else
            "automated capture and focused regression gates passed"
        )
        lines = [
            "# Visible smoke checkpoint review",
            "",
            f"Status: {gate_status}; contact sheets are ready for independent visual review.",
            "",
            "This is a deterministic test seed around production scene callers. It is not a full-game",
            "completion claim and it does not replace natural retail-route or native parity evidence.",
            "",
            "Review these contacts:",
            "",
        ]
        for contact in self.contacts:
            lines.append(f"- [{contact.label}]({contact.path.name})")
        lines.extend([
            "",
            "Manual visual checks:",
            "",
            "- Team Select builds in source phases; clipped BG3 reveal fragments are documented original behavior.",
            "- Team Select layers withdraw before the 67-frame black construction interval; frame 320 is black and Player Setup first reveals at frame 321.",
            "- Player Setup's first partial right-edge layers match the documented original construction reveal.",
            "- Neutral controller setup reaches CPU-vs-CPU and Start skips Matchup, Ratings, and the whole lineup.",
            "- Both the Start-skip route and the complete ten-card lineup enter Tipoff through black and the court brightness ramp.",
            "- The ten-card contact renders complete proportional text; the all-145 regression rejects three-digit bleed and locks the native `00` sentinels.",
            "- The Orlando oval/Ratings overlap is a documented original-game composition quirk.",
            "- During pass and inbound attachment frames the ball stays at the submitted hand pose, then separates on release.",
            "- Player bodies, heads, jersey colors, and numbers remain composed across the lab and live-direction grids.",
            "",
            "The harness deliberately leaves `.analysis/progress-screenshots/latest` unchanged.",
        ])
        (self.output / "review.md").write_text("\n".join(lines) + "\n", encoding="utf-8")

    def write_index(self) -> None:
        cards = "\n".join(
            f'<section><h2>{html.escape(contact.label)}</h2>'
            f'<a href="{html.escape(contact.path.name)}">'
            f'<img src="{html.escape(contact.path.name)}" loading="lazy"></a></section>'
            for contact in self.contacts
        )
        document = f"""<!doctype html>
<html lang="en"><head><meta charset="utf-8">
<title>NBA Live 95 visible smoke checkpoints</title>
<style>
body{{font:16px system-ui,sans-serif;background:#171717;color:#eee;margin:24px}}
a{{color:#8ecbff}} section{{margin:28px 0}} img{{max-width:100%;height:auto;border:1px solid #555}}
code{{background:#292929;padding:2px 5px}}
</style></head><body>
<h1>NBA Live 95 visible smoke checkpoints</h1>
<p>Automated route gates passed. Review <a href="review.md">review.md</a> and
<a href="manifest.json">manifest.json</a>. This run did not update <code>latest</code>.</p>
{cards}
</body></html>
"""
        (self.output / "index.html").write_text(document, encoding="utf-8")

    def write_manifest(self, pack_info: dict, trace: Path,
                       skip_regressions: bool) -> Path:
        manifest = {
            "schema": 1,
            "status": ("CAPTURE_ONLY_READY_FOR_VISUAL_REVIEW" if skip_regressions else
                       "PASS_READY_FOR_INDEPENDENT_VISUAL_REVIEW"),
            "generated_utc": datetime.now(timezone.utc).isoformat(),
            "elapsed_seconds": round(time.monotonic() - self.started, 3),
            "test_only": True,
            "production_executable_unmodified": True,
            "source_commit": git_text("rev-parse", "HEAD"),
            "source_dirty_status": git_text("status", "--short"),
            "identities": {
                "executable": {"path": str(self.exe), "sha256": sha256(self.exe)},
                "rom": {"path": str(self.rom), "sha256": sha256(self.rom)},
                "asset_pack": {"path": str(self.pack), "sha256": sha256(self.pack),
                               **pack_info},
                "gameplay_trace_gzip": {"path": str(trace.relative_to(self.output)).replace("\\", "/"),
                                         "sha256": sha256(trace)},
            },
            "boundaries": {
                "frontend": "continuous --setup-only handoff plus --team-only production caller and native input words",
                "lineup": "--player-setup-only production NbaGame caller plus native input words",
                "gameplay": "--tipoff-only production caller with an explicit long clock seed",
                "player_lab": "F9 production player compositor debug route",
                "normal_runtime_seeded": False,
                "native_evidence": False,
            },
            "checks": self.checks,
            "regressions_skipped": skip_regressions,
            "commands": self.commands,
            "contacts": [contact.path.name for contact in self.contacts],
            "latest_updated": False,
            "artifacts": self.artifacts(),
            "limits": [
                "Deterministic current-port smoke coverage; not a full-game or every-team claim.",
                "Direct test entries are clearly reported and do not count as a natural retail-route witness.",
                "Contact sheets require independent human visual review before publication.",
            ],
        }
        target = self.output / "manifest.json"
        target.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
        return target


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True,
                        help="production nba95_port executable")
    parser.add_argument("--pack", type=Path, required=True,
                        help="production v31 asset pack containing NBPDRAW1")
    parser.add_argument("--rom", type=Path, required=True,
                        help="verified NBA Live 95 USA ROM")
    parser.add_argument("--output", type=Path, required=True,
                        help="new directory under .analysis/progress-screenshots")
    parser.add_argument("--skip-regressions", action="store_true",
                        help="capture contacts only; manifest is not an accepted regression run")
    args = parser.parse_args()

    output = args.output.resolve()
    require_progress_output(output)
    if output.exists():
        parser.error(f"output must be new so stale evidence cannot mix: {output}")
    for label, path in (("executable", args.exe), ("ROM", args.rom),
                        ("asset pack", args.pack)):
        if not path.resolve().is_file():
            parser.error(f"{label} does not exist: {path.resolve()}")
    require(sha256(args.rom.resolve()) == ROM_SHA256,
            "ROM identity is not the supported NBA Live 95 USA image")
    pack_info = inspect_pack(args.pack.resolve())

    output.parent.mkdir(parents=True, exist_ok=True)
    output.mkdir()
    harness = Harness(args.exe, args.rom, args.pack, output)
    harness.logs.mkdir()
    harness.frames.mkdir()
    try:
        with tempfile.TemporaryDirectory(prefix="nba95-visible-smoke-") as temp_name:
            temporary = Path(temp_name)
            frontend = harness.capture_frontend(temporary)
            lineup = harness.capture_lineup(temporary)
            gameplay, _ = harness.capture_gameplay(temporary)
            harness.capture_player_lab(temporary)
            harness.make_court_contact(frontend, lineup, gameplay)
        trace = output / "gameplay.jsonl"
        if not args.skip_regressions:
            harness.run_regressions(trace)
        compressed_trace = harness.compress_trace(trace)
        harness.write_review(args.skip_regressions)
        harness.write_index()
        manifest = harness.write_manifest(pack_info, compressed_trace,
                                          args.skip_regressions)
    except Exception as error:
        failure = {
            "status": "FAIL_PARTIAL_OUTPUT_RETAINED",
            "error": str(error),
            "source_commit": git_text("rev-parse", "HEAD"),
            "commands": harness.commands,
            "checks": harness.checks,
        }
        (output / "failure.json").write_text(
            json.dumps(failure, indent=2) + "\n", encoding="utf-8"
        )
        raise

    print(f"VISIBLE SMOKE PASS: {output}")
    print(f"manifest sha256: {sha256(manifest)}")
    print("status: ready for independent visual review; latest was not changed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except RuntimeError as error:
        print(f"VISIBLE SMOKE FAIL: {error}", file=sys.stderr)
        raise SystemExit(1)
