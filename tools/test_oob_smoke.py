"""Headless OOB gameplay: verify every appearance/hold/retirement frame."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import time

import numpy as np
from PIL import Image, ImageDraw

from oob_visual import contacts, decode
from regenerate_oob_reference import ROM_SHA256, sha
from run_visible_smoke_checkpoints import FRONTEND_INPUT
from test_headless_input import read_trace, verify_inputs

ROOT = Path(__file__).resolve().parents[1]
FIXTURE_SHA256 = "c0ff305a164c6fd215c66aa686a389734a047276d1758b6a3dde41f5bd85f079"


def digest(data):
    return hashlib.sha256(data).hexdigest()


def contract(pack, probe, output):
    fixture = ROOT / "tests/fixtures/out-of-bounds-hud.json"
    if sha(fixture) != FIXTURE_SHA256:
        raise ValueError("immutable native HUD witness changed")
    expected = json.loads(fixture.read_text())["expected"]
    output.mkdir()
    run = subprocess.run([str(probe.resolve()), str(pack.resolve()), str(output)],
                         capture_output=True, text=True, timeout=30,
                         creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
    (output / "probe.log").write_text(run.stdout + run.stderr)
    if run.returncode or "OOB_CONTRACT 58" not in run.stdout:
        raise AssertionError("dispatcher/possession/retirement/scoreboard contract failed")
    for name in ("layout.map", "layout.chr", "native-text.chr"):
        if sha(output / name) != expected[name]:
            raise AssertionError("C differs from native canvas: " + name)
    native = (output / "team_09_side_0.vram").read_bytes()
    if digest(native[0x9C0:0xB40]) != expected["visible_map_09c0_0b40"] or \
            digest(native[0x2470:0x2EF0]) != expected["published_characters_2470_2ef0"]:
        raise AssertionError("C publication differs from native VRAM")
    palette = (output / "palette.cgram").read_bytes()
    labels = {}
    gallery = Image.new("RGB", (780, 10 * 75), (24, 25, 30))
    draw = ImageDraw.Draw(gallery)
    title = None
    for team in range(29):
        first = (output / f"team_{team:02d}_side_0.vram").read_bytes()
        second = (output / f"team_{team:02d}_side_1.vram").read_bytes()
        if first != second:
            raise AssertionError("the possession branches disagree for the same awarded team")
        rgb, mask = decode(first, palette)
        if mask[:56].any() or mask[104:].any() or mask[56:104, :16].any() or mask[56:104, 240:].any():
            raise AssertionError("text escaped its original grid")
        if not mask[56:80].any() or not mask[80:104].any():
            raise AssertionError("missing title or possession line")
        if title is None:
            title = mask[56:80].copy()
        elif not np.array_equal(title, mask[56:80]):
            raise AssertionError("violation title changed with the team")
        labels[team] = (rgb, mask)
        preview = rgb.copy()
        preview[~mask] = (55, 57, 62)
        x, y = (team % 3) * 260, (team // 3) * 75
        draw.text((x + 2, y), f"ROM team {team}", fill="white")
        gallery.paste(Image.fromarray(preview).crop((0, 54, 256, 104)), (x, y + 18))
    gallery.save(output / "all-team-labels.png")
    return labels


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--exe", type=Path, default=ROOT / "build/nba95_port.exe")
    parser.add_argument("--pack", type=Path, default=ROOT / "build/nba95_assets.pak")
    parser.add_argument("--probe", type=Path, default=ROOT / "build/oob_contract_probe.exe")
    parser.add_argument("--output", type=Path, default=ROOT / "build" / time.strftime("oob-smoke-%Y%m%d-%H%M%S"))
    parser.add_argument("--through-menus", action="store_true")
    parser.add_argument("--baseline-exe", type=Path)
    parser.add_argument("--baseline-pack", type=Path)
    args = parser.parse_args()
    if sha(args.rom) != ROM_SHA256 or bool(args.baseline_exe) != bool(args.baseline_pack):
        raise ValueError("wrong ROM or incomplete baseline arguments")
    started = time.monotonic()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    labels = contract(args.pack, args.probe, out / "contract")
    configuration = ["--headless", "--rom", str(args.rom.resolve())]
    expected_inputs = []
    home, visitor = (17 if args.through_menus else 18), 3
    if args.through_menus:
        script = out / "buttons.rle"
        script.write_text(FRONTEND_INPUT)
        configuration += ["--team-only", "--input-script", str(script)]
        for line in FRONTEND_INPUT.splitlines():
            duration, word = line.split()
            expected_inputs += [int(word, 16)] * int(duration)
    else:
        configuration += ["--tipoff-only", "--tipoff-home-team", str(home)]
    commands = []

    def run(name, frames, extras=(), exe=None, pack=None):
        trace, inputs = out / (name + ".jsonl"), out / (name + "-inputs.csv")
        command = [str((exe or args.exe).resolve()), *configuration,
                   "--assets", str((pack or args.pack).resolve()), "--frames", str(frames),
                   "--gameplay-trace", str(trace), "--input-trace", str(inputs), *map(str, extras)]
        result = subprocess.run(command, capture_output=True, text=True, timeout=90,
                                creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
        (out / (name + ".log")).write_text(result.stdout + result.stderr)
        commands.append({"args": command, "exit_code": result.returncode})
        if result.returncode or "[HEADLESS] Headless execution completed successfully." not in result.stdout:
            raise AssertionError("headless game did not complete: " + name)
        verify_inputs(read_trace(inputs, frames), (expected_inputs + [0] * frames)[:frames])
        return [json.loads(line) for line in trace.read_text().splitlines()]

    search = run("search", 6000 if args.through_menus else 1000)
    call = next(r for r in search if r["fouls"]["latched_event_raw_08f0"] == 3 and
                r["fouls"]["whistle_timer_raw_08de"] < 0x8000)
    first = call["frame"]
    draw_frame = next(r["frame"] for r in search if r["frame"] >= first and
                      r["fouls"]["hud_sequence_raw_08e6"] == 65535 and
                      r["fouls"]["whistle_timer_raw_08de"] < 0x8000)
    clear_frame = next(r["frame"] for r in search if r["frame"] > draw_frame and
                       r["fouls"]["whistle_timer_raw_08de"] >= 0x8000)
    if not first < draw_frame < first + 10 or not 150 < clear_frame - first < 250:
        raise AssertionError("unexpected OOB dispatcher lifecycle")
    first_capture, last_capture = first - 4, clear_frame + 12
    directory = out / "frames"
    directory.mkdir()
    replay = run("capture", last_capture, ["--dump-sequence-from", first_capture,
                 "--dump-sequence-dir", directory, "--dump-sequence-layers"])
    captured = [r for r in replay if first_capture <= r["frame"] <= last_capture]
    expected = [r for r in search if first_capture <= r["frame"] <= last_capture]
    if captured != expected or len(list(directory.glob("*.bmp"))) != last_capture - first_capture + 1:
        raise AssertionError("replay diverged or dropped a captured frame")
    results = []
    for row in captured:
        prefix = directory / f"frame_{row['frame']:04d}"
        with Image.open(prefix.with_suffix(".bmp")) as bmp:
            im = bmp.convert("RGB")
        data = prefix.with_suffix(".layers").read_bytes()
        if im.size != (256, 224) or len(data) != 32 + 256 * 224 or data[:8] != b"NBLAYER1" or \
                struct.unpack_from("<I", data, 16)[0] != row["frame"] or data[28] != home:
            raise AssertionError("invalid frame or capture-state header")
        if list(struct.unpack_from("<hh", data, 24)) != [row["camera"]["x"], row["camera"]["y"]]:
            raise AssertionError("captured camera does not match trace")
        layers = np.frombuffer(data[32:], dtype=np.uint8).reshape(224, 256)
        f = row["fouls"]
        side = 1 if f["hud_event_actor_raw_492d"] < 5 else 0
        team = (home, visitor)[side]
        rgb, mask = labels[team]
        visible = draw_frame <= row["frame"] < clear_frame
        if visible and (f["hud_kind_raw_08e8"] != 17 or f["latched_event_raw_08f0"] != 3 or
                        f["hud_pending_routine"] or row["match"]["inbound_state_raw"] != side * 5):
            raise AssertionError("wrong event, continuation or possession label")
        desired = mask if visible else np.zeros_like(mask)
        actual = layers == 3
        roi = np.zeros_like(mask)
        roi[56:104, 16:240] = True
        missing = int((desired & ~actual).sum())
        extra = int((actual & roi & ~desired).sum())
        wrong = int((desired & np.any(np.asarray(im) != rgb, axis=2)).sum())
        png = prefix.with_suffix(".png")
        im.save(png)
        results.append({"frame": row["frame"], "timer": f["whistle_timer_raw_08de"],
                        "file": png.name, "visible": visible, "awarded_team": team if visible else None,
                        "expected_pixels": int(desired.sum()), "missing": missing,
                        "extra": extra, "wrong_color": wrong, "sha256": sha(png)})
    pages = contacts(directory, results)
    negative = None
    if args.baseline_exe:
        frame = draw_frame + 5
        baseline = out / "baseline"
        baseline.mkdir()
        old = run("baseline", frame, ["--dump-sequence-from", frame,
                  "--dump-sequence-dir", baseline, "--dump-sequence-layers"], args.baseline_exe, args.baseline_pack)
        current = next(r for r in replay if r["frame"] == frame)
        old = next(r for r in old if r["frame"] == frame)
        for field in ("camera", "ball", "actors", "match", "possession"):
            if old[field] != current[field]:
                raise AssertionError("negative-control gameplay state differs")
        layers = np.frombuffer((baseline / f"frame_{frame:04d}.layers").read_bytes()[32:], dtype=np.uint8).reshape(224, 256)
        record = next(r for r in results if r["frame"] == frame)
        mask = labels[record["awarded_team"]][1]
        missing = int((mask & (layers != 3)).sum())
        if missing != int(mask.sum()):
            raise AssertionError("baseline did not reproduce the missing overlay")
        Image.open(baseline / f"frame_{frame:04d}.bmp").save(baseline / "before.png")
        negative = {"frame": frame, "missing_text_pixels": missing, "exe_sha256": sha(args.baseline_exe),
                    "pack_sha256": sha(args.baseline_pack), "same_gameplay_state": True}
    failures = sum(r["missing"] + r["extra"] + r["wrong_color"] for r in results)
    report = {"status": "FAIL" if failures else "PASS", "seconds": round(time.monotonic() - started, 3),
              "exe_sha256": sha(args.exe), "pack_sha256": sha(args.pack), "rom_sha256": ROM_SHA256,
              "fixture_sha256": FIXTURE_SHA256, "probe_sha256": sha(args.probe), "commands": commands,
              "through_menus": args.through_menus, "state_injection": False, "call_frame": first,
              "draw_frame": draw_frame, "clear_frame": clear_frame, "captured_frames": len(results),
              "pixel_failures": failures, "contact_pages": pages, "negative_control": negative, "frames": results}
    (out / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({k: report[k] for k in ("status", "seconds", "captured_frames", "pixel_failures", "call_frame", "draw_frame", "clear_frame")}))
    if failures:
        raise AssertionError("rendered OOB frames differ from the verified original HUD")


if __name__ == "__main__":
    main()
