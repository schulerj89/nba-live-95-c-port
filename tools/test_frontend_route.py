"""Production-route regressions for the source-backed frontend fixes."""

import argparse
import hashlib
import subprocess
import tempfile
from pathlib import Path

from PIL import Image, ImageChops


EXPECTED_TEAM_TO_PLAYER_HASHES = {
    178: "872f67151f1dc4fed9bca5fe54cb669861b105a197ac87b319e735437ecd2629",
    179: "1fa98c6e9f9ceca135cf1939c63c87a5e7d77a67513fa9bfaea4dff6ffe41092",
    228: "b36923413e6aa122e7e0b55adbe9b8adad033ab809acdbb630b8f0ab0adc178e",
    229: "2cbbeef1249170a43854962fa5b19fba628470c70beb9ce23e15a0f05cb891f2",
    295: "2cbbeef1249170a43854962fa5b19fba628470c70beb9ce23e15a0f05cb891f2",
    296: "573745bd2147d1f0b2e43eb573860a935845b09f9a325d8997de575c2d9fc1a4",
    297: "65a5856464736ec1a123f831353546e577907ac9a4d28735ca7539c71ed2fccc",
}

EXPECTED_SKIP_TO_GAME_HASHES = {
    586: "dc7bf300d70031c7b493aa9d549f366b9bc9a76ea6b0e10475a4c79c97af0e95",
    587: "4045756ff167bce15069b5b0b9f6a84c4a23e44a44c9f1d86ce04d33cc45f2e2",
    589: "24a5d44e073be6b3896cbdd477c68ac2b4ec60dc4bac477b1b23710a0d5a1ea5",
    591: "d292e116138b92e3f15d7f6155be73dd6f1617d58ddf39c390249d84020ec2ed",
    593: "24a5d44e073be6b3896cbdd477c68ac2b4ec60dc4bac477b1b23710a0d5a1ea5",
    595: "2cbbeef1249170a43854962fa5b19fba628470c70beb9ce23e15a0f05cb891f2",
    596: "741f18f3ef851294bbf67e4336a57d8a90354d00e1c6652264cc6580179a956f",
    600: "5f7420dc66097ca5252883d745865e5533812b04245916b212437bd5ac9ae8a8",
}


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


def rgb_hash(path):
    return hashlib.sha256(Image.open(path).convert("RGB").tobytes()).hexdigest()


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
        skip_sequence = directory / "skip_sequence"
        skip_sequence.mkdir()
        complete = run(exe, *base, "--frames", 620,
                       "--dump-sequence-from", 586,
                       "--dump-sequence-dir", skip_sequence, "--debug-state")
        if "SCN:TIPOFF" not in complete:
            raise AssertionError("Start presentation skips did not reach Tipoff:\n" + complete)
        for frame, expected_hash in EXPECTED_SKIP_TO_GAME_HASHES.items():
            if rgb_hash(skip_sequence / f"frame_{frame:04d}.bmp") != expected_hash:
                raise AssertionError(
                    f"presentation-skip -> Tipoff rendered frame {frame} changed")

        sequence = directory / "sequence"
        sequence.mkdir()
        run(exe, "--headless", "--rom", rom, "--assets", pack,
            "--team-only", "--team-confirm", "--frames", 300,
            "--dump-sequence-from", 160, "--dump-sequence-dir", sequence)
        frames = {number: Image.open(sequence / f"frame_{number:04d}.bmp")
                  for number in (178, 179, 200, 228, 229, 295, 296)}
        if changed(frames[178], frames[179]) < 10000:
            raise AssertionError("outgoing Team Select layers did not begin moving")
        if nonblack(frames[200]) == 0 or nonblack(frames[228]) == 0:
            raise AssertionError("outgoing layer owner ended before native +51 boundary")
        # Native Start frame 650 keeps the outgoing owner through 700, holds
        # exactly 67 fully-black frames 701..767, and first reveals Player
        # Setup at 768.  The port route starts at 178, so 229..295 and 296 are
        # the corresponding construction/reveal boundaries.
        if nonblack(frames[229]) != 0 or nonblack(frames[295]) != 0:
            raise AssertionError("forced-black construction interval changed")
        if nonblack(frames[296]) == 0:
            raise AssertionError("Player Setup reveal did not begin at destination boundary")
        for frame, expected_hash in EXPECTED_TEAM_TO_PLAYER_HASHES.items():
            if rgb_hash(sequence / f"frame_{frame:04d}.bmp") != expected_hash:
                raise AssertionError(
                    f"Team Select -> Player Setup rendered frame {frame} changed")

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
