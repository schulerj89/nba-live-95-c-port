"""Run the production-linked canonical WRAM lifetime probe."""
import argparse
import subprocess
from pathlib import Path


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--probe", type=Path, required=True)
    parser.add_argument("--pack", type=Path, required=True)
    args = parser.parse_args()
    result = subprocess.run(
        [str(args.probe.resolve()), str(args.pack.resolve())],
        check=True, capture_output=True, text=True)
    if "game-wram-lifetime-ok" not in result.stdout:
        raise AssertionError("lifetime probe completion marker missing")
    print("game WRAM lifetime probe passed")


if __name__ == "__main__":
    main()
