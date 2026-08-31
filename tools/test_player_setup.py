"""Regression checks for Team Select -> Player Setup and ROM PPU assets."""

import argparse
import hashlib
import struct
import subprocess
import tempfile
from pathlib import Path

from PIL import Image


EXPECTED_ASSET_HASHES = {
    257: "f90159b28fd5b1beb869a2dbc4fd2689babc47dbbb4cc0acf580ef9f7b741194",
    258: "d56926523b8d50f7b3e8c27cf68b94f8b9df0b572add17b3371ccff64c1104b4",
    259: "156d89e6286e725a6bcee2ce39b9078cbfc5d62c65ee95d7fff25aaeaf3bc757",
}

EXPECTED_FRAME_HASHES = {
    # Team Select's settled BG2 cadence now remains continuous from the
    # corrected Setup edge (Mesen frame 576 = vscroll 20).
    "outgoing": "872f67151f1dc4fed9bca5fe54cb669861b105a197ac87b319e735437ecd2629",
    "blank": "2cbbeef1249170a43854962fa5b19fba628470c70beb9ce23e15a0f05cb891f2",
    "background": "18e07023dae89084b28b738c7b50dddfa0ab77a6d24c41a6ec92ab6ac39432e8",
    "labels": "19673f073dbc2f2af58a3dbcf19cbe1e3b0d94f44a8600b7913a539c6e0750f8",
    "settled": "52d521e29d8665b63eb6e98a63af4d22480a51833ab72a40aee0eae6b022082d",
    # Native OAM places the left controller at x40, not the old C x42.
    # build/player-setup-attribution-v2 restores only the old offset and
    # reproduces the old full hash3dfda176... exactly. The318 changed pixels
    # lie in x40..78/y110..125. This remains a C regression, not full UI parity.
    "left": "3ed5198441075776e92641754bfd2302ed2782050e5889270693052988c8ad23",
}


def load_pack(path):
    raw = path.read_bytes()
    if raw[:8] != b"NBA95PAK":
        raise AssertionError("invalid pack magic")
    version, count = struct.unpack_from("<II", raw, 8)
    if version != 31 or 16 + count * 24 > len(raw):
        raise AssertionError("invalid Player Setup pack directory")
    assets = {}
    for index in range(count):
        fields = struct.unpack_from("<6I", raw, 16 + index * 24)
        asset_id, offset, size, width, height, flags = fields
        if offset + size > len(raw) or asset_id in assets:
            raise AssertionError("unsafe Player Setup pack entry")
        assets[asset_id] = (raw[offset:offset + size], width, height, flags)
    return assets


def run(exe, *args):
    result = subprocess.run([str(exe), *map(str, args)], text=True,
                            capture_output=True, check=False)
    if result.returncode:
        raise AssertionError(f"command failed:\n{result.stdout}\n{result.stderr}")
    return result.stdout


def frame_hash(path):
    return hashlib.sha256(Image.open(path).convert("RGB").tobytes()).hexdigest()


