"""Replay measured Mesen raster writes and compare native basket scanouts."""
import argparse
import json
from pathlib import Path
import subprocess

from PIL import Image
from regenerate_hoop_reference import ROM_SHA256, sha
from snes_ppu_oracle import parse_state, render_snapshot

ROOT = Path(__file__).resolve().parents[1]
FIXTURE_SHA256 = "db1b5c2985b64570ab482be6203929f5e6dfcb17c0466a421c9421973483ca4b"


def raster_rows(case):
    """Expected controls come from measured registers, not a camera formula."""
    tm, edge = case["initial_tm"], case["window"][1]
    source = edge
    events = sorted((e for e in case["writes"] if e[0] < 224), key=lambda e: e[0])
    index = 0
    for line in range(224):
        while index < len(events) and events[index][0] <= line:
            _, reg, value, _ = events[index]
            if reg == 0x212c:
                tm = value
            elif reg == 0x2129:
                edge = value
                source = case["narrow_source"]
                if source is None:
                    raise AssertionError("missing observed window-handler input")
            index += 1
        yield line, source, tm, edge


def verify_vectors(fixture, probe, frozen=True):
    fixture = Path(fixture)
    if frozen and sha(fixture) != FIXTURE_SHA256:
        raise AssertionError("frozen native raster fixture changed")
    data = json.loads(fixture.read_text())
    if data["rom_sha256"] != ROM_SHA256 or data["state_injection"]:
        raise AssertionError("wrong native raster provenance")
    requests, expected, labels = [], [], []
    for case in data["cases"]:
        for line, source, tm, edge in raster_rows(case):
            requests.append(f'{case["camera"][0]} {case["scroll"][1]} {source} {line}\n')
            expected.append([tm & 1, edge])
            labels.append((case["name"], line))
    run = subprocess.run([str(Path(probe).resolve())], input="".join(requests),
                         capture_output=True, text=True, timeout=15,
                         creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
    if run.returncode:
        raise AssertionError(run.stderr)
    actual = [[int(v) for v in line.split()] for line in run.stdout.splitlines()]
    if len(actual) != len(expected):
        raise AssertionError("raster probe dropped cases")
    for label, want, got in zip(labels, expected, actual):
        if want != got:
            raise AssertionError(f"native raster {label}: expected {want}, got {got}")
    return {"native_frames": len(data["cases"]), "scanlines": len(expected),
            "control_values": len(expected) * 2, "fixture_sha256": sha(fixture)}


def capture_fixture(directory, output):
    if Path(output).exists():
        raise ValueError("use a new --fixture path for a fresh native capture")
    directory = Path(directory)
    manifest = json.loads((directory / "manifest.json").read_text())
    if manifest["rom_sha256"] != ROM_SHA256 or manifest["exit_code"] or manifest["state_injection"]:
        raise AssertionError("invalid native capture")
    cases, results = [], []
    for row in manifest["frames"]:
        prefix = directory / row["name"]
        state = parse_state(prefix.with_suffix(".state"))
        case = {key: value for key, value in row.items() if key not in ("inputs", "sha256")}
        case["initial_tm"] = int(state["ppu.mainScreenLayers"])
        case["image_sha256"] = sha(prefix.with_suffix(".png"))
        for line, _, tm, edge in raster_rows(case):
            state[f"audit.mainScreenLayers[{line}]"] = str(tm)
            state[f"audit.window[1].right[{line}]"] = str(edge)
        rendered, winners = render_snapshot(prefix.with_suffix(".vram").read_bytes(),
            prefix.with_suffix(".cgram").read_bytes(), prefix.with_suffix(".oam").read_bytes(), state)
        native = list(Image.open(prefix.with_suffix(".png")).convert("RGB").getdata())
        failures = [i for i, (a, b) in enumerate(zip(rendered, native)) if a != b]
        Image.frombytes("RGB", (256, 224), bytes(c for pixel in rendered for c in pixel)).save(
            prefix.with_suffix(".oracle.png"))
        results.append({"name": row["name"], "pixels": len(native), "mismatch_pixels": len(failures),
                        "bg1_pixels": sum(w[1] == "BG1" for w in winners),
                        "first_mismatches": [[i % 256, i // 256] for i in failures[:8]]})
        cases.append(case)
        print(f'{row["name"]}: {len(failures)} native/oracle differences', flush=True)
    (directory / "pixel-report.json").write_text(json.dumps(results, indent=2) + "\n")
    if any(r["mismatch_pixels"] for r in results):
        raise AssertionError("native raster reconstruction differed; see pixel-report.json")
    fixture = {"schema": 1, "rom_sha256": ROM_SHA256, "state_injection": False,
               "mesen_sha256": manifest["mesen_sha256"], "script_sha256": manifest["script_sha256"],
               "capture_manifest_sha256": sha(directory / "manifest.json"),
               "routine_ranges": ["80:8410-84A0", "85:EEEE-EF39"],
               "cases": cases}
    Path(output).write_text(json.dumps(fixture, indent=2) + "\n", encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--native", type=Path)
    parser.add_argument("--fixture", type=Path, default=ROOT / "tests/fixtures/hoop-raster-native.json")
    parser.add_argument("--probe", type=Path, default=ROOT / "build/hoop_raster_probe.exe")
    args = parser.parse_args()
    if args.native:
        capture_fixture(args.native, args.fixture)
    print(json.dumps(verify_vectors(args.fixture, args.probe, frozen=not args.native)))


if __name__ == "__main__":
    main()
