"""Regression checks for the ROM-authentic Exhibition Team Select screen."""

import argparse
import hashlib
import struct
import subprocess
import tempfile
from pathlib import Path

from PIL import Image


EXPECTED_FRAME_HASHES = {
    # Captures correspond to live-ROM $7E:1693 selector states, not merely
    # self-consistent port behavior; native capture tests remain separate.
    "initial": "caa629073e4261102d063bce153a9014832f63544f97216132e7ec4194e97a86",
    "plate_phase_1": "5621b10391b624ef32cb4213f7fd2ebe7363db5a2071b779d8e65acd14b23cd9",
    "alphabetical_right": "3883cde7d8def515381d7bc3ff2a026714b8799020bc4809095d1647ba8f0171",
    "scoring": "3a12a2735ed9ebdbff35707b4222117c32f3b665b6dcaf73272ac8e73bab46e0",
    "overall": "5e355aa5b32910b75f1d53e60095b1623473d473f135a7cce08140cef786bace",
    "side_toggle_name": "e18c42754a108537fba5d5aa9ea70e85c2a02c2a30e10f29448ad7c8180557a2",
    "side_toggle_rank": "4bc485baba09c05bceaa3df5147aa4f6eddf706993825ffea3527e74a0a3bccf",
    "ranked_right": "f47ead7c29fef693748a05fec48eb807fe2f41f0d5b6864d48dce18b42bb824f",
    "alpha_wrap_right": "b2f3363c7f3c21ed4cb388cd22fe8d295cfd22fc71aab680643ff08e2450cec8",
    "alpha_wrap_left": "56203b4dded797eb41aa4e535c4826ad2703f3c0a0a469164cdb97bdaf4c4b65",
    "rank_wrap": "1ff537623a02762701a20379ed5ff9b50a89c6175f2fbe64067a5ebbe5440d40",
    "wallpaper_orlando": "a6c373361981029165758755f113b9132a680badbb8c1a1ada80217d8398e452",
    "left_golden_state": "eeb46a34f88102bdded834ab2c9042e84314c5eca8f0202b4761821d2135cf40",
    "right_philadelphia": "487b0efe17269dcf04d1f004fd4ca1f5647ecd60e3a4b72099427bd03fab3acf",
    "east": "09902c67a6ce3b516c83b079ae8f609494459cf100a13c0ec735c9e0f5a5dfe7",
    "west": "94d3d4fe72c06911a9c27748dd1e5fdfcd83ec6e407706ca2c20326b7d9d0662",
}

# Resource 287 is optional by design. Its addition changes the F12 item-count
# glyph but does not change asset 160 or its rendered logo. Both full hashes are
# retained so the count remains visible/correct, while the masked hash proves
# that the supported configurations have the same debugger canvas outside
# that one seven-pixel glyph.
EXPECTED_DEBUG_FRAME_HASHES = {
    "fallback": "4944e48bb54bb0c88976e02379aef296dfc239f73955d496cd952cc8743b6182",
    "literal": "628aa6decb1c13a1a62fb769b3a0996f6ea07a1522bf2e5252888d7611948041",
    "literal-court": "be6a5d5122d2534106c29781034a25aab00662092d15888ba22e7695871f1dc2",
}
EXPECTED_DEBUG_STABLE_HASH = \
    "3430e5ff2ca65cc16ab37e586a92bbcc717b65be0424b19dfc8a44032ad8e95d"
EXPECTED_PLAYER_DRAW_HASH = \
    "2c561159b63e56e5e42a4d461a1f03bee65c1f7b94fcc5ee933349cbc66bff9f"

# Continuous production Setup -> Team Select frames. These close the coverage
# gap left by direct --team-only captures: the outgoing framebuffer and scroll
# masks must be carried through the actual scene handoff. Native comparison
# anchors are Mesen 423/430/437/442/450 for port 186/193/200/205/213.
EXPECTED_SETUP_TO_TEAM_FRAME_HASHES = {
    186: "8f2cea8276b6b47abb9e30dcce4244174434fd129982618faddbd80501b93a68",
    193: "be4c79ac83dc08327769576b4c22a864aff2ccbfcc66254ad6e7b0caa79907bd",
    200: "edf85e0d24d5932dce33352b70fe2a54f25adc71742547bef4642ebc85ed240a",
    205: "5733bcdf4af93fc47941f42f8d9de0159f8b8574f4fa331d23d56a9f762fc002",
    213: "2cbbeef1249170a43854962fa5b19fba628470c70beb9ce23e15a0f05cb891f2",
}

