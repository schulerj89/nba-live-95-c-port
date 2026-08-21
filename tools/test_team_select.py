"""Regression checks for the ROM-authentic Exhibition Team Select screen."""

import argparse
import hashlib
import struct
import subprocess
import tempfile
from pathlib import Path

from PIL import Image


EXPECTED_FRAME_HASHES = {
    "initial": "73a518db950e159a8462131b323cfc8090bed38ce91ca47e898229b3f4c748ed",
    "right": "b6f520d388a93c4fb4594ce57b2e6d5872660f0fb74583f8f4ca831817683459",
    "left": "34dc2617977dfe01f6988525ecfb02ae740d7bb1d661c2e338f3b64cc59a3693",
    "overall": "f6b2212fbe443f50847c6f7766a24fa2584d9a1fa96171cb79c55438b136892a",
    "logo_debug": "fd650adfa8647660a06feaa56161586dc0e56cff0e710a1565a627bf25162ec1",
}

EXPECTED_LOGO_HASHES = {
    160: "65ee770e91a66fb13b1cd583b8ca8820e480cbbc39fca8b1d88d84f60d21db17",
    161: "47bf96b5069037cef3ec0ebab9cd0ec6df6091b2c583c8d45cedefbfc8bba000",
    162: "a6eb6ddf5b3cfe31c9fc90ba93f705364ca1fe87939da8375787cb13aafb10f9",
    163: "35bcb5c2eb9e7287267837ad8d2c55bf5e6b4bd0da50f4110975a7956a3d2783",
    164: "47b15cb281c56963ebae2a42575b5d7ef0495a41b104ad0dc18599791f21cbf6",
    165: "ea2cbe3ab390544bd510c38f46253e87f51b31a5053ded6cb194d24e9330d77a",
    166: "2769ece1503e861b0e182a481e23a30d889b6fdc2bace1f389476b545551e6d2",
    167: "4545d3bd22f269eaa28d14d17c703c9a2a1471df54a14edb9ebd6eed2ff583b3",
    168: "9dc563f127813871525837338258fa057594af91a11de41ea31f6c567909aa0b",
    169: "babc66d987ac40b923ec414c82fab5a1ac7141e0b23986341b0e958cedd9c98f",
    170: "f216dd25c4ced25ce5fc7c06ebab057f9597219d0948fe2327ab6c9c9b5d5ed3",
    171: "c9408439d3636f92ff9dc912c59467f1570d48ae2bb9342958673a854e965e70",
    172: "5b7e5489e1e2ef7c99669c04e26740e9453d0bc4ed8e5f992ebec2713186e71b",
    173: "cb3747568960c1f8155dd4cb7e7957b5e2fb496c4ef5fa6eb7eae6aba270a95f",
    174: "6e150f51454f4e17691f96c280efdcc8e2f85803c46b7d2e9741d078d738db73",
    175: "d8686414eb08c348062cb9541fa03abbffdc623da68e3d6e244b256495652487",
    176: "417576df9347a2960c85fb1bf107390d219fd1df5034b83fc7ac50e18ba27d63",
    177: "d66117b27f42c57be036bd5e966069c7b24c6c0c00cdce11fb4a819398fadd44",
    178: "63a5d9be700611d2218cc2058754da3c5f6e783dc06bf57f270b2633e7379385",
    179: "b757096e82487fee0c36ac0dd8fa1b1a9f147ca25bdbc8e86a730fcec3e6b8e1",
    180: "d773e6e38da19ee8f0a5198dffd848015f518451ccb8f01a1ca4a9baf3617c93",
    181: "f0a2dd4ae162a522ec4fe1cd98646f4d44312cd0f46c353176d291422208087b",
    182: "8ad9064db726b4024c85f820c5770732603346ce7dcdb21a4ef4e6a5f6641d2d",
    183: "7d4f25fd95e3176ca293cd864d8b33094846aa724a5aca64d65590901b2b4f2f",
    184: "fa23f8310f715206f0162df7a6b8c8684261efc33fcbe4cf8c0d5a218b8bfc0f",
    185: "a52025e851bcdc8677d1c803b21c0840162711aca0427a7e2b64268d4fcbd82c",
    186: "a5bfb7c7ed686eddc4dfb5218ac23d45879c3b625496e43f5a2efb244c51798a",
}