def obj(oam, index):
    high = (oam[512 + index // 4] >> ((index & 3) * 2)) & 3
    x = oam[index * 4] | ((high & 1) << 8)
    if x >= 256:
        x -= 512
    attr = oam[index * 4 + 3]
    return (x, oam[index * 4 + 1], oam[index * 4 + 2],
            (attr >> 1) & 7, 16 if high & 2 else 8)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--pack", required=True)
    parser.add_argument("--exe", required=True)
    parser.add_argument("--rom", required=True)
    args = parser.parse_args()
    pack, exe, rom = Path(args.pack), Path(args.exe), Path(args.rom)
    assets = load_pack(pack)

    expected_sizes = {257: 0x10000, 258: 0x200, 259: 0x220}
    for asset_id, expected_hash in EXPECTED_ASSET_HASHES.items():
        if asset_id not in assets:
            raise AssertionError(f"missing Player Setup asset {asset_id}")
        payload, width, height, flags = assets[asset_id]
        if len(payload) != expected_sizes[asset_id] or (width, height, flags) != (0, 0, 0):
            raise AssertionError(f"Player Setup asset {asset_id} metadata changed")
        if hashlib.sha256(payload).hexdigest() != expected_hash:
            raise AssertionError(f"Player Setup ROM PPU payload {asset_id} changed")

    oam = assets[259][0]
    plate_tiles = [0x20, 0x22, 0x24, 0x26, 0x28, 0x2A, 0x2C, 0x2E,
                   0x40, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42]
    right_plate = [obj(oam, index) for index in range(6, 21)]
    left_plate = [obj(oam, index) for index in range(32, 47)]
    controller = [obj(oam, index) for index in range(47, 52)]
    if [item[2] for item in right_plate] != plate_tiles or \
       [item[2] for item in left_plate] != plate_tiles or \
       {item[3] for item in right_plate} != {2} or \
       {item[3] for item in left_plate} != {1}:
        raise AssertionError("Player Setup plate groups changed")
    if [(item[0], item[1]) for item in controller] != \
       [(175, 111), (174, 110), (190, 110), (206, 110), (206, 118)]:
        raise AssertionError("Player Setup arrow/controller group changed")

    with tempfile.TemporaryDirectory() as directory:
        directory = Path(directory)
        base = ["--headless", "--rom", rom, "--assets", pack,
                "--team-only", "--team-confirm"]
        cases = {
            "outgoing": 177,
            # Native normal-input capture reaches forced black 51 presented
            # frames after the Team Select Start edge (650 -> 701).
            "blank": 229,
            "background": 327,
            "labels": 355,
            "settled": 378,
        }
        for name, frames in cases.items():
            image = directory / f"{name}.bmp"
            output = run(exe, *base, "--frames", frames, "--dump-frame", image)
            if name == "settled" and ("[PLAYER SETUP TEST] p1=RIGHT" not in output or
                                      "left=3:CHICAGO right=18:ORLANDO" not in output or
                                      "transition=200" not in output):
                raise AssertionError(f"Team Select handoff lost session teams:\n{output}")
            if frame_hash(image) != EXPECTED_FRAME_HASHES[name]:
                raise AssertionError(f"Player Setup {name} frame changed")

        left = directory / "left.bmp"
        output = run(exe, "--headless", "--rom", rom, "--assets", pack,
                     "--player-setup-only", "--player-setup-left",
                     "--frames", 240, "--dump-frame", left)
        if "p1=LEFT" not in output or frame_hash(left) != EXPECTED_FRAME_HASHES["left"]:
            raise AssertionError(f"Player 1 side assignment failed:\n{output}")

        # The CLI destination LEFT now requires two native one-step presses.
        # The first tap reaches neutral; its release must not advance again.
        for frames, selection in ((201, "NEUTRAL"), (202, "NEUTRAL"), (203, "LEFT")):
            output = run(exe, "--headless", "--rom", rom, "--assets", pack,
                         "--player-setup-only", "--player-setup-left", "--frames", frames)
            if f"[PLAYER SETUP TEST] p1={selection}" not in output:
                raise AssertionError(f"Player Setup press/release sequence changed at{frames}:\n{output}")

        changed = directory / "changed.bmp"
        output = run(exe, *base, "--team-side-toggle", "--team-right", 5,
                     "--frames", 390, "--dump-frame", changed)
        if "left=8:GOLDEN STATE right=18:ORLANDO" not in output:
            raise AssertionError(f"selected teams did not persist into Player Setup:\n{output}")

        home_changed = directory / "philadelphia_home.bmp"
        output = run(exe, *base, "--team-right", 1, "--frames", 390,
                     "--dump-frame", home_changed, "--debug-state")
        if "left=3:CHICAGO right=19:PHILADELPHIA" not in output or \
           frame_hash(home_changed) != \
                "48ff66cddbbfd42ee9a884e8e064a589c932051acfb6caf4cfa6339ae9346d8a":
            raise AssertionError("selected home logo/wallpaper did not replace Orlando")

    print("[TEST] PASS: Team Select layer exit, 200-frame transition, dynamic home wallpaper, selected teams, and Player 1 side")


if __name__ == "__main__":
    main()