EXPECTED_LOGO_HASHES = {
    160: "83012b77eeda4d75735eeb22f88690260ab82b55ff53560d46b6d8d306b99510",
    161: "8ef535efc6581fb321baeb59c45aac5f3c9efb3f0bffaa6adb2377a88110763d",
    162: "80b8dd4f439904978c08f4c541136fe0cce25ce9326136d810544328f54651a6",
    163: "9eb03cd3f0c6c2f0274b26170213d94f6648950c9e5e5e4b59b77458c772a058",
    164: "93002e4dae85f238ce91a12fa522465d93d5be5105f654ec23d81117446fb4ec",
    165: "bd3e47ea2581b40b670981d3a06aa77076df8ac0b457683de8022a0dc168578b",
    166: "0ceae3491a050d7e9ae4f6fbb5404d6cae3d4073ced7adc9c7b7d75aee5b989f",
    167: "10277e0b0e30bd3ee9437168c8895dc03ddc45ad2b620462eeef712368e78057",
    168: "0306e60c9deeca17b264e3d66bcc2eb598498dab2f2d9904afce9f67a23c3494",
    169: "44d9640cd838ee40c45c5c94cf6c8aa77c34b1ccbf7203c6087fea7700d6dab7",
    170: "bb7ee47e65c8bfb3a778b30286f2936f4af216e6e8b69e8f79f2c768b930fbeb",
    171: "742836f093b32445d20048dc18355da8e95b20e29604f7554c6cd5d124440f19",
    172: "20faad0e5f9b499562f42930416ec8cd90a4e55d1eaa79558cdfa2989042066b",
    173: "85dca3eaa6fdbcd85fca69412eca78b72b5f26221a58a44e4b7933a64b3274de",
    174: "9a0ea37db371de454d0f44ee209aad9cee790cac78b6900733442e5881d1d220",
    175: "08b2c4bd46a4aa33835891e0a6bbcb3a2cece52ca8d0a98621255660f28d774a",
    176: "e9c581c0d504ec69b89a170dbdf90e4ea45bfe548b25b864499276f5261b81ea",
    177: "af0a154de1c1e100857d5eb35350bc4af896d5429223becb26bdb882c16d7f63",
    178: "63a5d9be700611d2218cc2058754da3c5f6e783dc06bf57f270b2633e7379385",
    179: "acdea1dec6c0a17c210a4e709e58fdcf3d65a3f6eb308c97a8336cbb178f63ac",
    180: "e8a0973e948cf5316c0123680ce8e3d84498b32b2045163f262790c3694b01db",
    181: "d80e230e2f90f65b4e7a9450bd5cdec2e2f8925c0baae55be0090f260a96818b",
    182: "217b1acc086f8d16a863e92b6d22146198b5116e16d47a2c95adbb6fa54dbdce",
    183: "a09065b4befb54649dd0659450900e99c3ef93a09bd33b6f7df8eb6747b0b2d0",
    184: "f88f04049d53ff218ca711b0d55063817c6d01c279e6376ba0b7883c7ae155cb",
    185: "a52025e851bcdc8677d1c803b21c0840162711aca0427a7e2b64268d4fcbd82c",
    186: "a5bfb7c7ed686eddc4dfb5218ac23d45879c3b625496e43f5a2efb244c51798a",
    187: "f7a3797f8dfbfdc3d8c5bc2cc4abbece613ee123ac5aa4fae103d7a475c03e4d",
    188: "47d113c163119cf0c73c2256e854f2eaea9c88323e65f630487a2067d6b9ff33",
}


def load_pack(path):
    raw = path.read_bytes()
    if raw[:8] != b"NBA95PAK":
        raise AssertionError("invalid pack magic")
    version, count = struct.unpack_from("<II", raw, 8)
    if version != 31 or 16 + count * 24 > len(raw):
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


def debug_stable_hash(path):
    image = Image.open(path).convert("RGB")
    # `%02u` prints 264/265/266 at x=32; only the count glyph at x=48..54
    # differs between the otherwise identical supported packs.
    image.paste((0, 0, 0), (48, 19, 55, 26))
    return hashlib.sha256(image.tobytes()).hexdigest()


def player_draw_configuration(assets):
    item = assets.get(287)
    if item is None:
        if len(assets) != 264:
            raise AssertionError("fallback pack is not the reviewed 264-item configuration")
        return "fallback"
    payload, width, height, flags = item
    if len(assets) != 265 + int(288 in assets) or (len(payload), width, height, flags) != \
            (2144, 0, 0, 0) or payload[:8] != b"NBPDRAW1" or \
            struct.unpack_from("<6I", payload, 8) != \
            (1, 2096, 32, 8, 2128, 2144) or \
            hashlib.sha256(payload).hexdigest() != EXPECTED_PLAYER_DRAW_HASH:
        raise AssertionError("literal player-draw resource 287 changed")
    return "literal-court" if 288 in assets else "literal"


