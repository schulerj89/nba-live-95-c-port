"""Capture and check every Tipoff frame against ROM maps and native court tiles."""
import argparse
import gzip
import hashlib
import html
import json
import os
from pathlib import Path
import struct
import subprocess
import time

import numpy as np
from PIL import Image, ImageDraw, ImageFont

from extract_assets import decode_bg_layer, load_verified_rom
from upgrade_gameplay_hud_pack import unpack

PARQUET = {1, 16, 18}
WIDTH, HEIGHT = 256, 224
FLOOR = (470, 160, 710, 322)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def reference_surface(rom, capture_root, team):
    # Independent source input: raw native team PPU tiles, never pack panoramas.
    directory = capture_root / "player_intro_portraits_verified_20260823" / f"team_{team:02d}"
    vram = (directory / "slot_0_vram.bin").read_bytes()
    cgram = bytearray(512)
    pointer = int.from_bytes(rom[0x265BD + team * 4:0x265BD + team * 4 + 3], "little")
    offset = ((pointer >> 16) & 0x7f) * 0x8000 + (pointer & 0x7fff)
    palette = rom[offset:offset + 0xD6]
    for begin, end, source in ((0, 2, 0), (34, 38, 2), (64, 192, 0x20), (240, 246, 0xD0)):
        cgram[begin:end] = palette[source:source + end - begin]
    base = 0x100000 if team in PARQUET else 0x103C26
    words = np.frombuffer(rom[base + 6:base + 6 + 148 * 52 * 2], dtype="<u2").reshape(148, 52).T
    atlas = np.frombuffer(vram[0x4000:0xC000], dtype=np.uint8).reshape(1024, 32)
    shifts = np.arange(7, -1, -1)
    tiles = np.zeros((1024, 8, 8), dtype=np.uint8)
    for plane in range(4):
        rows = atlas[:, (plane // 2) * 16 + np.arange(8) * 2 + plane % 2]
        tiles |= (((rows[:, :, None] >> shifts) & 1) << plane).astype(np.uint8)
    y, x = np.indices((416, 1184))
    entry = words[y // 8, x // 8]
    tx = (x & 7) ^ (((entry >> 14) & 1) * 7)
    ty = (y & 7) ^ (((entry >> 15) & 1) * 7)
    colors = tiles[entry & 1023, ty, tx]
    indices = ((entry >> 10) & 7) * 16 + colors
    palette_words = np.frombuffer(cgram, dtype="<u2")
    value = palette_words[indices]
    rgb5 = np.stack((value & 31, (value >> 5) & 31, (value >> 10) & 31), axis=2)
    return ((rgb5 << 3) | (rgb5 >> 2)).astype(np.uint8)


def native_check(folder, team, expected):
    manifest = json.loads((folder / "manifest.json").read_text())
    if manifest["exit_code"] or manifest["team"] != team or not manifest["isolation"]["post_settings_verified"]:
        raise AssertionError("native capture identity/isolation failed")
    uploads = [item for item in manifest["uploads"] if item["home"] == team]
    destination = (0xB520 if team in PARQUET else 0xB4A0) // 2
    if not uploads or any((item["length"], item["vram_word"], item["source"]) !=
                          (0x8C0, destination, 0x7E8FEE) for item in uploads):
        raise AssertionError("native team upload used a different layout destination")
    results = []
    for path in sorted(folder.glob("frame_*_state.txt")):
        state = dict(line.split("=", 1) for line in path.read_text().splitlines())
        assert int(state["home"]) == team
        assert int(state["map_address"]) == (0x8000 if team in PARQUET else 0xBC26)
        stem = str(path).removesuffix("_state.txt")
        vram, cgram = Path(stem + "_vram.bin").read_bytes(), Path(stem + "_cgram.bin").read_bytes()
        native = decode_bg_layer(vram, cgram, 0x1000, 0x4000, 4, True, False,
                                 int(state["ppu.layers[1].hscroll"]), int(state["ppu.layers[1].vscroll"]))
        rgb = np.asarray(Image.frombytes("RGBA", (WIDTH, HEIGHT), native, "raw", "BGRA").convert("RGB"))
        dest = int(state["destination"])
        hs, vs = int(state["ppu.layers[1].hscroll"]), int(state["ppu.layers[1].vscroll"])
        ring_x, ring_y = (dest & 31) + ((dest & 0x400) >> 5), (dest & 0x3E0) >> 5
        px = int(state["coarse_x"]) * 8 + (((hs >> 3) - ring_x + 32) % 64 - 32) * 8 + (hs & 7)
        py = int(state["coarse_y"]) * 8 + (((vs >> 3) - ring_y + 16) % 32 - 16) * 8 + (vs & 7) + 1
        yy, xx = np.indices((HEIGHT, WIDTH))
        floor = ((xx + px >= FLOOR[0]) & (xx + px < FLOOR[2]) &
                 (yy + py >= FLOOR[1]) & (yy + py < FLOOR[3]))
        target = expected[py:py + HEIGHT, px:px + WIDTH]
        if target.shape != rgb.shape:
            raise AssertionError("native published viewport outside court")
        mismatch = int(np.count_nonzero(np.any(rgb != target, axis=2) & floor))
        if mismatch:
            raise AssertionError(f"native {team} {path.name}: {mismatch} floor pixels differ")
        results.append({"frame": path.name, "pixels": int(floor.sum()), "mismatches": mismatch})
    if len(results) < 6:
        raise AssertionError("native comparison did not cover both early and settled frames")
    return results


def native_crowd_check(root):
    # The descriptor tables relocate these same 28 native fan tiles by four
    # slots on standard courts. This checks raw PPU bytes, not port output.
    ids = list(range(808, 821)) + list(range(849, 864))
    results = []
    for frame in (20, 60, 90, 140, 160, 180):
        parquet = (root / "native-orlando" / f"frame_{frame:04d}_vram.bin").read_bytes()
        normal = (root / "native-new-york-state" / f"frame_{frame:04d}_vram.bin").read_bytes()
        for tile in ids:
            if normal[0x4000 + (tile - 4) * 32:0x4000 + (tile - 3) * 32] != \
                    parquet[0x4000 + tile * 32:0x4000 + (tile + 1) * 32]:
                raise AssertionError(f"native crowd relocation differs at frame {frame}, tile {tile}")
        results.append({"frame": frame, "relocated_tiles": len(ids), "different_bytes": 0})
    return results


def contact(frames, path, title, columns=4):
    font = ImageFont.truetype("C:/Windows/Fonts/consola.ttf", 13)
    rows = (len(frames) + columns - 1) // columns
    canvas = Image.new("RGB", (columns * 268 + 12, 44 + rows * 250), "#111820")
    draw = ImageDraw.Draw(canvas)
    draw.text((12, 12), title, font=font, fill="white")
    for i, (png, label) in enumerate(frames):
        x, y = 12 + (i % columns) * 268, 40 + (i // columns) * 250
        with Image.open(png) as image:
            canvas.paste(image, (x, y))
        draw.text((x, y + 226), label, font=font, fill="#accfff")
    canvas.save(path)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("exe", "pack", "rom", "capture-root", "native-root", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--teams", default="all")
    parser.add_argument("--frames", type=int, default=240)
    args = parser.parse_args()
    if not 220 <= args.frames <= 400:
        raise ValueError("capture through jump ball and first possession (220..400 frames)")
    teams = list(range(29)) if args.teams == "all" else [int(v) for v in args.teams.split(",")]
    if len(set(teams)) != len(teams) or any(team not in range(29) for team in teams):
        raise ValueError("teams must be unique IDs 0..28")
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    rom = load_verified_rom(args.rom)
    records = unpack(args.pack.read_bytes())
    assets = {record[0]: record for record in records}
    for ident, base, address in ((279, 0x100000, 0xA08000), (288, 0x103C26, 0xA0BC26)):
        if assets[ident] != (ident, 148, 52, address,
                             rom[base:base + 6 + 148 * 52 * 2]):
            raise AssertionError("pack court map differs from literal selected ROM map")
    env = {k: v for k, v in os.environ.items() if not k.startswith("NBA95_")}
    base_command = [str(args.exe.resolve()), "--headless", "--rom", str(args.rom.resolve()),
                    "--assets", str(args.pack.resolve())]
    report = {"status": "RUNNING", "exe_sha256": sha(args.exe.read_bytes()),
              "pack_sha256": sha(args.pack.read_bytes()), "rom_sha256": sha(rom),
              "test_sha256": sha(Path(__file__).read_bytes()), "frames": 0, "floor_pixels": 0,
              "native": {}, "cases": []}
    surfaces = {team: reference_surface(rom, args.capture_root.resolve(), team)
                for team in set(teams) | {1, 16, 17, 18}}
    native_folders = {1: "native-boston", 16: "native-milwaukee",
                      17: "native-new-york-state", 18: "native-orlando"}
    try:
        for team, folder in native_folders.items():
            report["native"][str(team)] = native_check(
                args.native_root / folder, team, surfaces[team])
        report["native_crowd_relocation"] = native_crowd_check(args.native_root)
        # Invalid state seeds must fail before producing a successful capture.
        for options in (["--tipoff-only", "--tipoff-home-team", "29"],
                        ["--tipoff-only", "--tipoff-away-team", "abc"],
                        ["--tipoff-home-team", "17"],
                        ["--tipoff-only", "--dump-sequence-layers"]):
            run = subprocess.run(base_command + options, env=env, capture_output=True, timeout=30)
            if run.returncode == 0:
                raise AssertionError("invalid state seed accepted: " + repr(options))
        cases = [(f"team-{team:02d}", team, 1, args.frames,
                  ["--tipoff-only", "--tipoff-home-team", str(team),
                   "--tipoff-away-team", "17" if team == 3 else "3"]) for team in teams]
        route = out / "frontend.input"
        values = [0] * (596 + args.frames)
        for frame, value in ((201, 0x0200), (203, 0x1000), (404, 0x0200),
                             (406, 0x1000), (587, 0x1000), (589, 0x1000), (595, 0x1000)):
            values[frame] = value
        lines, start = [], 1
        for i in range(2, len(values) + 1):
            if i == len(values) or values[i] != values[start]:
                lines.append(f"{i - start} {values[start]:04X}")
                start = i
        route.write_text("\n".join(lines) + "\n")
        cases.append(("buttons-new-york", 17, 595, 595 + args.frames,
                      ["--team-only", "--input-script", str(route)]))
        for name, team, first, last, options in cases:
            folder = out / name
            folder.mkdir()
            command = base_command + options + [
                "--frames", str(last), "--dump-sequence-dir", str(folder),
                "--dump-sequence-from", str(first), "--dump-sequence-layers",
                "--debug-state"]
            started = time.time()
            run = subprocess.run(command, env=env, capture_output=True, timeout=120,
                                 creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
            (folder / "stdout.txt").write_bytes(run.stdout)
            (folder / "stderr.txt").write_bytes(run.stderr)
            if run.returncode or b"SCN:TIPOFF" not in run.stdout or b"TIP PH:LIVE" not in run.stdout:
                raise AssertionError(f"{name}: Tipoff sequence failed, see stdout/stderr")
            frame_results = []
            for frame in range(first, last + 1):
                bmp = folder / f"frame_{frame:04d}.bmp"
                mask_path = bmp.with_suffix(".layers")
                mask_bytes = mask_path.read_bytes()
                if (bmp.stat().st_mtime < started - 2 or mask_path.stat().st_mtime < started - 2 or
                        len(mask_bytes) != 32 + WIDTH * HEIGHT or mask_bytes[:8] != b"NBLAYER1"):
                    raise AssertionError(f"{name}/{frame}: stale or malformed output")
                w, h, observed_step, state_frame, cx, cy, home, away, layout, reserved = struct.unpack_from(
                    "<4I2h4B", mask_bytes, 8)
                expected_state = frame if first == 1 else frame - first
                if (w, h, observed_step, state_frame, home, away, layout, reserved) != (
                        WIDTH, HEIGHT, frame, expected_state, team,
                        17 if team == 3 else 3, int(team not in PARQUET), 0):
                    raise AssertionError(f"{name}/{frame}: configured state mismatch")
                layers = np.frombuffer(mask_bytes, dtype=np.uint8, offset=32).reshape(HEIGHT, WIDTH)
                with Image.open(bmp) as image:
                    rgb = np.asarray(image.convert("RGB"))
                if rgb.shape != (HEIGHT, WIDTH, 3) or np.max(layers) > 4:
                    raise AssertionError("invalid rendered frame or layer index")
                px, py = max(0, min(cx + 582, 928)), max(0, min(cy + 243, 192))
                expected = surfaces[team][py:py + HEIGHT, px:px + WIDTH]
                brightness = min(state_frame, 15)
                if brightness < 15:
                    expected = (expected.astype(np.uint16) * brightness // 15).astype(np.uint8)
                yy, xx = np.indices((HEIGHT, WIDTH))
                visible_floor = ((layers == 2) &
                    (xx + px >= FLOOR[0]) & (xx + px < FLOOR[2]) &
                    (yy + py >= FLOOR[1]) & (yy + py < FLOOR[3]))
                pixels = int(visible_floor.sum())
                mismatch = int(np.count_nonzero(np.any(rgb != expected, axis=2) & visible_floor))
                if pixels < 5000 or mismatch:
                    raise AssertionError(f"{name}/{frame}: {pixels} checked floor pixels; {mismatch} mismatches")
                png = bmp.with_suffix(".png")
                Image.fromarray(rgb).save(png)
                with gzip.open(str(mask_path) + ".gz", "wb") as compressed:
                    compressed.write(mask_bytes)
                frame_results.append({"frame": frame, "state_frame": state_frame,
                    "camera": [cx, cy], "home": home, "away": away, "layout": layout,
                    "floor_pixels": pixels, "floor_mismatches": mismatch,
                    "png_sha256": sha(png.read_bytes()), "layers_sha256": sha(mask_bytes)})
                # Remove only losslessly converted files created in this fresh case folder.
                bmp.unlink()
                mask_path.unlink()
                report["frames"] += 1
                report["floor_pixels"] += pixels
            if len(list(folder.glob("frame_*.png"))) != last - first + 1:
                raise AssertionError("missing or extra sequence frame")
            (folder / "frames.json").write_text(json.dumps(frame_results, indent=2) + "\n")
            report["cases"].append({"name": name, "team": team, "frames": len(frame_results),
                                    "command": command, "exit_code": run.returncode})
            print(f"{name}: {len(frame_results)} frames, zero floor/logo mismatches", flush=True)
        if report["exe_sha256"] != sha(args.exe.read_bytes()) or report["pack_sha256"] != sha(args.pack.read_bytes()):
            raise AssertionError("binary or pack changed during smoke")
        choices = [case["name"] for case in report["cases"]]
        options = "".join(f'<option>{html.escape(name)}</option>' for name in choices)
        page = ('<!doctype html><meta charset="utf-8"><title>Tipoff frame smoke</title>'
                '<style>body{background:#111820;color:white;font:16px system-ui;margin:32px}'
                'img{width:768px;max-width:95vw;image-rendering:pixelated}input{width:500px}</style>'
                '<h1>Every Tipoff frame</h1><p>ROM floor/layout checks passed for every captured frame.</p>'
                f'<select id="team">{options}</select> <input id="frame" type="range" min="1" '
                f'max="{args.frames}" value="90"><span id="label"></span><p><img id="image"></p>'
                '<script>const t=document.querySelector("#team"),f=document.querySelector("#frame");'
                f'function show(){{f.max={args.frames}+(t.value==="buttons-new-york"?1:0);'
                'let n=Number(f.value)+(t.value==="buttons-new-york"?594:0);'
                'document.querySelector("#label").textContent=" Frame "+n;'
                'document.querySelector("#image").src=t.value+"/frame_"+String(n).padStart(4,"0")+".png"}'
                't.onchange=f.oninput=show;show()</script>')
        (out / "index.html").write_text(page)
        if 17 in teams:
            for start in range(1, args.frames + 1, 60):
                contact([(out / "team-17" / f"frame_{i:04d}.png", f"New York: frame {i}")
                         for i in range(start, min(start + 60, args.frames + 1))],
                        out / f"new-york-all-frames-{start:04d}.png",
                        "New York: consecutive Tipoff frames", columns=5)
        contact([(out / f"team-{team:02d}" / "frame_0090.png", f"Home team {team:02d}")
                 for team in teams], out / "all-home-courts.png",
                "Home-selected Tipoff courts: frame 90", columns=5)
        report["status"] = "PASS"
    except Exception as error:
        report["status"] = "FAIL"
        report["error"] = str(error)
        raise
    finally:
        (out / "manifest.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"status": report["status"], "frames": report["frames"],
                      "floor_pixels": report["floor_pixels"], "output": str(out)}))


if __name__ == "__main__":
    main()
