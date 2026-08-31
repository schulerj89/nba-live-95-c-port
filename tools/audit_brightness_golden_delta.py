"""Audit conversion-only golden changes against a controlled native PPU table.

This is not a frame-parity test. Old images must match the specified pre-change
goldens. Every new pixel must then equal the captured native brightness mapping
for the unchanged C PPU brightness. No expected image is authored by the new C.
The report proposes hashes; it never edits a regression test.
"""
import argparse
import ast
import hashlib
import json
import re
import subprocess
from pathlib import Path

import numpy as np
from PIL import Image
from verify_ppu_brightness import capture


def sha(data):
    return hashlib.sha256(data).hexdigest()


def read_native(directory):
    raw = (directory / "brightness.jsonl").read_bytes()
    # Reuse the native protocol/hash validator, without invoking either C or
    # the Python converter. The mapping below reads only observed samples.
    rows, _, _, manifest = capture(directory)
    if len(rows) != 1536 or manifest["calls"] != 1536:
        raise ValueError("native capture must contain all 1536 channel/level/brightness cases")
    if manifest["isolation"]["method"] != "private portable executable/settings":
        raise ValueError("require the canonical isolated native capture")
    if not manifest["isolation"]["post_settings_verified"]:
        raise ValueError("native color settings were not verified")
    tables = np.full((16, 3, 256), -1, dtype=np.int16)
    seen = set()
    for row in rows:
        channel, level, brightness = (row[key] for key in ("channel", "level", "brightness"))
        if any(type(value) is not int for value in (channel, level, brightness)):
            raise ValueError("native input fields must be integer words")
        if not (0 <= channel < 3 and 0 <= level < 32 and 0 <= brightness < 16):
            raise ValueError("native input out of range")
        key = channel, level, brightness
        if key in seen:
            raise ValueError("duplicate native input")
        seen.add(key)
        if (row["color"] != level << (channel * 5) or
                row["cgram_word"] != row["color"] or
                row["ppu_brightness"] != brightness or row["forced_blank"] is not False or
                any(row[field] != 0 for field in ("main_layers", "sub_layers",
                    "conflicting_writes", "rejection_reason_bits"))):
            raise ValueError("native input/state mismatch")
        samples = row["samples"]
        if len(samples) != 5 or any(type(v) is not int or not 0 <= v <= 0xffffff
                                    for v in samples) or len(set(samples)) != 1:
            raise ValueError("invalid/nonuniform native samples")
        shift = (2 - channel) * 8  # CGRAM uses R/G/B; sampled RGB is 0xRRGGBB.
        if samples[0] & ~(255 << shift):
            raise ValueError("single native channel leaked into another output channel")
        native = (samples[0] >> shift) & 255
        # Only this *old* conversion is reconstructed. The desired conversion
        # comes from native pixels, never from the repaired C/Python renderer.
        old = ((level << 3) | (level >> 2)) * brightness // 15
        previous = int(tables[brightness, channel, old])
        if previous != -1 and previous != native:
            raise ValueError("old quantization is ambiguous; cannot authorize a pixel-only rebaseline")
        tables[brightness, channel, old] = native
    return tables, manifest, sha(raw)


def read_goldens(path):
    tree = ast.parse(path.read_text(encoding="utf-8-sig"))
    matches = [ast.literal_eval(node.value) for node in tree.body
               if isinstance(node, ast.Assign) and any(isinstance(target, ast.Name) and
                   target.id == "EXPECTED_RGB_SHA256" for target in node.targets)]
    if len(matches) != 1 or not matches[0]:
        raise ValueError("expected one nonempty initial-Setup golden dictionary")
    if set(matches[0]) != {104, 105, 118, 125, 128, 130, 146, 162, 166} or any(
            type(key) is not int or not isinstance(value, str) or
            not re.fullmatch("[0-9a-f]{64}", value) for key, value in matches[0].items()):
        raise ValueError("require the nine complete pre-change initial Setup goldens")
    return matches[0]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("before", "after", "rom", "pack", "native", "baseline-test", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    sources = {name: dict(path=str(getattr(args, name).resolve()),
                         sha256=sha(getattr(args, name).read_bytes()))
               for name in ("before", "after", "rom", "pack", "baseline_test")}
    tables, manifest, native_hash = read_native(args.native)
    if sha(args.rom.read_bytes()) != manifest["sources"]["rom"]["sha256"]:
        raise ValueError("game and native table must use the same ROM")
    goldens = read_goldens(args.baseline_test)
    results = []
    for frame, expected in goldens.items():
        pictures, states = [], []
        for label in ("before", "after"):
            destination = args.output / f"{label}-{frame:04d}.bmp"
            process = subprocess.run([str(getattr(args, label).resolve()), "--headless",
                "--setup-only", "--rom", str(args.rom.resolve()), "--assets", str(args.pack.resolve()),
                "--frames", str(frame), "--dump-frame", str(destination), "--debug-state"],
                check=True, text=True, capture_output=True, timeout=60)
            (args.output / f"{label}-{frame:04d}.log").write_text(process.stdout, encoding="utf-8")
            state = re.findall(r"PPU B:(\d+) X1:(-?\d+) X2:(-?\d+) Y2:(-?\d+) Y3:(-?\d+)", process.stdout)
            if len(state) != 1:
                raise ValueError(f"frame{frame}: missing/ambiguous C PPU telemetry")
            states.append(tuple(map(int, state[0])))
            picture = np.asarray(Image.open(destination).convert("RGB"))
            if picture.shape != (224, 256, 3):
                raise ValueError("unexpected C framebuffer dimensions")
            pictures.append(picture)
        if states[0] != states[1]:
            raise AssertionError(f"frame{frame}: C brightness/scroll state changed")
        before, after = pictures
        if sha(before.tobytes()) != expected:
            raise AssertionError(f"frame{frame}: old binary does not reproduce committed golden")
        mapped = np.stack([tables[states[0][0], channel, before[:, :, channel]]
                           for channel in range(3)], axis=2)
        if np.any(mapped < 0):
            raise AssertionError(f"frame{frame}: old pixel not covered by native brightness inputs")
        mismatches = np.any(mapped != after, axis=2)
        if np.any(mismatches):
            y, x = np.argwhere(mismatches)[0]
            raise AssertionError(f"frame{frame}: unauthorized pixel change at{x},{y}: "
                                 f"native mapping{mapped[y,x].tolist()} C{after[y,x].tolist()}")
        results.append(dict(frame=frame, brightness=states[0][0], before_sha256=expected,
                            after_sha256=sha(after.tobytes()),
                            changed_pixels=int(np.count_nonzero(np.any(before != after, axis=2))),
                            unauthorized_pixels=0))
    for name, identity in sources.items():
        if sha(getattr(args, name).read_bytes()) != identity["sha256"]:
            raise AssertionError(f"source changed during audit: {name}")
    report = dict(result="PASS", scope="conversion-only initial Setup golden delta",
                  exclusions=["whole-frame native parity", "native frame timing", "other transitions"],
                  native_raw_sha256=native_hash,
                  native_manifest_sha256=sha((args.native / "manifest.json").read_bytes()),
                  sources=sources, rows=results)
    (args.output / "report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"PASS: {len(results)} old goldens reproduced; every changed pixel follows native PPU samples")


if __name__ == "__main__":
    main()
