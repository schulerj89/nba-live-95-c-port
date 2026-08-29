"""Regression checks for checked-in ROM census and feature matrix artifacts."""

import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def main():
    subprocess.run([sys.executable, str(ROOT / "tools/feature_capture_matrix.py")],
                   cwd=ROOT, check=True)
    matrix = json.loads((ROOT / "docs/feature-capture-matrix.json").read_text())
    require(sum(row["weight"] for row in matrix["features"]) == 100,
            "feature weights must total 100")
    require(len(matrix["features"]) >= 10, "whole-game matrix is unexpectedly narrow")

    census = json.loads((ROOT / "docs/full-rom-instruction-census.json").read_text())
    require(census["schema"] == 1, "unexpected census schema")
    require(len(census["banks"]) == 48, "census must describe all 48 physical banks")
    totals = census["totals"]
    require(totals["rom_bytes"] == 48 * 0x8000, "unexpected US ROM size")
    require(totals["decoded_starts"] > 0, "census decoded no instructions")
    require(totals["verified_starts"] <= totals["decoded_starts"],
            "verified starts exceed the decoded universe")
    require(totals["decoded_bytes"] + totals["undecoded_bytes"] == totals["rom_bytes"],
            "decoded and undecoded byte totals do not cover the ROM")
    print("project census regression: pass")


if __name__ == "__main__":
    main()