def load_pack(path):
    raw = path.read_bytes()
    if raw[:8] != b"NBA95PAK":
        raise AssertionError("invalid pack magic")
    version, count = struct.unpack_from("<II", raw, 8)
    if version != 12 or 16 + count * 24 > len(raw):
        raise AssertionError("invalid Team Select pack directory")
    assets = {}
    for index in range(count):
        fields = struct.unpack_from("<6I", raw, 16 + index * 24)
        asset_id, offset, size, width, height, flags = fields
        if offset + size > len(raw) or asset_id in assets:
            raise AssertionError("unsafe Team Select pack entry")
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


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--pack", required=True)
    parser.add_argument("--exe", required=True)
    parser.add_argument("--rom", required=True)
    args = parser.parse_args()
    pack, exe, rom = Path(args.pack), Path(args.exe), Path(args.rom)

    assets = load_pack(pack)
    required = {187, *range(160, 187), *range(192, 246)}
    if not required.issubset(assets):
        raise AssertionError(f"missing Team Select assets: {sorted(required - assets.keys())}")
    for asset_id, expected in EXPECTED_LOGO_HASHES.items():
        data, width, height, flags = assets[asset_id]
        if (width, height, flags, len(data)) != (48, 56, asset_id - 160, 48 * 56 * 4):
            raise AssertionError(f"logo metadata changed for asset {asset_id}")
        if hashlib.sha256(data).hexdigest() != expected:
            raise AssertionError(f"ROM OBJ logo {asset_id} changed")
    for team in range(27):
        if (len(assets[192 + team][0]), assets[192 + team][3]) != (0x10000, team):
            raise AssertionError(f"team {team} VRAM asset changed")
        if (len(assets[219 + team][0]), assets[219 + team][3]) != (0x200, team):
            raise AssertionError(f"team {team} CGRAM asset changed")

    listing = run(exe, "--team-list")
    lines = [line for line in listing.splitlines() if line.startswith("[TEAM DATA]")]
    if len(lines) != 27 or "03 CHICAGO" not in lines[3] or "S=23 R=09 B=13 D=03 O=07" not in lines[3]:
        raise AssertionError("source-controlled $80:D9AF ranking table changed")

    with tempfile.TemporaryDirectory() as directory:
        directory = Path(directory)
        base = ["--headless", "--rom", rom, "--assets", pack,
                "--team-only", "--frames", "205"]
        cases = {
            "initial": ([], "active=RIGHT category=0 left=3:CHICAGO right=18:ORLANDO"),
            "right": (["--team-right", "1"], "left=3:CHICAGO right=13:MIAMI"),
            "left": (["--team-side-toggle", "--team-right", "1"],
                     "active=LEFT category=0 left=7:DETROIT right=18:ORLANDO"),
            "overall": (["--team-category", "4", "--team-right", "1"],
                        "active=RIGHT category=4 left=3:CHICAGO right=8:GOLDEN STATE"),
        }
        for name, (extra, expected_state) in cases.items():
            image = directory / f"{name}.bmp"
            output = run(exe, *base, *extra, "--dump-frame", image)
            if expected_state not in output:
                raise AssertionError(f"Team Select navigation failed for {name}:\n{output}")
            if frame_hash(image) != EXPECTED_FRAME_HASHES[name]:
                raise AssertionError(f"Team Select {name} frame changed")

        logo_debug = directory / "logo_debug.bmp"
        run(exe, "--headless", "--rom", rom, "--assets", pack,
            "--asset-debug", "160", "--frames", "1", "--dump-frame", logo_debug)
        if frame_hash(logo_debug) != EXPECTED_FRAME_HASHES["logo_debug"]:
            raise AssertionError("F12 Team Select logo view changed")

    handoff = run(exe, "--headless", "--rom", rom, "--assets", pack,
                  "--setup-only", "--setup-main-row", "0", "--setup-main-confirm",
                  "--frames", "400", "--debug-state")
    if "route=TEAM_SELECTION" not in handoff or "SCN:TEAM_SELECT" not in handoff or \
       "AUD:SETUP_SPC" not in handoff or "transition=176" not in handoff:
        raise AssertionError(f"Start handoff or continuous Setup music failed:\n{handoff}")

    print("[TEST] PASS: Team Select handoff, 27 ROM teams/logos, side toggle, and rank-order cycling")


if __name__ == "__main__":
    main()
