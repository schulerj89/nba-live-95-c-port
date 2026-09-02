"""Press through setup headlessly and capture a complete, naturally reached dribble."""
import argparse
import gzip
import hashlib
import html
import itertools
import json
import os
from pathlib import Path
import struct
import subprocess
import time

from PIL import Image, ImageDraw

from run_visible_smoke_checkpoints import FRONTEND_INPUT
from test_headless_input import read_trace, verify_inputs
from verify_dribble_vectors import verify as verify_native, verify_draw, ROM_SHA256

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def ball_sprite(rom):
    """Independent reference from ROM descriptor $081D and palette $AF:E99F."""
    def offset(address):
        return ((address >> 16) & 127) * 32768 + (address & 32767)
    address = 0x898000 + (struct.unpack_from("<I", rom, offset(0x898000) + 0x81d * 4)[0] & 0xffffff)
    base = offset(address)
    count, _, size, second, graphics = struct.unpack_from("<5H", rom, base)
    x, y, attributes, large = struct.unpack_from("<hhHB", rom, base + 10)
    if (count, size, second, attributes, large) != (1, 32, 0, 0, 0):
        raise ValueError("unexpected original ball descriptor")
    tile = rom[base + graphics:base + graphics + 32]
    palette = struct.unpack_from("<16H", rom, offset(0xafe99f))
    pixels = []
    for py in range(8):
        for px in range(8):
            index = sum(((tile[py * 2 + plane // 2 * 16 + plane % 2] >> (7 - px)) & 1) << plane
                        for plane in range(4))
            if index:
                color = tuple((((palette[index] >> s) & 31) << 3) |
                              (((palette[index] >> s) & 31) >> 2) for s in (0, 5, 10))
                pixels.append((px, py, color))
    return {"address": f"{address:06X}", "offset": [x, y], "tile": tile, "pixels": pixels}


def check_ball(image, x, y, pixels):
    visible = [(x + px, y + py, color) for px, py, color in pixels
               if 0 <= x + px < 256 and 0 <= y + py < 224]
    failures = [(px, py) for px, py, color in visible if image.getpixel((px, py)) != color]
    return len(visible), failures


def require_coverage(evidence, episode, sprite_pixels):
    cycle = [e for e in evidence if episode[0] <= e["frame"] <= episode[1]]
    bounce = [e["frame"] for e in cycle if e["rom_phase"] == 4 and
              e["ball_pixels_checked"] == sprite_pixels and e["ball_pixels_occluded"] == 0]
    hand = [e["frame"] for e in cycle if e["rom_phase"] in (0, 1, 2, 6, 7) and
            0 < e["ball_pixels_occluded"] < e["ball_pixels_checked"]]
    if not bounce or not hand:
        raise AssertionError("dribble must show both an unobstructed bounce and a hand-phase OBJ overlap")
    return {"unobstructed_bounce_frames": bounce, "hand_phase_overlap_frames": hand}


def gallery(directory, items, title):
    """Keep every frame, plus readable crops, contact pages and slow motion."""
    directory.mkdir(parents=True, exist_ok=True)
    pages = []
    animated = []
    for start in range(0, len(items), 24):
        group = items[start:start + 24]
        sheet = Image.new("RGB", (6 * 240, 36 + 4 * 264), "#111820")
        draw = ImageDraw.Draw(sheet)
        draw.text((12, 10), title, fill="white")
        for i, item in enumerate(group):
            im = Image.open(item["path"]).convert("RGB")
            cx, cy = item["center"]
            crop = im.crop((cx - 40, cy - 64, cx + 40, cy + 16)).resize((240, 240), Image.Resampling.NEAREST)
            col, row = i % 6, i // 6
            sheet.paste(crop, (col * 240, 36 + row * 264))
            draw.text((col * 240 + 4, 36 + row * 264 + 243), item["label"], fill="white")
            animated.append(im.resize((768, 672), Image.Resampling.NEAREST))
        name = f"contact_{start // 24 + 1:02d}.png"
        sheet.save(directory / name)
        pages.append(name)
    if animated:
        animated[0].save(directory / "slow-motion.gif", save_all=True, append_images=animated[1:],
                         duration=67, loop=0, disposal=2)
    cards = "\n".join(f'<figure><img src="{html.escape(os.path.relpath(i["path"], directory).replace(chr(92), "/"))}">'
                       f'<figcaption>{html.escape(i["label"])}</figcaption></figure>' for i in items)
    (directory / "index.html").write_text(
        '<!doctype html><meta charset="utf-8"><title>' + html.escape(title) + '</title>'
        '<style>body{background:#111820;color:#edf3fa;font:16px sans-serif;margin:24px}'
        'img{image-rendering:pixelated;max-width:100%}#frames{display:flex;flex-wrap:wrap}'
        'figure{margin:8px}figure img{width:512px}figcaption{padding:6px}</style>'
        '<h1>' + html.escape(title) + '</h1><p>Every captured frame is retained below. '
        'The animation plays at one quarter speed.</p><img src="slow-motion.gif">'
        '<div id="frames">' + cards + '</div>', encoding="utf-8")
    return pages


def dribbler(row):
    owner = row["ball"]["owner"]
    if not 0 <= owner < 10:
        return -1
    actor = row["actors"][owner]
    if actor["raw"]["control_mode"] != 11 or actor["animation"] not in (9, 11):
        return -1
    return owner


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--exe", type=Path, default=ROOT / "build/nba95_port.exe")
    parser.add_argument("--pack", type=Path, default=ROOT / "build/nba95_assets.pak")
    parser.add_argument("--probe", type=Path, default=ROOT / "build/ball_driver_owned_vector_probe.exe")
    parser.add_argument("--draw-probe", type=Path, default=ROOT / "build/dribble_draw_vector_probe.exe")
    parser.add_argument("--output", type=Path, default=ROOT / "build" / time.strftime("dribble-smoke-%Y%m%d-%H%M%S"))
    parser.add_argument("--search-frames", type=int, default=6000)
    parser.add_argument("--tipoff-only", action="store_true", help="seed neutral Tipoff directly instead of pressing through setup")
    args = parser.parse_args()
    started = time.monotonic()
    if sha(args.rom) != ROM_SHA256 or not 600 <= args.search_frames <= 20000:
        raise ValueError("wrong ROM or search duration")
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    reference = ball_sprite(args.rom.read_bytes())
    native = verify_native(ROOT / "tests/fixtures/dribble-native.json", args.probe, args.pack, args.rom)
    native_draw = verify_draw(ROOT / "tests/fixtures/dribble-draw-native.json", args.draw_probe, args.pack, args.rom)
    commands = []
    common = [str(args.exe.resolve()), "--headless", "--rom", str(args.rom.resolve()),
              "--assets", str(args.pack.resolve())]
    expected_inputs = []
    if args.tipoff_only:
        common += ["--tipoff-only"]
    else:
        script = out / "buttons.rle"
        script.write_text(FRONTEND_INPUT)
        for line in FRONTEND_INPUT.splitlines():
            count, word = line.split()
            expected_inputs.extend([int(word, 16)] * int(count))
        common += ["--team-only", "--input-script", str(script)]

    def run(name, frames, extra):
        trace = out / (name + ".jsonl")
        inputs = out / (name + "-inputs.csv")
        command = common + ["--frames", str(frames), "--gameplay-trace", str(trace),
                            "--input-trace", str(inputs)] + [str(v) for v in extra]
        result = subprocess.run(command, capture_output=True, text=True, timeout=90,
                                creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
        (out / (name + ".log")).write_text(result.stdout + result.stderr)
        commands.append({"args": command, "exit_code": result.returncode})
        if result.returncode or result.stdout.count("[HEADLESS] Headless execution completed successfully.") != 1:
            raise AssertionError(f"{name} did not complete; see retained log")
        actual_inputs = read_trace(inputs, frames)
        expected = (expected_inputs + [0] * frames)[:frames]
        verify_inputs(actual_inputs, expected)
        rows = [json.loads(line) for line in trace.read_text().splitlines()]
        with gzip.open(trace.with_suffix(".jsonl.gz"), "wt") as compressed:
            for row in rows:
                compressed.write(json.dumps(row, separators=(",", ":")) + "\n")
        return rows, actual_inputs

    rows, _ = run("search", args.search_frames, [])
    candidates = []
    for owner, group in itertools.groupby(rows, dribbler):
        group = list(group)
        if owner < 0 or len(group) < 32:
            continue
        phases = {r["actors"][owner]["raw"]["animation_rom"]["upper_phase_3a"] for r in group}
        if phases == set(range(8)) and min(r["ball"]["z"] for r in group) <= 2 and \
                max(r["ball"]["z"] for r in group) >= 8 and all(
                    12 <= r["ball"]["screen_x"] <= 240 and 40 <= r["ball"]["screen_y"] <= 204 for r in group):
            candidates.append(group)
    if not candidates:
        raise AssertionError("no complete visible dribble cycle reached; retained search trace is diagnostic")
    episode = candidates[0]
    owner = episode[0]["ball"]["owner"]
    first, last = episode[0]["frame"] - 8, episode[-1]["frame"] + 8
    raw = out / "frames"
    raw.mkdir()
    replay, inputs = run("capture", last, ["--dump-sequence-from", first,
        "--dump-sequence-dir", raw, "--dump-sequence-layers"])
    if replay != [r for r in rows if r["frame"] <= last]:
        raise AssertionError("rendered replay diverged from the ordinary gameplay search")
    captured = [r for r in replay if first <= r["frame"] <= last]
    if [r["frame"] for r in captured] != list(range(first, last + 1)) or \
            len(list(raw.glob("*.bmp"))) != len(captured):
        raise AssertionError("capture dropped, duplicated or reordered gameplay frames")
    evidence, items = [], []
    for row in captured:
        frame = row["frame"]
        bmp = raw / f"frame_{frame:04d}.bmp"
        im = Image.open(bmp).convert("RGB")
        if im.size != (256, 224):
            raise AssertionError(f"wrong image dimensions at {frame}")
        png = bmp.with_suffix(".png")
        im.save(png)
        ball = row["ball"]
        x = ball["screen_x"] + reference["offset"][0]
        y = ball["screen_y"] + reference["offset"][1]
        checked, failures = check_ball(im, x, y, reference["pixels"])
        layer_file = raw / f"frame_{frame:04d}.layers"
        layer_data = layer_file.read_bytes()
        if len(layer_data) != 32 + 256 * 224 or layer_data[:8] != b"NBLAYER1":
            raise AssertionError("missing/truncated winning-layer evidence")
        # The AF1E pose stream legitimately places fingers and other players
        # in front of the ball. Every displaced ball-colored pixel must be
        # covered by an OBJ; an exposed floor pixel means the ball moved.
        uncovered = [(px, py) for px, py in failures if layer_data[32 + py * 256 + px] != 4]
        if uncovered:
            raise AssertionError(f"frame {frame}: {len(uncovered)}/{checked} exposed ball pixels "
                                 f"differ from the ROM sprite at {x},{y}")
        actor = row["actors"][owner]
        phase = actor["raw"]["animation_rom"]["upper_phase_3a"]
        items.append({"path": png, "center": [actor["screen_x"], actor["screen_y"]],
                      "label": f"f{frame} p{phase} z{ball['z']} owner {ball['owner']}"})
        evidence.append({"frame": frame, "png": str(png.relative_to(out)), "sha256": sha(png),
                         "ball_origin": [ball["screen_x"], ball["screen_y"]],
                         "ball_top_left": [x, y], "ball_pixels_checked": checked,
                         "ball_pixels_occluded": len(failures),
                         "mismatches": 0, "rom_phase": phase,
                         "compatibility_phase": actor["raw"]["upper_phase"]})
    coverage = require_coverage(evidence, [episode[0]["frame"], episode[-1]["frame"]],
                                len(reference["pixels"]))
    pages = gallery(out / "review", items, "Dribble smoke: original hand phase and ball placement")
    report = {"passed": True, "elapsed_seconds": round(time.monotonic() - started, 3),
              "exe_sha256": sha(args.exe), "pack_sha256": sha(args.pack), "rom_sha256": ROM_SHA256,
              "native": native, "native_draw": native_draw, "commands": commands, "first": first, "last": last,
              "episode": [episode[0]["frame"], episode[-1]["frame"]], "actor": owner,
              "button_presses": sum(bool(r["pressed"]) for r in inputs),
              "ball_sprite": {"resource": "081D", "descriptor": reference["address"], "offset": reference["offset"]},
              "frames": evidence, "coverage": coverage, "contact_pages": pages}
    (out / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(f"[DRIBBLE SMOKE] PASS: {len(evidence)} consecutive frames, phases=0..7, "
          f"{report['button_presses']} button presses, {sum(e['ball_pixels_checked'] for e in evidence)} "
          f"ROM ball pixels, {native['replays']} native physics replays, "
          f"{native_draw['native_calls']} native draw replays, {report['elapsed_seconds']}s")
    print(out / "review/index.html")


if __name__ == "__main__":
    main()
