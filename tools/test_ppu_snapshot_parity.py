"""Differential test: independent Python oracle vs production C PPU snapshot."""

import argparse
import json
import subprocess
import sys
import tempfile
from pathlib import Path

from PIL import Image

sys.path.insert(0, str(Path(__file__).parent))
from snes_ppu_oracle import load_snapshot, render_snapshot


def rasterize_state(state_path, raster_path, target_frame):
    """Reconstruct TM at each visible scanline from bounded register writes."""
    state = state_path.read_text()
    writes = []
    for line in raster_path.read_text().splitlines():
        fields = line.split()
        if len(fields) >= 4 and fields[0].isdigit() and fields[2] == "212C":
            writes.append((int(fields[0]), int(fields[1]), int(fields[3], 16)))
    # gameplay_frame advances at endFrame. The upload + visible writes tagged
    # N therefore produce the completed scanout captured as N+1 at $80:8188.
    source_frame = target_frame - 1
    setup = [value for frame, scanline, value in writes
             if frame == source_frame and scanline >= 224]
    if not setup:
        raise AssertionError("raster trace does not include the scanout's TM setup write")
    current = setup[-1]
    by_line = []
    frame_writes = sorted((scanline, value) for frame, scanline, value in writes
                          if frame == source_frame and scanline < 224)
    index = 0
    for scanline in range(224):
        while index < len(frame_writes) and frame_writes[index][0] <= scanline:
            current = frame_writes[index][1]
            index += 1
        by_line.append(current)
    return state + "".join(
        f"audit.mainScreenLayers[{line}]={value}\n"
        for line, value in enumerate(by_line))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--probe", required=True)
    parser.add_argument("--native-dir", required=True)
    parser.add_argument("--frame", type=int, default=990)
    parser.add_argument("--out", required=True)
    parser.add_argument("--prefix", default="scanout")
    parser.add_argument("--raster-log")
    parser.add_argument("--report")
    args = parser.parse_args()
    root, frame = Path(args.native_dir), args.frame
    prefix = root / f"{args.prefix}_{frame:04d}"
    files = [prefix.with_name(prefix.name + suffix) for suffix in
             ("_vram.bin", "_cgram.bin", "_oam.bin", "_state.txt")]
    missing = [str(path) for path in files if not path.exists()]
    if missing:
        raise AssertionError("missing native PPU fixture(s): " + ", ".join(missing))
    output = Path(args.out)
    state_text = files[3].read_text()
    if args.raster_log:
        state_text = rasterize_state(files[3], Path(args.raster_log), frame)
    with tempfile.TemporaryDirectory() as temporary:
        augmented = Path(temporary) / "state.txt"
        augmented.write_text(state_text)
        probe_files = files[:3] + [augmented]
        subprocess.run([args.probe, *map(str, probe_files), str(output)], check=True)
        from snes_ppu_oracle import parse_state
        state = parse_state(augmented)
        expected, _ = render_snapshot(
            files[0].read_bytes(), files[1].read_bytes(), files[2].read_bytes(), state)
    actual = list(Image.open(output).convert("RGB").getdata())
    mismatches = [i for i, pair in enumerate(zip(actual, expected)) if pair[0] != pair[1]]
    if mismatches:
        first = mismatches[0]
        raise AssertionError(
            f"C/Python PPU mismatch: {len(mismatches)} pixels; "
            f"first=({first % 256},{first // 256}) C={actual[first]} oracle={expected[first]}")
    native = list(Image.open(prefix.with_suffix(".png")).convert("RGB").getdata())
    native_mismatch = sum(left != right for left, right in zip(actual, native))
    if native_mismatch:
        raise AssertionError(f"C/native PPU mismatch: {native_mismatch} pixels")
    regions = {
        "right_goal": (166, 0, 256, 123),
        "out_of_bounds_and_sideline": (0, 0, 166, 96),
    }
    region_report = {}
    for name, (x0, y0, x1, y1) in regions.items():
        indices = [y * 256 + x for y in range(y0, y1) for x in range(x0, x1)]
        mismatches = sum(actual[i] != native[i] for i in indices)
        region_report[name] = {"pixels": len(indices), "mismatch_pixels": mismatches}
    report = {
        "status": "PASS", "frame": frame, "pixels": len(actual),
        "c_vs_independent_oracle_mismatch_pixels": 0,
        "c_vs_mesen_mismatch_pixels": native_mismatch,
        "regions": region_report,
    }
    if args.report:
        Path(args.report).write_text(json.dumps(report, indent=2) + "\n")
    print(f"PPU snapshot PASS: frame {frame}, 57344/57344 pixels match independent oracle and Mesen")


if __name__ == "__main__":
    main()