def wallpaper_hash(path):
    image = Image.open(path).convert("RGB")
    return hashlib.sha256(image.crop((232, 120, 256, 224)).tobytes()).hexdigest()


def rank_text_bounds(path, left, right):
    image = Image.open(path).convert("RGB")
    rows = ((119, 132), (135, 148), (151, 164), (167, 180), (187, 200))
    bounds = []
    for top, bottom in rows:
        points = [(x, y) for y in range(top, bottom) for x in range(left, right)
                  if all(channel > 150 for channel in image.getpixel((x, y)))]
        if not points:
            raise AssertionError("rank ordinal disappeared")
        bounds.append((min(x for x, _ in points), min(y for _, y in points),
                       max(x for x, _ in points), max(y for _, y in points)))
    return bounds


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--pack", required=True)
    parser.add_argument("--exe", required=True)
    parser.add_argument("--rom", required=True)
    args = parser.parse_args()
    pack, exe, rom = Path(args.pack), Path(args.exe), Path(args.rom)

    assets = load_pack(pack)
    draw_configuration = player_draw_configuration(assets)
    required = {189, 250, *range(160, 189), *range(192, 221), *range(221, 250)}
    if not required.issubset(assets):
        raise AssertionError(f"missing Team Select assets: {sorted(required - assets.keys())}")
    for asset_id, expected in EXPECTED_LOGO_HASHES.items():
        data, width, height, flags = assets[asset_id]
        if (width, height, flags, len(data)) != (48, 56, asset_id - 160, 48 * 56 * 4):
            raise AssertionError(f"logo metadata changed for asset {asset_id}")
        if hashlib.sha256(data).hexdigest() != expected:
            raise AssertionError(f"ROM OBJ logo {asset_id} changed")
    for team in range(29):
        if (len(assets[192 + team][0]), assets[192 + team][3]) != (0x10000, team):
            raise AssertionError(f"team {team} VRAM asset changed")
        if (len(assets[221 + team][0]), assets[221 + team][3]) != (0x200, team):
            raise AssertionError(f"team {team} CGRAM asset changed")
    cycle, width, height, cadence = assets[250]
    if (len(cycle), width, height, cadence) != (26, 7, 7, 8):
        raise AssertionError("selected-plate palette-cycle metadata changed")
    phases = [cycle[phase * 2:phase * 2 + 14] for phase in range(7)]
    colors = [struct.unpack_from("<H", cycle, offset)[0] & 0x7FFF
              for offset in range(0, 26, 2)]
    if hashlib.sha256(cycle).hexdigest() != \
       "f3573907c51de1bb052e0fd0000431e9d20e4328837ec76e6c7cf25e06f9a459" or \
       len(set(phases)) != 7 or colors[7:] != colors[:6]:
        raise AssertionError("$82:8968 selected-plate cycle no longer wraps seven ROM colors")
    oam = assets[189][0]
    def obj(index):
        high = (oam[512 + index // 4] >> ((index & 3) * 2)) & 3
        x = oam[index * 4] | ((high & 1) << 8)
        if x >= 256:
            x -= 512
        return (x, oam[index * 4 + 1], oam[index * 4 + 2],
                (oam[index * 4 + 3] >> 1) & 7, 16 if high & 2 else 8)
    plate_tiles = [0x20, 0x22, 0x24, 0x26, 0x28, 0x2A, 0x2C, 0x2E,
                   0x40, 0x42, 0x42, 0x42, 0x42, 0x42, 0x42]
    right_plate = [obj(index) for index in range(6, 21)]
    left_plate = [obj(index) for index in range(32, 47)]
    if [item[2] for item in right_plate] != plate_tiles or \
       [item[2] for item in left_plate] != plate_tiles or \
       {item[3] for item in right_plate} != {2} or \
       {item[3] for item in left_plate} != {1} or \
       (min(item[0] for item in left_plate),
        max(item[0] + item[4] for item in left_plate)) != (30, 78):
        raise AssertionError("complete 15-piece silver/gold plate geometry changed")

    listing = run(exe, "--team-list")
    lines = [line for line in listing.splitlines() if line.startswith("[TEAM DATA]")]
    if len(lines) != 29 or "03 CHICAGO" not in lines[3] or \
       "S=23 R=09 B=13 D=03 O=07" not in lines[3] or \
       "27 EAST" not in lines[27] or "S=- R=- B=- D=- O=-" not in lines[27] or \
       "28 WEST" not in lines[28] or "S=- R=- B=- D=- O=-" not in lines[28]:
        raise AssertionError("source-controlled $80:D9AF-$DA3F ranking table changed")

    with tempfile.TemporaryDirectory() as directory:
        directory = Path(directory)
        base = ["--headless", "--rom", rom, "--assets", pack,
                "--team-only", "--team-action-gap", "1"]
        # These are the semantic states observed in Mesen and traced through
        # $82:83BC-$8548. Selector 0/1 is a name row; 2..6 is a rank row.
        cases = {
            "initial": (225, [],
                        "active=RIGHT selector=1 category=-1 left=3:CHICAGO right=18:ORLANDO"),
            "plate_phase_1": (233, [],
                              "active=RIGHT selector=1 category=-1 left=3:CHICAGO right=18:ORLANDO"),
            "alphabetical_right": (225, ["--team-right", "1"],
                                   "selector=1 category=-1 left=3:CHICAGO right=19:PHILADELPHIA"),
            "scoring": (225, ["--team-down", "1"],
                        "active=RIGHT selector=2 category=0 left=3:CHICAGO right=18:ORLANDO"),
            "overall": (225, ["--team-up", "1"],
                        "active=RIGHT selector=6 category=4 left=3:CHICAGO right=18:ORLANDO"),
            "side_toggle_name": (225, ["--team-side-toggle"],
                                 "active=LEFT selector=0 category=-1 left=3:CHICAGO right=18:ORLANDO"),
            "side_toggle_rank": (225, ["--team-category", "3", "--team-side-toggle"],
                                 "active=LEFT selector=5 category=3 left=3:CHICAGO right=18:ORLANDO"),
            "ranked_right": (225, ["--team-category", "0", "--team-right", "1"],
                             "active=RIGHT selector=2 category=0 left=3:CHICAGO right=13:MIAMI"),
            "alpha_wrap_right": (225, ["--team-right", "11"],
                                 "selector=1 category=-1 left=3:CHICAGO right=0:ATLANTA"),
            "alpha_wrap_left": (225, ["--team-left", "21"],
                                "selector=1 category=-1 left=3:CHICAGO right=26:WASHINGTON"),
            "rank_wrap": (250, ["--team-category", "0", "--team-right", "27"],
                          "selector=2 category=0 left=3:CHICAGO right=18:ORLANDO"),
            # Long names exercise $81:9FD4 left anchoring and $81:A01F right
            # anchoring. These also distinguish visitor changes from the
            # right/home team's wallpaper ownership.
            "wallpaper_orlando": (240, [],
                                  "left=3:CHICAGO right=18:ORLANDO"),
            "left_golden_state": (240, ["--team-side-toggle", "--team-right", "5"],
                                  "active=LEFT selector=0 category=-1 left=8:GOLDEN STATE right=18:ORLANDO"),
            "right_philadelphia": (240, ["--team-right", "1"],
                                   "active=RIGHT selector=1 category=-1 left=3:CHICAGO right=19:PHILADELPHIA"),
            "east": (240, ["--team-right", "9"],
                     "active=RIGHT selector=1 category=-1 left=3:CHICAGO right=27:EAST"),
            "west": (240, ["--team-right", "10"],
                     "active=RIGHT selector=1 category=-1 left=3:CHICAGO right=28:WEST"),
        }
        captured = {}
        for name, (frames, extra, expected_state) in cases.items():
            image = directory / f"{name}.bmp"
            output = run(exe, *base, "--frames", frames, *extra,
                         "--dump-frame", image)
            if expected_state not in output:
                raise AssertionError(f"Team Select navigation failed for {name}:\n{output}")
            if frame_hash(image) != EXPECTED_FRAME_HASHES[name]:
                raise AssertionError(f"Team Select {name} frame changed")
            captured[name] = image

        if wallpaper_hash(captured["wallpaper_orlando"]) != \
           wallpaper_hash(captured["left_golden_state"]):
            raise AssertionError("visitor change incorrectly replaced the home wallpaper")
        if wallpaper_hash(captured["wallpaper_orlando"]) == \
           wallpaper_hash(captured["right_philadelphia"]):
            raise AssertionError("home-team change failed to replace the wallpaper")

        if rank_text_bounds(captured["initial"], 36, 70) != [
                (40, 119, 65, 131), (40, 135, 57, 147),
                (40, 151, 62, 163), (40, 167, 58, 179),
                (40, 187, 58, 199)]:
            raise AssertionError("left rank values shifted, overlapped, or were clipped")
        # Stop at x=216: the silver plate has bright pixels at x=216-217 on
        # the first row, immediately beside (but not part of) the ordinal.
        if rank_text_bounds(captured["initial"], 188, 216) != [
                (192, 119, 215, 131), (192, 135, 215, 147),
                (192, 151, 214, 163), (192, 167, 214, 179),
                (192, 187, 209, 199)]:
            raise AssertionError("right rank values shifted, overlapped, or were clipped")

        logo_debug = directory / "logo_debug.bmp"
        run(exe, "--headless", "--rom", rom, "--assets", pack,
            "--asset-debug", "160", "--frames", "1", "--dump-frame", logo_debug)
        if frame_hash(logo_debug) != \
                EXPECTED_DEBUG_FRAME_HASHES[draw_configuration]:
            raise AssertionError(
                f"F12 Team Select logo view changed for {draw_configuration} pack")
        if debug_stable_hash(logo_debug) != EXPECTED_DEBUG_STABLE_HASH:
            raise AssertionError("F12 Team Select stable logo canvas changed")

    handoff = run(exe, "--headless", "--rom", rom, "--assets", pack,
                  "--setup-only", "--setup-main-row", "0", "--setup-main-confirm",
                  "--frames", "400", "--debug-state")
    if "route=TEAM_SELECTION" not in handoff or "SCN:TEAM_SELECT" not in handoff or \
       "AUD:SETUP_SPC" not in handoff or "transition=176" not in handoff:
        raise AssertionError(f"Start handoff or continuous Setup music failed:\n{handoff}")

    # Mesen frames 401/403/421/422/450/451/452/514, rebased to the
    # setup-only harness. These guards prevent the removed screenshot fade
    # from being reintroduced at the scene boundary.
    edge_states = {
        164: ("SCN:GAME_SETUP", "X1:512 X2:000", "Y3:000"),
        166: ("SCN:GAME_SETUP", "Y3:014"),
        184: ("SCN:GAME_SETUP", "X1:000 X2:000", "Y3:182"),
        185: ("SCN:GAME_SETUP", "X1:008 X2:1016", "Y3:182"),
        213: ("SCN:GAME_SETUP", "B:01 X1:232 X2:792"),
        214: ("SCN:GAME_SETUP", "BLK:1"),
        215: ("SCN:TEAM_SELECT", "TF:052"),
        277: ("SCN:TEAM_SELECT", "TF:114"),
    }
    for frames, markers in edge_states.items():
        edge = run(exe, "--headless", "--rom", rom, "--assets", pack,
                   "--setup-only", "--setup-main-row", "0",
                   "--setup-main-confirm", "--frames", frames, "--debug-state")
        if any(marker not in edge for marker in markers):
            raise AssertionError(
                f"Setup -> Team Select edge frame {frames} changed:\n{edge}")

    # State markers alone cannot detect stale tilemap cells in the outgoing
    # framebuffer. Render the natural handoff once and lock the reviewed frames
    # that previously exposed the port-only colored-tile corruption.
    with tempfile.TemporaryDirectory() as handoff_directory:
        handoff_directory = Path(handoff_directory)
        run(exe, "--headless", "--rom", rom, "--assets", pack,
            "--setup-only", "--setup-main-row", "0", "--setup-main-confirm",
            "--frames", "213", "--dump-sequence-from", "186",
            "--dump-sequence-dir", handoff_directory)
        for frame, expected_hash in EXPECTED_SETUP_TO_TEAM_FRAME_HASHES.items():
            path = handoff_directory / f"frame_{frame:04d}.bmp"
            if frame_hash(path) != expected_hash:
                raise AssertionError(
                    f"continuous Setup -> Team Select rendered frame {frame} changed")

    a_handoff = run(exe, "--headless", "--rom", rom, "--assets", pack,
                    "--setup-only", "--setup-main-row", "0", "--setup-main-a",
                    "--frames", "400", "--debug-state")
    if "action=0" not in a_handoff or "SCN:GAME_SETUP" not in a_handoff or \
       "route=TEAM_SELECTION" in a_handoff:
        raise AssertionError(f"Controller A incorrectly confirmed Exhibition:\n{a_handoff}")

    print("[TEST] PASS: continuous Setup pixel handoff, Start-only dispatch, seven-position ROM selector, 29 teams/logos, dash ranks, and navigation wrap")


if __name__ == "__main__":
    main()
