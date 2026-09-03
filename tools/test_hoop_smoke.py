"""Reach both baskets headlessly and check every captured BG1 pixel."""
import argparse
import gzip
import json
from pathlib import Path
import struct
import subprocess
import time

import numpy as np
from PIL import Image, ImageDraw

from regenerate_hoop_reference import ROM_SHA256, sha
from run_visible_smoke_checkpoints import FRONTEND_INPUT
from test_headless_input import read_trace, verify_inputs
from upgrade_gameplay_hud_pack import unpack
from verify_hoop_raster import verify_vectors

ROOT = Path(__file__).resolve().parents[1]


def s16(value):
    value &= 65535
    return value - 65536 if value >= 32768 else value


def raster(camera):
    """Independent source equations, gated separately by measured Mesen writes."""
    y = np.arange(224)
    scroll = s16(camera["raw_087e"])
    right = np.full(224, camera["raw_0880"], dtype=np.int32)
    if camera["x"] >= 0:
        timer = (255 - scroll) & 65535
        enabled = (y < (timer & 511)) & (s16(timer - 512) >= 0)
    else:
        first = s16(4 - scroll)
        enabled = y >= (first & 511) if first >= 0 else np.ones(224, dtype=bool)
        right[y >= ((first + 76) & 511)] = max(0, s16(camera["raw_0880"] - 55)) & 255
    return enabled, right


