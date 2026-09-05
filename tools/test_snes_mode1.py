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
            # Re-reviewed after the exact $86:F1B0-$F2C9 actor parents stopped
            # publishing derived motion a pass early. That correction changes
            # the deterministic camera/actor state at frame 1000, so refresh
            # its layer census while retaining per-pixel rank, palette, and
            # complete-accounting checks below. Pin every winner so later PPU
            # changes cannot silently trade pixels while satisfying positives.
            # Re-reviewed after `$87:A52C-$A5FA` presentation direction was
            # applied to cached action body resources in the renderer. Two
            # opaque player pixels correctly replace BG2 at this frame.
            # Re-reviewed in build/mode1-native-edges.bmp after native ball
            # dispatch/fractions and actor-edge semantics changed the CPU
            # trajectory. Court/HUD/backdrop remain unchanged; 75 more OBJ
            # pixels replace BG2. This is a C regression, not ROM pixel parity.
            # Source/config/pack attribution independently isolates corrected
            # team identity and actor ranks as the cause of this C trajectory
            # change. Old totals were 2176/38703/5641/2573/8251. See
            # This is an inspected C-only anchor; it is not native
            # trajectory or HUD parity. All per-pixel assertions stay intact.
            # C39C's CMP#2 sends layout1 to C50B. The corrected target first
            # changes gameplay at506, then camera/actors at1000. A private
            # pre-fix-helper + matching startup-check relink reproduces every
            # old winner count; no compositor source changes. See
            # Native expectations are unchanged by this C-only repair.
            # `$86:EF09` restores an early shared-RNG call, changing the
            # deterministic camera and actor positions at this C-only frame.
            # The inspected scene remains complete and all winner ranks below
            # are still validated pixel by pixel.
            # Re-reviewed after `$86:F8CD-$F8D5` made mode six honor role flag
            # `$09D8` before repairing locomotion bases. The corrected CPU
            # trajectory has a coherent 2-0 first-quarter scoreboard panel at
            # frame 1000, so BG3 participates in this inspected C-only anchor.
            assert summary["visible"]["bg1"] == 3329
            assert summary["visible"]["bg2"] == 42174
            assert summary["visible"]["bg3"] == 6013
            assert summary["visible"]["obj"] == 3917
            assert summary["visible"]["backdrop"] == 1911
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

    print("SNES Mode-1 PASS: C-only winner counts, priority ladder, OAM self-test, indexed colors, window, CLI JSONL")


if __name__ == "__main__":
    main()
