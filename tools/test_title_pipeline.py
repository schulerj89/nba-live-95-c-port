"""Regression checks for the ROM-driven NBA Live '95 title pipeline."""

import argparse
import hashlib
import struct
import subprocess
import tempfile
from pathlib import Path

from PIL import Image
from audio_fingerprint import assert_wav_fingerprint


EXPECTED_CUES = [(235, 1), (469, 2), (586, 3), (703, 4), (820, 5), (938, 6)]
EXPECTED_RGB_SHA256 = {
    # Filled from Mesen-verified, software-PPU output. These guard the static
    # title/light-cycle states plus active and settled credit raster states.
    840: "94aeb0c07ec8bcbba98532e6322618af8fb574bae6af08dbea90951a1d3c4967",
    960: "1df6a727a484fef0a735f3e2c0acf47e2f7a8e3189ac11957f5debc600b9785e",
    1320: "f93474ec0ed7002fa9c1a12cf821bd123d4152d68cae1ce421eb0a3be24afc3e",
    1440: "43a3c225bc5a7a1793474c761e1250b895811c630d4e54c3417d7b3f7f7fbfd9",
}
EXPECTED_AUDIO_RMS = [909, 841, 1133, 1089, 1674, 1266, 966, 1393, 1512, 1601]


def load_pack(path):
    data = Path(path).read_bytes()
    if len(data) < 16 or data[:8] != b"NBA95PAK":
        raise AssertionError("invalid asset pack")
    version, count = struct.unpack_from("<II", data, 8)
    if version != 1 or 16 + count * 24 > len(data):
        raise AssertionError("invalid asset directory")
    assets = {}
    for index in range(count):
        asset_id, offset, size, width, height, flags = struct.unpack_from(
            "<6I", data, 16 + index * 24
        )
        if offset + size > len(data):
            raise AssertionError(f"asset {asset_id} extends beyond pack")
        if asset_id in assets:
            raise AssertionError(f"duplicate asset ID {asset_id}")
        assets[asset_id] = data[offset : offset + size]
    return data, assets


def check_pack(pack_path, repo):
    raw, assets = load_pack(pack_path)
    for retired in (12, 13, 14, 15):
        if retired in assets:
            raise AssertionError(f"retired screenshot/WAV asset {retired} returned")
    if b"NBTITLE1" in raw or b"RIFF" == assets.get(80, b"")[:4]:
        raise AssertionError("recorded title video/audio shortcut returned")

    required = {80, 81, 82, 83, 84, 85, 86, 87}
    if not required.issubset(assets):
        raise AssertionError(f"missing title assets: {sorted(required - assets.keys())}")
    if len(assets[80]) != 0x10000 or len(assets[81]) != 0x200:
        raise AssertionError("invalid title VRAM/CGRAM size")
    if assets[82][:8] != b"NBTPPU1\0" or assets[85][:8] != b"NBTSPC1\0":
        raise AssertionError("invalid title hardware-state format")
    if len(assets[83]) != 0x10000 or len(assets[84]) != 0x80:
        raise AssertionError("invalid title SPC RAM/DSP size")

    apu = assets[86]
    if apu[:8] != b"NBTAPU1\0":
        raise AssertionError("invalid APU trace")
    _, frames, writes = struct.unpack_from("<III", apu, 8)
    if frames != 2160 or writes != 43777 or len(apu) != 20 + writes * 4:
        raise AssertionError(f"unexpected APU trace dimensions: {frames}, {writes}")

    cue = assets[87]
    if cue[:8] != b"NBTCUE1\0":
        raise AssertionError("invalid cue trace")
    _, cue_frames, cue_count = struct.unpack_from("<III", cue, 8)
    cues = []
    for index in range(cue_count):
        frame, value, _ = struct.unpack_from("<HBB", cue, 20 + index * 4)
        cues.append((frame, value))
    if cue_frames != 2160 or cues[:6] != EXPECTED_CUES:
        raise AssertionError(f"primary cue regression: {cues[:6]}")
    if not {7, 8, 9, 10}.issubset({value for _, value in cues[6:]}):
        raise AssertionError("credit/attract cues are incomplete")

    source_text = "\n".join(
        (repo / relative).read_text(encoding="utf-8")
        for relative in ("src/nba_title_sequence.c", "src/nba_game.c", "tools/extract_assets.py")
    )
    for retired_text in ("NBTITLE1", "post_ea_title_sequence.wav", "NBA Live 95 (USA).avi"):
        if retired_text in source_text:
            raise AssertionError(f"retired title shortcut remains in source: {retired_text}")


def check_frames(exe, rom, pack):
    with tempfile.TemporaryDirectory(prefix="nba95-title-test-") as directory:
        for frame, expected_hash in EXPECTED_RGB_SHA256.items():
            output = Path(directory) / f"title_{frame}.bmp"
            audio_output = Path(directory) / "title_runtime.wav"
            command = [str(exe), "--headless", "--title-only", "--rom", str(rom),
                       "--assets", str(pack), "--frames", str(frame),
                       "--dump-frame", str(output)]
            if frame == min(EXPECTED_RGB_SHA256):
                command.extend(["--dump-audio", str(audio_output)])
            result = subprocess.run(command, text=True, capture_output=True, check=True)
            if "Synthesized title through SPC700/S-DSP" not in result.stdout:
                raise AssertionError("title did not use the SPC700/S-DSP runtime path")
            rgb = Image.open(output).convert("RGB").tobytes()
            actual_hash = hashlib.sha256(rgb).hexdigest()
            if actual_hash != expected_hash:
                raise AssertionError(
                    f"title frame {frame} changed: {actual_hash} != {expected_hash}"
                )
            if frame == min(EXPECTED_RGB_SHA256):
                assert_wav_fingerprint(
                    audio_output, 1152000, EXPECTED_AUDIO_RMS, 13000, 15000
                )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--pack", required=True)
    parser.add_argument("--exe", required=True)
    parser.add_argument("--rom", required=True)
    args = parser.parse_args()
    repo = Path(__file__).resolve().parent.parent
    check_pack(Path(args.pack), repo)
    check_frames(Path(args.exe), Path(args.rom), Path(args.pack))
    print("[TEST] PASS: ROM title assets, cue schedule, SPC path, and frame hashes")


if __name__ == "__main__":
    main()