def tiles(vram, base):
    raw = np.frombuffer(vram[base:base + 32768], dtype=np.uint8).reshape(1024, 32)
    result = np.zeros((1024, 8, 8), dtype=np.uint8)
    for plane in range(4):
        rows = raw[:, np.arange(8) * 2 + (plane // 2) * 16 + plane % 2]
        result |= ((rows[:, :, None] >> (7 - np.arange(8))) & 1).astype(np.uint8) << plane
    return result


def sample(tile_data, entries, x, y):
    tx = np.where(entries & 0x4000, 7 - (x & 7), x & 7)
    ty = np.where(entries & 0x8000, 7 - (y & 7), y & 7)
    return tile_data[entries & 1023, ty, tx]


class CourtOracle:
    def __init__(self, pack, rom, home):
        assets = {r[0]: r[4] for r in unpack(Path(pack).read_bytes())}
        state = assets[284][24 + home * 0x10200:24 + (home + 1) * 0x10200]
        vram, cgram = state[:65536], state[65536:]
        self.goal_map = np.frombuffer(vram[:2048], dtype="<u2").reshape(32, 32)
        self.goal_tiles = tiles(vram, 0x2000)
        self.court_tiles = tiles(vram, 0x4000)
        palette = np.frombuffer(cgram, dtype="<u2")
        channels = (palette[:, None] >> np.array([0, 5, 10])) & 31
        self.rgb = ((channels << 3) | (channels >> 2)).astype(np.uint8)
        base = 0x100000 if home in (1, 16, 18) else 0x103c26
        data = Path(rom).read_bytes()
        self.court_map = np.frombuffer(data[base + 6:base + 6 + 148 * 52 * 2],
                                       dtype="<u2").reshape(148, 52)

    def check(self, im, layers, camera):
        x, y = np.meshgrid(np.arange(256), np.arange(224))
        gx = (x + camera["raw_087c"]) & 255
        gy = (y + camera["raw_087e"] + 1) & 255
        goal_entry = self.goal_map[gy >> 3, gx >> 3]
        goal_color = sample(self.goal_tiles, goal_entry, gx, gy)
        palette = ((goal_entry >> 10) & 7) * 16 + goal_color
        enabled, right = raster(camera)
        goal = (goal_color != 0) & enabled[:, None] & \
               (x >= camera["raw_0882"]) & (x <= right[:, None])
        cx = x + min(max(camera["x"] + 582, 0), 928)
        cy = y + min(max(camera["y"] + 243, 0), 192)
        court_entry = self.court_map[cx >> 3, cy >> 3]
        court_color = sample(self.court_tiles, court_entry, cx, cy)
        # BG1-high wins over both BG2 priorities. BG2-high beats BG1-low.
        goal &= ((goal_entry & 0x2000) != 0) | ((court_entry & 0x2000) == 0) | (court_color == 0)
        actual = layers == 1
        comparable = layers <= 2  # HUD and foreground sprites may cover the board.
        missing = goal & comparable & ~actual
        extra = actual & ~goal
        wrong_color = actual & np.any(im != self.rgb[palette], axis=2)
        return {"pixels_examined": 256 * 224, "goal_pixels": int(actual.sum()),
                "expected_visible_goal_pixels": int((goal & comparable).sum()),
                "missing_goal_pixels": int(missing.sum()), "extra_goal_pixels": int(extra.sum()),
                "wrong_goal_colors": int(wrong_color.sum())}


def contact(directory, rows, title):
    frames = []
    for page in range(0, len(rows), 24):
        sheet = Image.new("RGB", (1536, 1008), "#111820")
        draw = ImageDraw.Draw(sheet)
        draw.text((8, 8), title, fill="white")
        for i, row in enumerate(rows[page:page + 24]):
            im = Image.open(directory / f'frame_{row["frame"]:04d}.png').convert("RGB")
            sheet.paste(im, ((i % 6) * 256, 32 + (i // 6) * 244))
            draw.text(((i % 6) * 256 + 4, 32 + (i // 6) * 244 + 226),
                      f'frame {row["frame"]}  camera {row["camera"]["x"]},{row["camera"]["y"]}', fill="white")
            frames.append(im.resize((768, 672), Image.Resampling.NEAREST))
        sheet.save(directory / f"contact_{page // 24 + 1:02d}.png")
    frames[0].save(directory / "slow-motion.gif", save_all=True, append_images=frames[1:],
                   duration=67, loop=0, disposal=2)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--exe", type=Path, default=ROOT / "build/nba95_port.exe")
    parser.add_argument("--pack", type=Path, default=ROOT / "build/nba95_assets.pak")
    parser.add_argument("--probe", type=Path, default=ROOT / "build/hoop_raster_probe.exe")
    parser.add_argument("--output", type=Path, default=ROOT / "build" / time.strftime("hoop-smoke-%Y%m%d-%H%M%S"))
    parser.add_argument("--home-team", type=int)
    parser.add_argument("--through-menus", action="store_true")
    args = parser.parse_args()
    if args.through_menus and args.home_team is not None:
        raise ValueError("--home-team configures direct Tipoff; the button route selects New York")
    home_team = 17 if args.through_menus else (18 if args.home_team is None else args.home_team)
    if sha(args.rom) != ROM_SHA256 or not 0 <= home_team < 29:
        raise ValueError("wrong ROM or home team")
    started = time.monotonic()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    native = verify_vectors(ROOT / "tests/fixtures/hoop-raster-native.json", args.probe)
    oracle = CourtOracle(args.pack, args.rom, home_team)
    common = [str(args.exe.resolve()), "--headless", "--rom", str(args.rom.resolve()),
              "--assets", str(args.pack.resolve())]
    expected_inputs = []
    if args.through_menus:
        script = out / "buttons.rle"
        script.write_text(FRONTEND_INPUT)
        common += ["--team-only", "--input-script", str(script)]
        for line in FRONTEND_INPUT.splitlines():
            duration, word = line.split()
            expected_inputs.extend([int(word, 16)] * int(duration))
    else:
        common += ["--tipoff-only", "--tipoff-home-team", str(home_team)]
    commands = []

    def run(name, frames, extra):
        trace = out / (name + ".jsonl")
        inputs = out / (name + "-inputs.csv")
        command = common + ["--frames", str(frames), "--gameplay-trace", str(trace),
                            "--input-trace", str(inputs)] + list(map(str, extra))
        result = subprocess.run(command, capture_output=True, text=True, timeout=90,
                                creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
        (out / (name + ".log")).write_text(result.stdout + result.stderr)
        commands.append({"args": command, "exit_code": result.returncode})
        if result.returncode or "[HEADLESS] Headless execution completed successfully." not in result.stdout:
            raise AssertionError(f"{name} did not complete; see retained log")
        verify_inputs(read_trace(inputs, frames), (expected_inputs + [0] * frames)[:frames])
        rows = []
        with trace.open() as source, gzip.open(trace.with_suffix(".jsonl.gz"), "wt") as compressed:
            for line in source:
                compressed.write(line)
                row = json.loads(line)
                rows.append({"frame": row["frame"], "camera": row["camera"]})
        return rows

    search = run("search", 6000 if args.through_menus else 2800, [])
    episodes = {}
    for side in ("north", "south"):
        for index, row in enumerate(search[:-23]):
            group = search[index:index + 24]
            cameras = [r["camera"] for r in group]
            if side == "north":
                visible = all(c["x"] >= 120 and -220 < c["y"] < -115 for c in cameras)
            else:
                visible = all(c["x"] < -400 for c in cameras)
            if visible:
                episodes[side] = group
                break
        if side not in episodes:
            raise AssertionError(f"no visible {side} basket sequence reached")
    results = []
    for side, group in episodes.items():
        first, last = group[0]["frame"], group[-1]["frame"]
        directory = out / side
        directory.mkdir()
        rows = run(side, last, ["--dump-sequence-from", first, "--dump-sequence-dir", directory,
                                "--dump-sequence-layers"])
        captured = [r for r in rows if first <= r["frame"] <= last]
        if captured != group or len(list(directory.glob("*.bmp"))) != 24:
            raise AssertionError("replay diverged or dropped capture frames")
        for row in captured:
            prefix = directory / f'frame_{row["frame"]:04d}'
            im = Image.open(prefix.with_suffix(".bmp")).convert("RGB")
            raw = prefix.with_suffix(".layers").read_bytes()
            if im.size != (256, 224) or len(raw) != 32 + 256 * 224 or raw[:8] != b"NBLAYER1":
                raise AssertionError("invalid capture dimensions or layer evidence")
            if struct.unpack_from("<I", raw, 16)[0] != row["frame"] or \
               list(struct.unpack_from("<hh", raw, 24)) != [row["camera"]["x"], row["camera"]["y"]] or \
               raw[28] != home_team:
                raise AssertionError("capture state/header mismatch")
            png = prefix.with_suffix(".png")
            im.save(png)
            checked = oracle.check(np.asarray(im), np.frombuffer(raw[32:], dtype=np.uint8).reshape(224, 256), row["camera"])
            results.append({"side": side, **row, **checked, "image_sha256": sha(png)})
        contact(directory, captured, f"{side.title()} basket: every frame")
    errors = sum(r["missing_goal_pixels"] + r["extra_goal_pixels"] + r["wrong_goal_colors"] for r in results)
    if any(r["expected_visible_goal_pixels"] < 50 for r in results):
        raise AssertionError("smoke did not expose enough of a basket")
    report = {"status": "FAIL" if errors else "PASS", "seconds": round(time.monotonic() - started, 3),
              "exe_sha256": sha(args.exe), "pack_sha256": sha(args.pack), "rom_sha256": ROM_SHA256,
              "native": native, "home_team": home_team,
              "button_presses": sum(v != 0 for v in expected_inputs),
              "commands": commands, "frames": results, "pixel_failures": errors}
    (out / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({k: report[k] for k in ("status", "seconds", "button_presses", "pixel_failures")}))
    print(f"Checked all {len(results)} frames; gallery: {out}")
    if errors:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
