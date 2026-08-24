"""Regression checks for Player Setup -> Starting Lineup presentation."""

import argparse
import hashlib
import struct
import subprocess
import tempfile
from pathlib import Path

from PIL import Image

EXPECTED_ASSETS = {
    260: (229376, "372c50ea64dc4180637c1666d123d3c018012a5c65b31823fc943be28358440b"),
    261: (6015784, "91120473949026d6803083ceb70bcc4e84623baa49151d10b7e6846df16ea14c"),
    264: (6144, "7888790592673fd5e9fb1f76d7c50344fb8f6853d7d21594aad9a9c588e73d0b"),
}


def load_pack(path):
    raw = path.read_bytes()
    if raw[:8] != b"NBA95PAK":
        raise AssertionError("invalid pack magic")
    version, count = struct.unpack_from("<II", raw, 8)
    if version != 19 or 16 + count * 24 > len(raw):
        raise AssertionError("invalid asset directory")
    assets = {}
    for index in range(count):
        asset_id, offset, size, width, height, flags = struct.unpack_from(
            "<6I", raw, 16 + index * 24)
        assets[asset_id] = (raw[offset:offset + size], width, height, flags)
    return assets


def run(exe, *args):
    result = subprocess.run([str(exe), *map(str, args)], text=True,
                            capture_output=True, check=False)
    if result.returncode:
        raise AssertionError(result.stdout + result.stderr)
    return result.stdout


def rgb_hash(path):
    return hashlib.sha256(Image.open(path).convert("RGB").tobytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--pack", required=True)
    parser.add_argument("--exe", required=True)
    parser.add_argument("--rom", required=True)
    args = parser.parse_args()
    pack, exe, rom = Path(args.pack), Path(args.exe), Path(args.rom)
    assets = load_pack(pack)

    for asset_id, (size, digest) in EXPECTED_ASSETS.items():
        payload, width, height, flags = assets[asset_id]
        expected_dimensions = {260: (256, 224), 261: (72, 72), 264: (16, 16)}[asset_id]
        if len(payload) != size or (width, height) != expected_dimensions:
            raise AssertionError(f"Player Introduction asset {asset_id} metadata changed")
        if flags != {260: 0, 261: 290, 264: 6}[asset_id]:
            raise AssertionError(f"Player Introduction asset {asset_id} flags changed")
        if hashlib.sha256(payload).hexdigest() != digest:
            raise AssertionError(f"Player Introduction asset {asset_id} payload changed")

    portrait = assets[261][0]
    if portrait[:8] != b"NBINTRO1" or struct.unpack_from("<II", portrait, 8) != (2, 290):
        raise AssertionError("portrait catalog header/count changed")
    offset, keys = 24, []
    for _ in range(290):
        team, slot, side, size = struct.unpack_from("<BBHI", portrait, offset)
        keys.append((side, team, slot))
        offset += 8 + size
    if keys != [(side, team, slot) for side in range(2)
                for team in range(29) for slot in range(5)]:
        raise AssertionError("lineup portrait team/slot ordering changed")

    with tempfile.TemporaryDirectory() as directory:
        matchup = Path(directory) / "matchup.bmp"
        run(exe, "--headless", "--rom", rom, "--assets", pack,
            "--player-setup-only", "--player-setup-confirm",
            "--frames", 500, "--dump-frame", matchup)
        if rgb_hash(matchup) != "6a67f9baa1b4f8d350cb184371db8133ebc4cafe4e111dfcc3289c0fdd1ca1bd":
            raise AssertionError("ROM-layout visitor/VS/home presentation changed")

        ratings = Path(directory) / "ratings.bmp"
        ratings_next = Path(directory) / "ratings_next.bmp"
        run(exe, "--headless", "--rom", rom, "--assets", pack,
            "--player-setup-only", "--player-setup-confirm",
            "--frames", 800, "--dump-frame", ratings)
        run(exe, "--headless", "--rom", rom, "--assets", pack,
            "--player-setup-only", "--player-setup-confirm",
            "--frames", 812, "--dump-frame", ratings_next)
        if rgb_hash(ratings) != "98342ebcc2bc04490fc34f4b34f5e56d53f9664cc88578bf8eab9be4c8a75616" or \
           rgb_hash(ratings_next) != "b93d5129aec0c07e42e54dcbd1eaecda4565421a64698ab2db61b22f5d8446ab":
            raise AssertionError("rating-ball thresholds, placement, or 12-frame animation changed")

        frame = Path(directory) / "lineup.bmp"
        output = run(exe, "--headless", "--rom", rom, "--assets", pack,
                     "--player-setup-only", "--player-setup-confirm",
                     "--frames", 1100, "--dump-frame", frame, "--debug-state")
        if "SCN:PLAYER_INTRO" not in output or "CARD:01/10" not in output or \
           "ROM LOOP:$87:BE92" not in output:
            raise AssertionError("Player Setup did not hand off to the lineup state")
        if rgb_hash(frame) != "ba775d7945cfa26dafdb9042ef3e0a0763eca0319abdc586fa2bff19c9688e78":
            raise AssertionError("first Starting Lineup frame changed")

        output = run(exe, "--headless", "--rom", rom, "--assets", pack,
                     "--player-setup-only", "--player-setup-confirm",
                     "--frames", 3270, "--debug-state")
        if "CARD:06/10" not in output:
            raise AssertionError("visitor-to-home lineup boundary changed")

        output = run(exe, "--headless", "--rom", rom, "--assets", pack,
                     "--player-setup-only", "--player-setup-confirm",
                     "--frames", 5006, "--debug-state")
        if "CARD:10/10" not in output:
            raise AssertionError("final home starter cadence changed")

        away_frame = Path(directory) / "golden_state_away.bmp"
        output = run(exe, "--headless", "--rom", rom, "--assets", pack,
                     "--team-only", "--team-side-toggle", "--team-right", 5,
                     "--team-confirm", "--player-setup-confirm", "--frames", 1600,
                     "--dump-frame", away_frame, "--debug-state")
        if "TEAM L:08 R:18" not in output or "CARD:01/10" not in output or \
           rgb_hash(away_frame) != "3692ecb800b94532d428548115862225ee3885044af19529357382e0cb151ba7":
            raise AssertionError("non-default visitor portrait selection changed")

        home_frame = Path(directory) / "san_antonio_home.bmp"
        output = run(exe, "--headless", "--rom", rom, "--assets", pack,
                     "--team-only", "--team-right", 5, "--team-confirm",
                     "--player-setup-confirm", "--frames", 3600,
                     "--dump-frame", home_frame, "--debug-state")
        if "TEAM L:03 R:23" not in output or "CARD:06/10" not in output or \
           rgb_hash(home_frame) != "7561ce66fdfc393b54233e34fe0c53533ce36ccf1ff508837216db9b11e625d9":
            raise AssertionError("non-default home portrait selection changed")

    print("Player Introduction regression checks passed")


if __name__ == "__main__":
    main()
