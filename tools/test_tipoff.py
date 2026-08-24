"""Regression checks for Starting Lineups -> center-court jump ball."""

import argparse
import hashlib
import struct
import subprocess
import tempfile
from pathlib import Path

from PIL import Image

EXPECTED_ASSETS = {
    262: (56, 8, 8, 0x0D9C27,
          "a74d28ab12d3bfc35d1c6f4dacc3b7d2b962d7a11f5b7116edc122a17d19ba69"),
    263: (229376, 256, 224, 0,
          "8a012ead8fdf95d0cf23b0eef70e9a552d866eaacf0a46e0603c44ab9098831e"),
}
EXPECTED_FRAMES = {
    90: ("TIP PH:FORMATION", "f1864c5cab39fa8615db073565ecbf029bb8d2f601802b8cc147ca972941496b"),
    170: ("TIP PH:JUMP BALL", "e987d14e15586c4f69d49c7e130343ee88be6d91f00692b8c8a4efb1626a9673"),
    220: ("TIP PH:LIVE", "42ae39af55deb554dcff8e4e8aef7a1dd01d4d8a5736b6e221b798a189208d16"),
}


def pack_assets(path):
    raw = path.read_bytes()
    if raw[:8] != b"NBA95PAK":
        raise AssertionError("invalid pack magic")
    version, count = struct.unpack_from("<II", raw, 8)
    if version != 22 or 16 + count * 24 > len(raw):
        raise AssertionError("invalid tip-off pack version/directory")
    assets = {}
    for index in range(count):
        asset_id, offset, size, width, height, flags = struct.unpack_from(
            "<6I", raw, 16 + index * 24)
        assets[asset_id] = (raw[offset:offset + size], width, height, flags)
    return assets


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--pack", required=True)
    parser.add_argument("--exe", required=True)
    parser.add_argument("--rom", required=True)
    args = parser.parse_args()
    assets = pack_assets(Path(args.pack))
    for asset_id, expected in EXPECTED_ASSETS.items():
        payload, width, height, flags = assets[asset_id]
        size, expected_width, expected_height, expected_flags, digest = expected
        if (len(payload), width, height, flags) != (
                size, expected_width, expected_height, expected_flags):
            raise AssertionError(f"tip-off asset {asset_id} metadata changed")
        if hashlib.sha256(payload).hexdigest() != digest:
            raise AssertionError(f"tip-off asset {asset_id} payload changed")
    if assets[262][0][:12] != b"NBBALL1\0\x01\0\0\0":
        raise AssertionError("tip-off ball schema changed")
    courts, width, height, flags = assets[272]
    frame_size = 256 * 224 * 4
    if courts[:8] != b"NBCOURT1" or \
            struct.unpack_from("<IIII", courts, 8) != (1, 29, 256, 224) or \
            (width, height, flags, len(courts)) != \
            (256, 224, 29, 24 + 29 * frame_size):
        raise AssertionError("gameplay home-court catalog changed")
    if hashlib.sha256(courts[24 + 18 * frame_size:
                              24 + 19 * frame_size]).hexdigest() != \
            EXPECTED_ASSETS[263][4]:
        raise AssertionError("Orlando gameplay court no longer matches its oracle")
    if len({hashlib.sha256(courts[24 + team * frame_size:
                                  24 + (team + 1) * frame_size]).digest()
            for team in range(29)}) < 27:
        raise AssertionError("gameplay home courts lost ROM-selected variation")

    with tempfile.TemporaryDirectory() as directory:
        for frame, (phase, expected_hash) in EXPECTED_FRAMES.items():
            output = Path(directory) / f"tipoff_{frame:04d}.bmp"
            result = subprocess.run([
                args.exe, "--headless", "--rom", args.rom, "--assets", args.pack,
                "--tipoff-only", "--frames", str(frame), "--dump-frame", output,
                "--debug-state",
            ], capture_output=True, text=True, check=False)
            if result.returncode or phase not in result.stdout or \
                    "INT:$85:9700" not in result.stdout or \
                    "BALL M:" not in result.stdout:
                raise AssertionError(result.stdout + result.stderr)
            digest = hashlib.sha256(Image.open(output).convert("RGB").tobytes()).hexdigest()
            if digest != expected_hash:
                raise AssertionError(f"tip-off frame {frame} changed: {digest}")

        result = subprocess.run([
            args.exe, "--headless", "--rom", args.rom, "--assets", args.pack,
            "--player-setup-only", "--player-setup-confirm", "--frames", "5330",
            "--debug-state",
        ], capture_output=True, text=True, check=False)
        if result.returncode or "SCN:TIPOFF" not in result.stdout:
            raise AssertionError("final lineup card did not hand off to tip-off\n" +
                                 result.stdout + result.stderr)

        selected_home = Path(directory) / "san_antonio_tipoff.bmp"
        result = subprocess.run([
            args.exe, "--headless", "--rom", args.rom, "--assets", args.pack,
            "--team-only", "--team-right", "5", "--team-confirm",
            "--player-setup-confirm", "--frames", "5900",
            "--dump-frame", selected_home, "--debug-state",
        ], capture_output=True, text=True, check=False)
        digest = hashlib.sha256(
            Image.open(selected_home).convert("RGB").tobytes()).hexdigest() \
            if selected_home.exists() else ""
        if result.returncode or "SCN:TIPOFF" not in result.stdout or \
                digest != "763708f398a25a5ce97ef407cf32ec4f14966e83bdc0977405eabb769d5c2d2d":
            raise AssertionError("selected home court did not persist into tip-off\n" +
                                 result.stdout + result.stderr)

    source = Path(__file__).parents[1] / "src" / "nba_tipoff.c"
    text = source.read_text()
    for value in ("NBA_TIPOFF_BALL_APPEAR_FRAME", "NBA_TIPOFF_TOSS_FRAME",
                  "NBA_TIPOFF_CONTACT_FRAME",
                  "nba_player_sprite_render", "nba_assets_gameplay_home_court",
                  "visible_submission[8]"):
        if value not in text:
            raise AssertionError(f"tip-off implementation lost {value}")
    print("Tip-off regression checks passed")


if __name__ == "__main__":
    main()
