"""Regression checks for the ROM-driven title -> Game Setup handoff."""

import argparse
import hashlib
import re
import struct
import subprocess
import tempfile
from pathlib import Path

from PIL import Image
from audio_fingerprint import assert_wav_fingerprint


EXPECTED_RGB_SHA256 = {
    # Frames passed to --setup-only. 104 is the final forced-blank frame;
    # 105 releases $80:A2BF. Frames 146/166 match Mesen frames 1784/1804.
    104: "2cbbeef1249170a43854962fa5b19fba628470c70beb9ce23e15a0f05cb891f2",
    105: "fc55ce12b6688bea4700cdc3c65cbf84dcf860658a774f0c03b741decfa33459",
    118: "b74d254c5419713f36925c96faf4e9c5dac5f6432a01cb0a8484a0e95caa0c85",
    146: "047185a6c2ffb0c4f079f0984ebb6f04afeaa3220c651de7812d1e710df310a2",
    162: "e7f61a0f21ca67bf4f3833ddbaa13c9e5501e04d1fd57f6bf3f54a0dc2d1719f",
    166: "51ef64c72ae13fc1c37e15a2cf9c3a913ccce788e9547c61f246b75bacdef416",
}
EXPECTED_AUDIO_RMS = [0, 769, 888, 1186, 1123, 1484, 654, 1381, 1428, 1445]
EXPECTED_CURSOR_SHA256 = {
    0: "e3ec329dc39626391b9315e7ff60bf4c60b9a31b23d903bdcdca22ac5738fc65",
    1: "efc1e6e70a979c44ce821752be4a2eab6afff558ced34ad2fcb1aae0ac5c787b",
    2: "9fcfb2c5ec4c2fe5245c8f0ba166aa771f18ea27eb433f618a81c49b2b600ad8",
    3: "05567ed02467a3b7248729e6ca591c26f417274d1a38bbd3c14b5bcc2cc970ce",
    4: "ca995057d4f8848287c0422f307dd31330297eccb82019bbce7b0e7baa7c2354",
    5: "80d54ec7fc3fde9e6ad83a976b3b342c90ad59f06ddc24f59cf7f669b2b823c8",
    6: "e3ec329dc39626391b9315e7ff60bf4c60b9a31b23d903bdcdca22ac5738fc65",
}


def load_pack(path):
    data = Path(path).read_bytes()
    if len(data) < 16 or data[:8] != b"NBA95PAK":
        raise AssertionError("invalid asset pack")
    version, count = struct.unpack_from("<II", data, 8)
    if version != 1 or 16 + count * 24 > len(data):
        raise AssertionError("invalid asset directory")
    assets = {}
    for index in range(count):
        asset_id, offset, size, _, _, _ = struct.unpack_from(
            "<6I", data, 16 + index * 24
        )
        if asset_id in assets:
            raise AssertionError(f"duplicate asset ID {asset_id}")
        if offset + size > len(data):
            raise AssertionError(f"asset {asset_id} extends beyond pack")
        assets[asset_id] = data[offset : offset + size]
    return data, assets


def check_pack(pack_path):
    raw, assets = load_pack(pack_path)
    required = {16, 17, 88, 89, 90, 91}
    if not required.issubset(assets):
        raise AssertionError(f"missing Setup assets: {sorted(required - assets.keys())}")
    if len(assets[16]) != 0x10000 or len(assets[17]) != 0x200:
        raise AssertionError("invalid Setup VRAM/CGRAM size")
    if len(assets[88]) != 0x10000 or len(assets[89]) != 0x80:
        raise AssertionError("invalid Setup SPC RAM/DSP size")
    if assets[90][:8] != b"NBTSSPC1" or assets[91][:8] != b"NBTSAPU1":
        raise AssertionError("invalid Setup SPC asset format")
    if any(blob[:4] == b"RIFF" for blob in (assets[88], assets[89], assets[90], assets[91])):
        raise AssertionError("recorded Setup WAV returned")

    version, frames, writes = struct.unpack_from("<III", assets[91], 8)
    if version != 1 or frames != 1800 or writes != 102445:
        raise AssertionError(f"unexpected Setup APU dimensions: {frames}, {writes}")
    if len(assets[91]) != 20 + writes * 6:
        raise AssertionError("truncated Setup APU trace")
    previous = -1
    for index in range(writes):
        cycle, port, _ = struct.unpack_from("<IBB", assets[91], 20 + index * 6)
        if cycle < previous or port > 3:
            raise AssertionError(f"invalid cycle event {index}")
        previous = cycle
    if previous > frames * 1024000 // 60:
        raise AssertionError("Setup APU trace exceeds its declared duration")
    if b"post_ea_game_setup.wav" in raw:
        raise AssertionError("recorded Setup WAV name returned")


