"""Production-route regressions for the source-backed frontend fixes."""

import argparse
import subprocess
import tempfile
from pathlib import Path

from PIL import Image, ImageChops


def run(exe, *args):
    result = subprocess.run([str(exe), *map(str, args)], text=True,
                            capture_output=True, check=False)
    if result.returncode:
        raise AssertionError(result.stdout + result.stderr)
    return result.stdout


def nonblack(image):
    return sum(pixel != (0, 0, 0) for pixel in image.convert("RGB").getdata())


def changed(left, right):
    return nonblack(ImageChops.difference(left.convert("RGB"),
                                          right.convert("RGB")))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--pack", required=True)
    parser.add_argument("--exe", required=True)
    parser.add_argument("--rom", required=True)
    args = parser.parse_args()
    pack, exe, rom = Path(args.pack), Path(args.exe), Path(args.rom)

    # Direct Team Select entry is a production NbaGame caller, not a seeded
    # Player Setup/Introduction state. Left centers controller one after the
    # handoff; Start then skips each source-owned presentation as a new edge.
    script_rows = (
        "200 0000\n1 0200\n1 0000\n1 1000\n200 0000\n"
        "1 0200\n1 0000\n1 1000\n180 0000\n"
        "1 1000\n1 0000\n1 1000\n1 0000\n"
        "1 0100\n1 0000\n1 0200\n1 0000\n1 1000\n20 0000\n"
    )
    with tempfile.TemporaryDirectory() as directory_name:
        directory = Path(directory_name)
        script = directory / "neutral_to_tipoff.input"
        script.write_text(script_rows, encoding="ascii")
        base = ["--headless", "--rom", rom, "--assets", pack,
                "--team-only", "--input-script", script]
        centered = run(exe, *base, "--frames", 405, "--debug-state")
        if "[PLAYER SETUP TEST] p1=NEUTRAL" not in centered or \
                "SCN:PLAYER_SETUP" not in centered:
            raise AssertionError("centered CPU-vs-CPU selection was not retained:\n" + centered)
        complete = run(exe, *base, "--frames", 620, "--debug-state")
        if "SCN:TIPOFF" not in complete:
            raise AssertionError("Start presentation skips did not reach Tipoff:\n" + complete)

        sequence = directory / "sequence"
        sequence.mkdir()
        run(exe, "--headless", "--rom", rom, "--assets", pack,
            "--team-only", "--team-confirm", "--frames", 300,
            "--dump-sequence-from", 160, "--dump-sequence-dir", sequence)
        frames = {number: Image.open(sequence / f"frame_{number:04d}.bmp")
                  for number in (178, 179, 200, 228, 229, 298, 299)}
        if changed(frames[178], frames[179]) < 10000:
            raise AssertionError("outgoing Team Select layers did not begin moving")
        if nonblack(frames[200]) == 0 or nonblack(frames[228]) == 0:
            raise AssertionError("outgoing layer owner ended before native +51 boundary")
        if nonblack(frames[229]) != 0 or nonblack(frames[298]) != 0:
            raise AssertionError("forced-black construction interval changed")
        if nonblack(frames[299]) == 0:
            raise AssertionError("Player Setup reveal did not begin at destination boundary")

    source = (Path(__file__).parents[1] / "src" / "nba_player_intro.c").read_text(
        encoding="utf-8")
    for address in ("$83:F277-$F2BA", "$83:F5F0-$F63C",
                    "$83:F7D0-$F830", "$87:BF5E-$BFA6"):
        if address not in source:
            raise AssertionError(f"missing source provenance {address}")
    if "NBA_BTN_A" in source[source.index("void nba_player_intro_update"):
                              source.index("static void draw_asset_rgba")]:
        raise AssertionError("lineup retained the non-source A-card shortcut")

    print("[TEST] PASS: layer exit, centered CPU-vs-CPU, and Start presentation skips")


if __name__ == "__main__":
    main()
