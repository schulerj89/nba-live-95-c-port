"""Compare asset-pack-backed runtime court inputs with a native scanout."""

import argparse
import json
import sys
import tempfile
from pathlib import Path

from PIL import Image

sys.path.insert(0, str(Path(__file__).parent))
from snes_ppu_oracle import parse_state, render_snapshot
from test_ppu_snapshot_parity import rasterize_state


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--native-dir", required=True)
    parser.add_argument("--port-frame", required=True)
    parser.add_argument("--frame", type=int, default=989)
    parser.add_argument("--report")
    args = parser.parse_args()
    root, frame = Path(args.native_dir), args.frame
    prefix = root / f"scanout_{frame:04d}"
    state_path = prefix.with_name(prefix.name + "_state.txt")
    state_text = rasterize_state(
        state_path, root / "ppu_raster_writes.txt", frame)
    with tempfile.TemporaryDirectory() as temporary:
        augmented = Path(temporary) / "state.txt"
        augmented.write_text(state_text)
        state = parse_state(augmented)
    native, winners = render_snapshot(
        prefix.with_name(prefix.name + "_vram.bin").read_bytes(),
        prefix.with_name(prefix.name + "_cgram.bin").read_bytes(),
        prefix.with_name(prefix.name + "_oam.bin").read_bytes(), state)
    port = list(Image.open(args.port_frame).convert("RGB").getdata())
    if len(port) != len(native):
        raise AssertionError("runtime frame is not 256x224")

    # OAM 33/34 are the `$0822` rim/net resource in this native witness.
    goal_oam = {33, 34}
    background = [i for i, winner in enumerate(winners)
                  if winner[1] != "OBJ"]
    goal_object = [i for i, winner in enumerate(winners)
                   if winner[1] == "OBJ" and winner[5] in goal_oam]
    regions = {
        "right_goal_nonplayer": [i for i in range(len(native))
                                  if 166 <= i % 256 < 256 and i // 256 < 123
                                  and (winners[i][1] != "OBJ" or
                                       winners[i][5] in goal_oam)],
        "out_of_bounds_and_sideline": [i for i in range(len(native))
                                        if i % 256 < 166 and i // 256 < 96
                                        and winners[i][1] != "OBJ"],
    }
    result = {
        "status": "PASS", "native_frame": frame,
        "camera": [135, -220],
        "background_pixels": len(background),
        "background_mismatch_pixels": sum(port[i] != native[i]
                                            for i in background),
        "goal_obj_pixels": len(goal_object),
        "goal_obj_mismatch_pixels": sum(port[i] != native[i]
                                         for i in goal_object),
        "regions": {},
    }
    for name, indices in regions.items():
        result["regions"][name] = {
            "pixels": len(indices),
            "mismatch_pixels": sum(port[i] != native[i] for i in indices),
        }
    failures = (result["background_mismatch_pixels"] +
                result["goal_obj_mismatch_pixels"] +
                sum(region["mismatch_pixels"]
                    for region in result["regions"].values()))
    if failures:
        result["status"] = "FAIL"
    if args.report:
        Path(args.report).write_text(json.dumps(result, indent=2) + "\n")
    if failures:
        raise AssertionError(json.dumps(result, indent=2))
    print("Runtime PPU inputs PASS: 54,688 background and 182 goal OBJ pixels match Mesen")


if __name__ == "__main__":
    main()