def check_frames(exe, rom, pack):
    with tempfile.TemporaryDirectory(prefix="nba95-setup-test-") as directory:
        for frame, expected_hash in EXPECTED_RGB_SHA256.items():
            output = Path(directory) / f"setup_{frame}.bmp"
            audio_output = Path(directory) / "setup_runtime.wav"
            command = [str(exe), "--headless", "--setup-only", "--rom", str(rom),
                       "--assets", str(pack), "--frames", str(frame),
                       "--dump-frame", str(output)]
            if frame == min(EXPECTED_RGB_SHA256):
                command.extend(["--dump-audio", str(audio_output)])
            result = subprocess.run(
                command,
                text=True, capture_output=True, check=True,
            )
            match = re.search(
                r"Synthesized Game Setup through SPC700/S-DSP: "
                r"1800 frames, 102445 cycle-timed APU writes, peak=(\d+)",
                result.stdout,
            )
            if not match or int(match.group(1)) == 0:
                raise AssertionError("Game Setup audio was not synthesized from the SPC assets")
            actual = hashlib.sha256(Image.open(output).convert("RGB").tobytes()).hexdigest()
            if actual != expected_hash:
                raise AssertionError(
                    f"Setup transition frame {frame} changed: {actual} != {expected_hash}"
                )
            if frame == min(EXPECTED_RGB_SHA256):
                assert_wav_fingerprint(
                    audio_output, 960000, EXPECTED_AUDIO_RMS, 14000, 16000
                )

        # Exercise the real title-dismiss path as well as the direct fixture.
        # Start on title frame 0, take $80:E5C7's snap/hold/fade, preserve the
        # forced-blank loader, and land on the same first visible Setup frame.
        integrated = Path(directory) / "title_to_setup.bmp"
        result = subprocess.run(
            [str(exe), "--headless", "--title-only", "--enter-setup",
             "--rom", str(rom), "--assets", str(pack), "--frames", "243",
             "--dump-frame", str(integrated)],
            text=True, capture_output=True, check=True,
        )
        if "Synthesized Game Setup through SPC700/S-DSP" not in result.stdout:
            raise AssertionError("title handoff did not start the Setup SPC path")
        integrated_hash = hashlib.sha256(
            Image.open(integrated).convert("RGB").tobytes()
        ).hexdigest()
        if integrated_hash != EXPECTED_RGB_SHA256[105]:
            raise AssertionError("title-to-Setup integration timing changed")

        # Both $80:E5C7 paths must reach the same first visible Setup frame:
        # 120-frame snap hold while building, and 40-frame hold once complete.
        for press_frame, total_frames in ((100, 343), (1000, 1163)):
            output = Path(directory) / f"title_exit_{press_frame}.bmp"
            result = subprocess.run(
                [str(exe), "--headless", "--title-only", "--title-press",
                 str(press_frame), "--rom", str(rom), "--assets", str(pack),
                 "--frames", str(total_frames), "--dump-frame", str(output)],
                text=True, capture_output=True, check=True,
            )
            if "Synthesized Game Setup through SPC700/S-DSP" not in result.stdout:
                raise AssertionError(f"title exit at {press_frame} did not reach Setup")
            actual = hashlib.sha256(Image.open(output).convert("RGB").tobytes()).hexdigest()
            if actual != EXPECTED_RGB_SHA256[105]:
                raise AssertionError(f"title exit timing changed for press {press_frame}")

        # All six HDMA highlight rows plus one wrap back to row zero.
        for down_count, expected_hash in EXPECTED_CURSOR_SHA256.items():
            output = Path(directory) / f"setup_cursor_{down_count}.bmp"
            subprocess.run(
                [str(exe), "--headless", "--setup-only", "--setup-down",
                 str(down_count), "--rom", str(rom), "--assets", str(pack),
                 "--frames", "200", "--dump-frame", str(output)],
                text=True, capture_output=True, check=True,
            )
            actual = hashlib.sha256(Image.open(output).convert("RGB").tobytes()).hexdigest()
            if actual != expected_hash:
                raise AssertionError(f"Setup cursor row {down_count} changed")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--pack", required=True)
    parser.add_argument("--exe", required=True)
    parser.add_argument("--rom", required=True)
    args = parser.parse_args()
    check_pack(Path(args.pack))
    check_frames(Path(args.exe), Path(args.rom), Path(args.pack))
    print("[TEST] PASS: forced blank, Setup scroll staging, asset-pack SPC audio")


if __name__ == "__main__":
    main()
