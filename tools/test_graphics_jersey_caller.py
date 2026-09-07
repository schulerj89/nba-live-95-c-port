"""Exercise the production Tipoff renderer caller for the jersey appender."""
from __future__ import annotations

import argparse
import subprocess
from pathlib import Path


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", type=Path, required=True)
    parser.add_argument("--pack", type=Path, required=True)
    args = parser.parse_args()
    result = subprocess.run([str(args.probe.resolve()), str(args.pack.resolve())],
                            text=True, capture_output=True)
    if result.returncode or "graphics-jersey-caller-ok" not in result.stdout:
        print(result.stdout, end="")
        print(result.stderr, end="")
        raise SystemExit(result.returncode or 1)
    print("[GRAPHICS JERSEY CALLER] PASS: miss/hit/direction/rollover")


if __name__ == "__main__":
    main()
