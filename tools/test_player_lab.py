import argparse
import hashlib
import struct
import subprocess
import tempfile
from pathlib import Path


def read_pack(path):
    raw = Path(path).read_bytes()
    if raw[:8] != b"NBA95PAK":
        raise AssertionError("invalid asset-pack magic")
    version, count = struct.unpack_from("<II", raw, 8)
    if version != 19:
        raise AssertionError(f"Player Lab requires pack v18, got {version}")
    assets = {}
    for index in range(count):
        entry = struct.unpack_from("<IIIIII", raw, 16 + index * 24)
        asset_id, offset, size, width, height, flags = entry
        assets[asset_id] = (raw[offset:offset + size], width, height, flags)
    return assets


def run(exe, rom, pack, *args):
    command = [str(exe), "--headless", "--rom", str(rom), "--assets", str(pack),
               "--frames", "1", *map(str, args)]
    return subprocess.run(command, capture_output=True, text=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--pack", required=True)
    parser.add_argument("--exe", required=True)
    parser.add_argument("--rom", required=True)
    args = parser.parse_args()
    assets = read_pack(args.pack)
    for asset_id in range(251, 257):
        if asset_id not in assets:
            raise AssertionError(f"missing Player Lab asset {asset_id}")

    roster, width, height, record_size = assets[251]
    if roster[:8] != b"NBPROST1" or (width, height, record_size) != (29, 12, 64):
        raise AssertionError("invalid Player Lab roster schema")
    version, teams, players, packed_size = struct.unpack_from("<IIII", roster, 8)
    if (version, teams, players, packed_size) != (1, 29, 12, 64):
        raise AssertionError("invalid Player Lab roster header")
    if len(roster) != 24 + 29 * 12 * 64:
        raise AssertionError("Player Lab roster asset is truncated")

    def record(team, player):
        off = 24 + (team * 12 + player) * 64
        p = roster[off:off + 64]
        return (struct.unpack_from("<I", p)[0], *p[4:12],
                p[32:64].split(b"\0", 1)[0].decode("ascii"))

    chicago = record(3, 2)
    if chicago != (0xADA074, 24, 0, 85, 145, 0x99, 0x80, 0x19, 2, "Cartwright"):
        raise AssertionError(f"Chicago ROM record changed: {chicago}")
    west = record(28, 10)
    if west[-1] != "D. Robinson" or west[5:8] != (0xF5, 0x99, 0x8E):
        raise AssertionError(f"West ROM record changed: {west}")

    pose, pose_w, pose_h, _ = assets[252]
    if (pose_w, pose_h, len(pose)) != (24, 64, 24 * 64 * 4):
        raise AssertionError("invalid ROM-built front pose")
    opaque = sum(pose[index + 3] != 0 for index in range(0, len(pose), 4))
    if opaque < 300:
        raise AssertionError(f"default pose lost pixels: {opaque}")
    if assets[253][0][:8] != b"NBPTILE2" or assets[254][0][:8] != b"NBPALET2" or \
       assets[255][0][:8] != b"NBPPOSE2":
        raise AssertionError("ROM Player Lab source assets changed")
    pose_tiles, head_tiles = struct.unpack_from("<II", assets[253][0], 12)
    if (pose_tiles, head_tiles) != (17, 195) or \
       len(assets[253][0]) != 20 + (17 + 195) * 40:
        raise AssertionError("player tile-source manifest is incomplete")
    if struct.unpack_from("<IIII", assets[254][0], 8) != (2, 29, 2, 3) or \
       len(assets[254][0]) != 36 + 29 * 2 * 3 * 32:
        raise AssertionError("player team/palette matrix is incomplete")
    animations, states, directions, schema = assets[256]
    if animations[:8] != b"NBPANIM1" or (states, directions, schema) != (57, 8, 4):
        raise AssertionError("invalid ROM player-animation schema")
    (animation_version, state_count, resource_count, bank84_offset,
     attachment_offset, directory_offset, data_offset, digit_source_offset,
     bcd_table_offset, number_attachment_offset, number_palette_offset,
     number_visibility_offset) = struct.unpack_from("<12I", animations, 8)
    if animation_version != 4 or state_count != 57 or resource_count < 1800 or \
       bank84_offset != 56 or attachment_offset != 56 + 0x8000 or \
       not attachment_offset < directory_offset < data_offset < \
       digit_source_offset < bcd_table_offset < number_attachment_offset or \
       digit_source_offset + 90 * 32 != bcd_table_offset or \
       bcd_table_offset + 100 != number_attachment_offset or \
       number_attachment_offset + 0x830 * 2 != number_palette_offset or \
       number_palette_offset + 64 + 29 * 4 != number_visibility_offset or \
       number_visibility_offset + 0x830 != len(animations):
        raise AssertionError("player-animation ROM tables are incomplete")
    if animations[bcd_table_offset + 54] != 0x54 or \
       not any(animations[digit_source_offset:digit_source_offset + 90 * 32]):
        raise AssertionError("jersey-number ROM tables are incomplete")
    if not any(animations[number_palette_offset:number_palette_offset + 64]):
        raise AssertionError("jersey-number OBJ palette source is incomplete")
    if animations[number_visibility_offset + 0x00F3] != 0x08 or \
       animations[number_visibility_offset + 0x01A7] != 0xF5:
        raise AssertionError("jersey-number upper-frame eligibility table changed")
    resource_ids = []
    for index in range(resource_count):
        resource_id, reserved, offset, size = struct.unpack_from(
            "<HHII", animations, directory_offset + index * 12)
        if reserved or offset < data_offset or offset + size > len(animations) or size < 17:
            raise AssertionError(f"invalid animation resource ${resource_id:04X}")
        resource_ids.append(resource_id)
    if resource_ids != sorted(set(resource_ids)) or \
       not all(resource in resource_ids for resource in range(0x049C, 0x049C + 195)) or \
       not all(resource in resource_ids for resource in (0x0591, 0x0592, 0x0593)):
        raise AssertionError("animation resource directory is incomplete")
    for team in range(29):
        for player in range(12):
            off = 24 + (team * 12 + player) * 64
            palette, raw, style, modifier, base, front = struct.unpack_from(
                "<BBBBHH", roster, off + 12)
            expected_style = raw & 0x1f if raw >= 0x27 else raw
            if palette > 2 or style != expected_style or \
               base != 0x049c + style * 5 or front != base + 2:
                raise AssertionError(
                    f"invalid appearance map team={team} player={player}")
    for team, player, expected_name, expected_style in (
            (10, 4, "Workman", 38), (12, 11, "Rambis", 37)):
        selected = record(team, player)
        packed_offset = 24 + (team * 12 + player) * 64
        if selected[-1] != expected_name or \
           roster[packed_offset + 14] != expected_style:
            raise AssertionError(
                f"high head-family roster mapping changed: {selected}")

    with tempfile.TemporaryDirectory() as directory:
        frame = Path(directory) / "player.bmp"
        result = run(args.exe, args.rom, args.pack, "--player-lab", "--player-team", 3,
                     "--player-roster", 2, "--dump-frame", frame)
        if result.returncode:
            raise AssertionError(result.stdout + result.stderr)
        if "name=Cartwright" not in result.stdout or "source=asset-pack" not in result.stdout or \
           "angle=270 flip=off" not in result.stdout:
            raise AssertionError("Player Lab CLI diagnostics missing")
        digest = hashlib.sha256(frame.read_bytes()).hexdigest()
        expected = "8ee7d108ea55b0be9ddece53e3bb83f1ae8c4bb8ebbe82756ffe00ef1cb5d30d"
        if digest != expected:
            raise AssertionError(f"Player Lab frame changed: {digest}")
        second = Path(directory) / "oneal.bmp"
        result = run(args.exe, args.rom, args.pack, "--player-lab", "--player-team", 18,
                     "--player-roster", 2, "--dump-frame", second)
        if result.returncode or hashlib.sha256(second.read_bytes()).hexdigest() == digest:
            raise AssertionError("team/player appearance did not change the runtime sprite")
        animation_hashes = set()
        for state in (0x00, 0x03, 0x16, 0x2B, 0x32):
            frame = Path(directory) / f"animation_{state:02x}.bmp"
            result = run(args.exe, args.rom, args.pack, "--player-lab",
                         "--player-team", 3, "--player-roster", 2,
                         "--player-animation", state, "--player-direction", 6,
                         "--frames", 19, "--dump-frame", frame)
            if result.returncode or f"animation=${state:02X}" not in result.stdout:
                raise AssertionError(result.stdout + result.stderr)
            animation_hashes.add(hashlib.sha256(frame.read_bytes()).hexdigest())
        if len(animation_hashes) != 5:
            raise AssertionError("ROM animation states did not render distinct poses")
        front = Path(directory) / "front_number_24.bmp"
        result = run(args.exe, args.rom, args.pack, "--player-lab",
                     "--player-team", 3, "--player-roster", 2,
                     "--player-animation", 3, "--player-direction", 3,
                     "--frames", 19, "--dump-frame", front)
        front_digest = hashlib.sha256(front.read_bytes()).hexdigest()
        if result.returncode or front_digest != \
           "ea4648bb03388987ce2254e269aeb7c60c71f2cfbd0836f16f1082c980588f37":
            raise AssertionError(
                f"front jersey-number tile/palette changed: {front_digest}")
        visibility_cases = (
            (0, "upper=$00F3 number=visible gate=$08",
             "0a06aff372e90da95c85473b2245696ae3eab986144bf5c2c14f91fa86cadd96"),
            (1, "upper=$01A7 number=hidden gate=$F5",
             "49caab038eb97fb2bf5ea7ada3f4f4caf9f38be44a238e1072e063cf19be703d"),
        )
        for state, diagnostic, expected_hash in visibility_cases:
            frame = Path(directory) / f"number_gate_state_{state}.bmp"
            result = run(args.exe, args.rom, args.pack, "--player-lab",
                         "--player-team", 18, "--player-roster", 2,
                         "--player-animation", state, "--player-direction", 0,
                         "--dump-frame", frame)
            digest = hashlib.sha256(frame.read_bytes()).hexdigest()
            if result.returncode or diagnostic not in result.stdout or \
               digest != expected_hash:
                raise AssertionError(
                    f"upper-frame number eligibility changed for state "
                    f"{state}: {digest}\n{result.stdout}{result.stderr}")
        # $80:AE78-$80:AE86 flips only number overlay $0591. Direction 0
        # ($0593) must remain unflipped, while direction 6 ($0591) must flip.
        oblique_expected = {
            0: "359a6117d03bacee279f54b708dca08cd698d53b1cc8a150d16b84523d33126e",
            6: "1e8fc1430ed6118a85f6f6531878cdce991fa3051cfd1018c742437bb6649a4a",
        }
        for direction, expected_hash in oblique_expected.items():
            frame = Path(directory) / f"cleveland_18_direction_{direction}.bmp"
            result = run(args.exe, args.rom, args.pack, "--player-lab",
                         "--player-team", 4, "--player-roster", 0,
                         "--player-animation", 3, "--player-direction", direction,
                         "--frames", 19, "--dump-frame", frame)
            digest = hashlib.sha256(frame.read_bytes()).hexdigest()
            if result.returncode or digest != expected_hash:
                raise AssertionError(
                    f"oblique jersey-number flip changed at direction "
                    f"{direction}: {digest}")
        oneal_expected = {
            0: "c7a6d7f54b144816c836f9dd8f9361c631fe934d0456708e720d74a9e11f13da",
            6: "1741fa348c967ef10ff08010872aff9d54bc62da75cd13093e1c66a6a68f1757",
        }
        for direction, expected_hash in oneal_expected.items():
            frame = Path(directory) / f"oneal_32_direction_{direction}.bmp"
            result = run(args.exe, args.rom, args.pack, "--player-lab",
                         "--player-team", 18, "--player-roster", 2,
                         "--player-animation", 3, "--player-direction", direction,
                         "--frames", 19, "--dump-frame", frame)
            digest = hashlib.sha256(frame.read_bytes()).hexdigest()
            if result.returncode or digest != expected_hash:
                raise AssertionError(
                    f"O'Neal oblique number changed at direction "
                    f"{direction}: {digest}")
        for team, player, label, expected_hash in (
                (10, 4, "workman", "9b819907167cf7267fecefe3eb2618ef84507a35b65142f0fa27ba27a56fc49b"),
                (12, 11, "rambis", "c94befbf47a3f0daef9271d3dbcd89779050fed3f0be2e1125a4d44f3d37b355")):
            frame = Path(directory) / f"{label}_head.bmp"
            result = run(args.exe, args.rom, args.pack, "--player-lab",
                         "--player-team", team, "--player-roster", player,
                         "--player-animation", 3, "--player-direction", 3,
                         "--frames", 19, "--dump-frame", frame)
            digest = hashlib.sha256(frame.read_bytes()).hexdigest()
            if result.returncode or digest != expected_hash:
                raise AssertionError(
                    f"high head-family render changed for {label}: {digest}")
        side_hashes = []
        for direction in (1, 5):
            frame = Path(directory) / f"side_{direction}.bmp"
            result = run(args.exe, args.rom, args.pack, "--player-lab",
                         "--player-team", 3, "--player-roster", 2,
                         "--player-animation", 3, "--player-direction", direction,
                         "--frames", 19, "--dump-frame", frame)
            if result.returncode:
                raise AssertionError(result.stdout + result.stderr)
            side_hashes.append(hashlib.sha256(frame.read_bytes()).hexdigest())
        if side_hashes[0] == side_hashes[1]:
            raise AssertionError("mirrored side directions lost the ROM flip state")

    invalid = run(args.exe, args.rom, args.pack, "--player-lab", "--player-team", 29)
    if invalid.returncode == 0 or "team must be 0..28" not in invalid.stderr:
        raise AssertionError("invalid Player Lab team was accepted")
    wrapped = run(args.exe, args.rom, args.pack, "--player-lab", "--player-team", 28,
                  "--player-roster", 11, "--player-team-right", 1,
                  "--player-roster-down", 1, "--frames", 2)
    if wrapped.returncode or "team=00" not in wrapped.stdout or \
       "roster=00" not in wrapped.stdout:
        raise AssertionError("Player Lab team/roster navigation did not wrap")
    cycled = run(args.exe, args.rom, args.pack, "--player-lab",
                  "--player-animation", 3, "--player-direction", 6,
                  "--player-animation-right", 1,
                  "--player-direction-right", 1, "--frames", 2)
    if cycled.returncode or "animation=$04" not in cycled.stdout or \
       "direction=7" not in cycled.stdout:
        raise AssertionError("Player Lab animation/direction cycling failed")
    print("[PLAYER LAB TEST] PASS: ROM rosters, descriptor animations, directions, frame hashes")


if __name__ == "__main__":
    main()
