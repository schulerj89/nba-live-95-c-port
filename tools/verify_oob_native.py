"""Compare production C HUD children with independently captured Mesen states."""
import argparse
import json
from pathlib import Path
import struct
import subprocess

import numpy as np
from PIL import Image

from mesen_portable import verify
from oob_visual import decode
from regenerate_oob_reference import ROM_SHA256, sha

MESEN_SHA256 = "d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("native", "rom", "pack", "probe", "output"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    native = args.native
    manifest = json.loads((native / "manifest.json").read_text())
    if sha(args.rom) != ROM_SHA256 or manifest["rom_sha256"] != ROM_SHA256 or \
            manifest["mesen_sha256"] != MESEN_SHA256 or manifest["exit_code"]:
        raise ValueError("wrong native ROM/runtime or incomplete capture")
    if sha(native / "portable-mesen/Mesen.exe") != MESEN_SHA256:
        raise ValueError("captured Mesen executable changed")
    if sha(native / "capture.lua") != manifest["script_sha256"]:
        raise ValueError("executed capture script changed")
    verify(native, manifest["isolation"])
    for name, digest in manifest["artifacts"].items():
        if sha(native / name) != digest:
            raise ValueError("native evidence changed: " + name)
    calls = manifest["calls"]
    if [c["pc"] for c in calls] != [0x83DA12, 0x83DA8C, 0x83EBDB]:
        raise ValueError("missing native OOB lifecycle")
    if manifest["state_injection"]:
        scenario = manifest["scenario"]
        base = 0x34EB + scenario["owner"] * 0x100
        if scenario["pc"] != 0x8792A5 or [(w["address"], w["after"]) for w in scenario["writes"]] != \
                [(base + 4, 0), (base + 8, 209), (base + 12, 0)]:
            raise ValueError("unexpected scenario writes")
    # Only entry buffers enter the executable. Expected outputs are read later.
    data = bytearray(struct.pack("<I", len(calls)))
    for call in calls:
        data += struct.pack("<I", call["pc"])
        for suffix, size in (("wram", 131072), ("vram", 65536), ("cgram", 512)):
            raw = (native / (call["name"] + "_before." + suffix)).read_bytes()
            if len(raw) != size:
                raise ValueError("truncated entry buffer")
            data += raw
    (out / "input.bin").write_bytes(data)
    command = [str(args.probe.resolve()), str(args.pack.resolve()), str(out / "input.bin"), str(out / "actual.bin")]
    run = subprocess.run(command, capture_output=True, text=True, timeout=30,
                         creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
    (out / "probe.log").write_text(run.stdout + run.stderr)
    if run.returncode:
        raise AssertionError("native adapter failed")
    data = (out / "actual.bin").read_bytes()
    stride = 8 + 0x30200
    if len(data) != 4 + len(calls) * stride or struct.unpack_from("<I", data)[0] != len(calls):
        raise ValueError("truncated adapter output")
    comparisons, actual_vram = [], []
    for i, call in enumerate(calls):
        start = 4 + i * stride
        if struct.unpack_from("<II", data, start) != (1, 0):
            raise AssertionError("child was not completely translated")
        raw = data[start + 8:start + 8 + 0x20000]
        expected = (native / (call["name"] + "_after.wram")).read_bytes()
        for label, first, last in (("map", 0x4A70, 0x5070), ("characters", 0x5070, 0x6130),
                                   ("sequence", 0x8E6, 0x8E8), ("clear", 0x8EE, 0x8F0)):
            bad = sum(a != b for a, b in zip(raw[first:last], expected[first:last]))
            comparisons.append({"pc": call["pc"], "buffer": label, "bytes": last - first,
                                "different_bytes": bad})
        actual_vram.append(data[start + 8 + 0x20000:start + 8 + 0x30000])
    stable = next(r for r in manifest["frames"] if r["court"] >= calls[1]["court"] + 12)
    visible = (native / (stable["name"] + ".vram")).read_bytes()
    retired = (native / (manifest["frames"][-1]["name"] + ".vram")).read_bytes()
    for label, actual, expected, first, last in (
            ("visible map", actual_vram[1], visible, 0x9C0, 0xB40),
            ("visible characters", actual_vram[1], visible, 0x2470, 0x2EF0),
            ("retired map", actual_vram[2], retired, 0x800, 0xF00)):
        comparisons.append({"buffer": label, "bytes": last - first,
                            "different_bytes": sum(a != b for a, b in zip(actual[first:last], expected[first:last]))})
    cgram = (native / (stable["name"] + ".cgram")).read_bytes()
    rgb, mask = decode(actual_vram[1], cgram)
    displayed = []
    for row in manifest["frames"]:
        path = native / (row["name"] + ".png")
        if sha(path) != row["sha256"]:
            raise ValueError("native captured frame changed")
        image = np.asarray(Image.open(path).convert("RGB"))
        if np.array_equal(image[mask], rgb[mask]):
            displayed.append(row["court"])
    if len(displayed) < 10 or displayed[0] < calls[1]["court"] or \
            displayed[-1] >= manifest["frames"][-1]["court"]:
        raise AssertionError("missing native appearance/hold/retirement evidence")
    for row in manifest["frames"]:
        if displayed[0] <= row["court"] <= displayed[-1] and row["court"] not in displayed:
            raise AssertionError("native text pixels changed during display")
    failures = sum(c["different_bytes"] for c in comparisons)
    report = {"status": "FAIL" if failures else "PASS", "native_manifest_sha256": sha(native / "manifest.json"),
              "probe_sha256": sha(args.probe), "pack_sha256": sha(args.pack), "command": command,
              "comparisons": comparisons, "different_bytes": failures,
              "native_frames_checked": len(manifest["frames"]), "displayed_frames": displayed,
              "text_pixels_per_frame": int(mask.sum()), "state_injection": manifest["state_injection"],
              "scope": "original child-owned buffers and published HUD pixels; asynchronous DMA latency is not emulated"}
    (out / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({k: report[k] for k in ("status", "different_bytes", "native_frames_checked", "text_pixels_per_frame")}))
    if failures:
        raise AssertionError("C differs from the original HUD")


if __name__ == "__main__":
    main()
