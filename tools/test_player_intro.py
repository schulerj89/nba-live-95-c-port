"""Regression checks for Player Setup -> Starting Lineup presentation."""

import argparse
import hashlib
import struct
import subprocess
import tempfile
from pathlib import Path

from PIL import Image
from audio_fingerprint import assert_wav_fingerprint

EXPECTED_AUDIO_RMS = [
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2311, 251, 2219, 1657, 2673, 387,
    3879, 5371, 4376, 4198, 3915, 5455, 3747, 4886, 5206, 3277, 1072,
    3229, 3239, 3041, 2540, 3237, 4635, 6171, 3933, 4531, 3302, 5465,
    3876, 4935, 4494, 5133, 2970, 4870, 1765, 4246, 4441, 4546, 4252,
    3594, 5256, 4022, 4298, 3492, 5665, 3697, 4972, 5431, 3304, 880,
    3274, 1215, 2301, 2471, 3250, 4823, 5574, 3877, 5127, 4287, 5471,
    4037, 4818, 4938, 4950, 2877, 4432, 1028, 3745, 4208,
]

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
    if version != 30 or 16 + count * 24 > len(raw):
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

    for asset_id, size, magic in (
            (265, 0x10000, None), (266, 0x80, None),
            (267, 20, b"NBPISPC1"), (268, None, b"NBPIDSP1")):
        payload = assets[asset_id][0]
        if (size is not None and len(payload) != size) or \
                (magic is not None and payload[:8] != magic):
            raise AssertionError(f"Player Introduction audio asset {asset_id} changed")
    trace = assets[268][0]
    version, frames, event_count = struct.unpack_from("<III", trace, 8)
    if version != 1 or not 5000 <= frames <= 6000 or \
            len(trace) != 20 + event_count * 6 or event_count < 100:
        raise AssertionError("Player Introduction DSP program dimensions changed")
    font, width, height, flags = assets[269]
    if len(font) != 0x1000 or (width, height, flags) != (8, 16, 0xA98000) or \
            font[:6] != b"\x10\x00\x0e\x00\x01\x02":
        raise AssertionError("Player Introduction ROM font descriptor changed")
    font, width, height, flags = assets[270]
    if len(font) != 0x1000 or (width, height, flags) != (16, 16, 0xA6BB16) or \
            font[:6] != b"\x10\x00\x10\x00\x00\x02":
        raise AssertionError("Starting Lineup ROM font descriptor changed")

    courts, width, height, flags = assets[271]
    frame_size = 256 * 224 * 4
    if courts[:8] != b"NBCOURT1" or \
            struct.unpack_from("<IIII", courts, 8) != (1, 29, 256, 224) or \
            (width, height, flags, len(courts)) != \
            (256, 224, 29, 24 + 29 * frame_size):
        raise AssertionError("home-court catalog header/dimensions changed")
    court_hashes = {
        hashlib.sha256(courts[24 + team * frame_size:
                              24 + (team + 1) * frame_size]).hexdigest()
        for team in range(29)
    }
    if len(court_hashes) != 28 or \
            hashlib.sha256(courts[24 + 18 * frame_size:
                                  24 + 19 * frame_size]).hexdigest() != \
            EXPECTED_ASSETS[260][1]:
        raise AssertionError("home-team courts are not unique/ROM-ordered")

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
        audio = Path(directory) / "player_intro.wav"
        output = run(exe, "--headless", "--rom", rom, "--assets", pack,
                     "--player-setup-only", "--player-setup-confirm",
                     "--frames", 500, "--dump-frame", matchup,
                     "--dump-audio", audio)
        if "Synthesized Player Introduction through ROM BRR/S-DSP: " \
                "5560 frames, 48271 cycle-timed DSP writes, peak=25983" not in output:
            raise AssertionError("Player Introduction did not start its ROM SPC path")
        assert_wav_fingerprint(
            audio, 2965333, EXPECTED_AUDIO_RMS,
            [921799, 34924, 13747, 8129, 19548, 1564, 290],
            [3760, 3760], 1.0, 25000, 27000)
        if rgb_hash(matchup) != "55fee4c1f2ba099b0682abd830beda56b144c5c1cade00914dde760ecaf25973":
            raise AssertionError("ROM-layout visitor/VS/home presentation changed")

        ratings = Path(directory) / "ratings.bmp"
        ratings_next = Path(directory) / "ratings_next.bmp"
        run(exe, "--headless", "--rom", rom, "--assets", pack,
            "--player-setup-only", "--player-setup-confirm",
            "--frames", 800, "--dump-frame", ratings)
        run(exe, "--headless", "--rom", rom, "--assets", pack,
            "--player-setup-only", "--player-setup-confirm",
            "--frames", 812, "--dump-frame", ratings_next)
        if rgb_hash(ratings) != "a358ee549b0f06d5ddf3d98ea2a4cf8016501fd1ad113c4ee990cd665eca1eae" or \
           rgb_hash(ratings_next) != "956729d5656ffa97e46d2c27e54859f155ef2463053577eb8b5b986ea2ba548b":
            raise AssertionError("rating-ball thresholds, placement, or 12-frame animation changed")

        frame = Path(directory) / "lineup.bmp"
        output = run(exe, "--headless", "--rom", rom, "--assets", pack,
                     "--player-setup-only", "--player-setup-confirm",
                     "--frames", 1100, "--dump-frame", frame, "--debug-state")
        if "SCN:PLAYER_INTRO" not in output or "CARD:01/10" not in output or \
           "ROM LOOP:$87:BE92" not in output:
            raise AssertionError("Player Setup did not hand off to the lineup state")
        if rgb_hash(frame) != "40a35ddd7b828401c2cf9702ce973bf57e7761117c27912bef7675276cbe1d7b":
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
           rgb_hash(away_frame) != "0150574f6ccb502d45747766dcbbaf8c755bd0af2ebafc9156d62296866dfec9":
            raise AssertionError("non-default visitor portrait selection changed")

        home_frame = Path(directory) / "san_antonio_home.bmp"
        output = run(exe, "--headless", "--rom", rom, "--assets", pack,
                     "--team-only", "--team-right", 5, "--team-confirm",
                     "--player-setup-confirm", "--frames", 3600,
                     "--dump-frame", home_frame, "--debug-state")
        if "TEAM L:03 R:23" not in output or "CARD:06/10" not in output or \
           rgb_hash(home_frame) != "0674f5cb327859fb972ff6a6e3a5ad2f9f29d2d97cd5b212f08eae7a2adfaec5":
            raise AssertionError("non-default home portrait selection changed")

    print("Player Introduction regression checks passed")


if __name__ == "__main__":
    main()
