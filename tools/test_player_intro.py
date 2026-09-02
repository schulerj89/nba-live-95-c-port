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
    260: (229376, "56a982c2388bb12ab67264d6d055229454b55e6dde6e8a143b1ad3336f136992"),
    261: (6015784, "91120473949026d6803083ceb70bcc4e84623baa49151d10b7e6846df16ea14c"),
    264: (6144, "7888790592673fd5e9fb1f76d7c50344fb8f6853d7d21594aad9a9c588e73d0b"),
}

# Native normal-route frame 2550 contains no BG3/OBJ pixels in the top 16
# rows. Its RGB crop therefore locks the BG2/backdrop crowd band in the packed
# Orlando pregame background without treating a screenshot as extraction input.
EXPECTED_ORLANDO_TOP_RGB = \
    "6b4cda12034520a700e01fd6135f3e9d993e4a518801a94d0b59fbc2b8388c0d"

EXPECTED_LINEUP_TO_TIPOFF_HASHES = {
    5319: "568301e236eea794758cf4d6f956cdbfe23bb8adf6861599dc50f475b599caf3",
    5321: "568301e236eea794758cf4d6f956cdbfe23bb8adf6861599dc50f475b599caf3",
    5322: "2cbbeef1249170a43854962fa5b19fba628470c70beb9ce23e15a0f05cb891f2",
    5323: "8dec91def2408adbb695d7626c00d6be3169e463379b82f40303c1d2106b2215",
    5324: "9f5bbbf92a2eb7a2fdefd4382c6e635562f54363fa3f42064a06072f29358caf",
    5330: "afcdcb9c515270ec993d73b76e784a887bcbbb5d3d2b4693d3ea4fbe98bcfab9",
}


def load_pack(path):
    raw = path.read_bytes()
    if raw[:8] != b"NBA95PAK":
        raise AssertionError("invalid pack magic")
    version, count = struct.unpack_from("<II", raw, 8)
    if version != 31 or 16 + count * 24 > len(raw):
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
    orlando = Image.frombytes(
        "RGBA", (256, 224),
        courts[24 + 18 * frame_size:24 + 19 * frame_size], "raw", "BGRA")
    if hashlib.sha256(orlando.convert("RGB").crop((0, 0, 256, 16)).tobytes()).hexdigest() \
            != EXPECTED_ORLANDO_TOP_RGB:
        raise AssertionError("Orlando pregame background lost native BG2/backdrop crowd band")

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
        # $81:9756 uses each character's descriptor width. This catches both
        # small-font M/W second strips and narrow one-strip lineup glyphs;
        # the complete frame also locks the native +2,+4 variable-logo offset.
        if rgb_hash(matchup) != "25d6a923895f9cff23460feb75b52bffe63899820e31290fa777ee20061658af":
            raise AssertionError("ROM-layout visitor/VS/home presentation changed")

        ratings = Path(directory) / "ratings.bmp"
        ratings_next = Path(directory) / "ratings_next.bmp"
        run(exe, "--headless", "--rom", rom, "--assets", pack,
            "--player-setup-only", "--player-setup-confirm",
            "--frames", 800, "--dump-frame", ratings)
        run(exe, "--headless", "--rom", rom, "--assets", pack,
            "--player-setup-only", "--player-setup-confirm",
            "--frames", 812, "--dump-frame", ratings_next)
        if rgb_hash(ratings) != "78544a87568f29bec99f6705a5b060bacafddf0692fe45126ddc7bc680383ccc" or \
           rgb_hash(ratings_next) != "de8887c8612ca71949cea934f3add37793207c59e7d61a14224bccdb167e6a64":
            raise AssertionError("rating-ball thresholds, placement, or 12-frame animation changed")

        frame = Path(directory) / "lineup.bmp"
        output = run(exe, "--headless", "--rom", rom, "--assets", pack,
                     "--player-setup-only", "--player-setup-confirm",
                     "--frames", 1100, "--dump-frame", frame, "--debug-state")
        if "SCN:PLAYER_INTRO" not in output or "CARD:01/10" not in output or \
           "ROM LOOP:$87:BE92" not in output:
            raise AssertionError("Player Setup did not hand off to the lineup state")
        if rgb_hash(frame) != "694a89064d79f1eeb476d6de822dd8183b741ffd106bf9b3f659768f540b296b":
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

        tipoff_sequence = Path(directory) / "lineup_to_tipoff"
        tipoff_sequence.mkdir()
        output = run(exe, "--headless", "--rom", rom, "--assets", pack,
                     "--player-setup-only", "--player-setup-confirm",
                     "--frames", 5330, "--dump-sequence-from", 5319,
                     "--dump-sequence-dir", tipoff_sequence, "--debug-state")
        if "SCN:TIPOFF" not in output:
            raise AssertionError("unskipped final lineup card did not reach Tipoff")
        for transition_frame, expected_hash in \
                EXPECTED_LINEUP_TO_TIPOFF_HASHES.items():
            if rgb_hash(tipoff_sequence / f"frame_{transition_frame:04d}.bmp") != \
                    expected_hash:
                raise AssertionError(
                    f"unskipped lineup -> Tipoff frame {transition_frame} changed")

        away_frame = Path(directory) / "golden_state_away.bmp"
        output = run(exe, "--headless", "--rom", rom, "--assets", pack,
                     "--team-only", "--team-side-toggle", "--team-right", 5,
                     "--team-confirm", "--player-setup-confirm", "--frames", 1600,
                     "--dump-frame", away_frame, "--debug-state")
        if "TEAM L:08 R:18" not in output or "CARD:01/10" not in output or \
           rgb_hash(away_frame) != "6ba8e0a29f4bf7dc553915813ab1032bc514162426242cd739dd0636d946b0e7":
            raise AssertionError("non-default visitor portrait selection changed")

        home_frame = Path(directory) / "san_antonio_home.bmp"
        output = run(exe, "--headless", "--rom", rom, "--assets", pack,
                     "--team-only", "--team-right", 5, "--team-confirm",
                     "--player-setup-confirm", "--frames", 3600,
                     "--dump-frame", home_frame, "--debug-state")
        if "TEAM L:03 R:23" not in output or "CARD:06/10" not in output or \
           rgb_hash(home_frame) != "72ac6e4619be39c66fa7e0ef60be7999e838c802e66a79491f49127f3677fc4b":
            raise AssertionError("non-default home portrait selection changed")

    print("Player Introduction regression checks passed")


if __name__ == "__main__":
    main()
