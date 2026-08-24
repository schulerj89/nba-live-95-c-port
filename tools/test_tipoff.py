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
          "f728c33e94f9266c36798975e5c8580868e237bb223494067893f31dd3c29d10"),
}
EXPECTED_FRAMES = {
    90: ("TIP PH:FORMATION", "5e78edca172620c2a89d327d900d7061b5161a56241d09374dacb91c1767df26"),
    170: ("TIP PH:JUMP BALL", "84330113b1285541dab7c9b6d9b11c84fa0460d025bc64d7e0c116813b7d404a"),
    220: ("TIP PH:LIVE", "3ae9e50fd4dd7536cdc7389975c125dcf2cbe250c11ab417521dc2e25d7f4099"),
}


def pack_assets(path):
    raw = path.read_bytes()
    if raw[:8] != b"NBA95PAK":
        raise AssertionError("invalid pack magic")
    version, count = struct.unpack_from("<II", raw, 8)
    if version != 23 or 16 + count * 24 > len(raw):
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
    panoramas, width, height, flags = assets[273]
    panorama_size = 912 * 416 * 4
    if panoramas[:8] != b"NBCOURT2" or \
            struct.unpack_from("<IIII", panoramas, 8) != (1, 29, 912, 416) or \
            (width, height, flags, len(panoramas)) != \
            (912, 416, 29, 24 + 29 * panorama_size):
        raise AssertionError("complete ROM court panorama catalog changed")
    if hashlib.sha256(panoramas[24 + 18 * panorama_size:
                                24 + 19 * panorama_size]).hexdigest() != \
            "f6324c6ca875ad636c4ba77b74df96e4f1a67c001404cc9040d2306409ba6cf5":
        raise AssertionError("Orlando ROM court panorama changed")

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
                digest != "66b743433c73d9e5b70efd73d3b0750ccfbf1e498adf0653e8bd86d6dcf6b57d":
            raise AssertionError("selected home court did not persist into tip-off\n" +
                                 result.stdout + result.stderr)

    source = Path(__file__).parents[1] / "src" / "nba_tipoff.c"
    text = source.read_text()
    for value in ("NBA_TIPOFF_BALL_APPEAR_FRAME", "NBA_TIPOFF_TOSS_FRAME",
                  "NBA_TIPOFF_CONTACT_FRAME",
                  "nba_player_sprite_render", "nba_assets_gameplay_court_panorama",
                  "visible_submission[8]"):
        if value not in text:
            raise AssertionError(f"tip-off implementation lost {value}")
    print("Tip-off regression checks passed")


if __name__ == "__main__":
    main()
