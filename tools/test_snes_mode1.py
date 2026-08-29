"""Mode-1 compositor priority, provenance, and CLI trace regression."""

import argparse
import json
import subprocess
import tempfile
from pathlib import Path


EXPECTED_RANKS = {
    ("BACKDROP", 0): 0, ("BG3", 0): 1, ("OBJ", 0): 2,
    ("OBJ", 1): 4, ("BG2", 0): 5, ("BG1", 0): 6,
    ("OBJ", 2): 7, ("BG2", 1): 8, ("BG1", 1): 9,
    ("OBJ", 3): 10, ("BG3", 1): 11,
}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", required=True)
    parser.add_argument("--rom", required=True)
    parser.add_argument("--pack", required=True)
    args = parser.parse_args()

    with tempfile.TemporaryDirectory() as temporary:
        trace = Path(temporary) / "ppu.jsonl"
        result = subprocess.run(
            [args.exe, "--headless", "--rom", args.rom, "--assets", args.pack,
             "--tipoff-only", "--frames", "1000", "--ppu-trace", str(trace)],
            text=True, capture_output=True, check=False,
        )
        if result.returncode:
            raise AssertionError(result.stdout + result.stderr)
        assert "[PPU TRACE] BG1=" in result.stdout

        with trace.open() as stream:
            summary = json.loads(next(stream))
            assert summary["type"] == "summary"
            assert summary["state_frame"] == 1000
            assert summary["visible"]["bg1"] > 0
            assert summary["visible"]["bg2"] > 0
            assert summary["visible"]["bg3"] > 0
            assert summary["visible"]["obj"] > 0
            assert summary["visible"]["backdrop"] > 0
            counts = {name: 0 for name in
                      ("BACKDROP", "BG1", "BG2", "BG3", "OBJ")}
            indexed = direct = rows = 0
            for line in stream:
                pixel = json.loads(line)
                rows += 1
                layer = pixel["layer"]
                counts[layer] += 1
                assert pixel["rank"] == EXPECTED_RANKS[
                    (layer, pixel["priority"])]
                if pixel["palette"] == 255 and pixel["color"] == 255:
                    direct += 1
                else:
                    indexed += 1
                    if layer == "BACKDROP":
                        assert pixel["palette"] == 0 and pixel["color"] == 0
                    elif layer == "BG3":
                        assert 0 < pixel["color"] < 4
                    else:
                        assert 0 < pixel["color"] < 16
            assert rows == 256 * 224
            assert counts["BG1"] == summary["visible"]["bg1"]
            assert counts["BG2"] == summary["visible"]["bg2"]
            assert counts["BG3"] == summary["visible"]["bg3"]
            assert counts["OBJ"] == summary["visible"]["obj"]
            assert counts["BACKDROP"] == summary["visible"]["backdrop"]
            assert indexed > 0 and direct > 0

    print("SNES Mode-1 PASS: priority ladder, OAM self-test, indexed colors, window, CLI JSONL")


if __name__ == "__main__":
    main()
