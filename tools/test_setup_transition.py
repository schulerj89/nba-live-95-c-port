"""Regression checks for the ROM-driven title -> Game Setup handoff."""

import argparse
import hashlib
import re
import struct
import subprocess
import tempfile
import wave
from pathlib import Path

import numpy as np
from PIL import Image
from audio_fingerprint import assert_wav_fingerprint, wav_fingerprint


EXPECTED_RGB_SHA256 = {
    # Frames passed to --setup-only. 104 is the final forced-blank frame;
    # 105 releases $80:A2BF. 125/128 guard the formerly corrupt wrapped
    # construction cells; 130 is pixel-exact with Mesen transition frame 132.
    104: "2cbbeef1249170a43854962fa5b19fba628470c70beb9ce23e15a0f05cb891f2",
    105: "a916ad913f486038f1811c8d15b8e7bc539476d7d49e37a45eb2e44f2289e2f8",
    118: "6fdb16a76df0a6724650a250ecb0932ca7f78dd831ede75a28acca03ebe2a6fb",
    125: "17b0b585205691998a2e9c5238ed919f874a90094afd8e51cd8025a4309377f5",
    128: "160ca9f0c0e602e43fa77116e9693179dbc79ca7121ff7383199c1970948c17e",
    130: "95e6190d88f4cbe4a6edb85ca1d4e8bc24870ff4cd09b3b9b2affd73a5666489",
    146: "047185a6c2ffb0c4f079f0984ebb6f04afeaa3220c651de7812d1e710df310a2",
    162: "e7f61a0f21ca67bf4f3833ddbaa13c9e5501e04d1fd57f6bf3f54a0dc2d1719f",
    166: "51ef64c72ae13fc1c37e15a2cf9c3a913ccce788e9547c61f246b75bacdef416",
}
EXPECTED_AUDIO_RMS_EIGHTHS = [
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6123, 4989, 3945,
    3471, 1069, 1139, 888, 1505, 811, 404, 4485, 2990, 4451, 2948, 4595,
    3179, 5502, 3725, 1098, 4764, 4607, 3960, 4541, 4196, 2192, 2887,
    3940, 3612, 3854, 4021, 3740, 5273, 5603, 4133, 3521, 1826, 1086,
    903, 1483, 755, 404, 4160, 3388, 4422, 3201, 4388, 3193, 5181, 4132,
    1219, 4300, 4395, 4198, 4282, 4748, 2550, 2683, 4479, 3927, 4439,
    3793, 3971, 3214, 5939, 4129,
]
EXPECTED_AUDIO_BAND_PPM = [889127, 48767, 32034, 14587, 14255, 1068, 162]
EXPECTED_AUDIO_CHANNEL_RMS = [3363, 3363]

# Compact oracle derived from the independently recorded Mesen WAV beginning
# at its last-title-fade boundary (198.32 s). Raw emulator PCM is not shipped.
MESEN_SETUP_RMS_EIGHTHS = [
    155, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5213, 5602, 4033,
    3584, 1545, 1101, 910, 1468, 822, 397, 4229, 3257, 4183, 3206, 4353,
    3334, 5263, 3918, 986, 4301, 4664, 3808, 4546, 4394, 2432, 2202,
    4156, 3310, 4006, 3799, 3903, 4675, 5940, 4077, 3620, 2127, 1063,
    900, 1451, 793, 398, 3952, 3435, 4264, 3294, 4284, 3375, 5019, 4074,
    1616, 4221, 4304, 4166, 4171, 4577, 2802, 1190, 4904, 3803, 4399,
    3805, 3962, 2949, 5902, 4307,
]
EXPECTED_CURSOR_SHA256 = {
    0: "e3ec329dc39626391b9315e7ff60bf4c60b9a31b23d903bdcdca22ac5738fc65",
    1: "efc1e6e70a979c44ce821752be4a2eab6afff558ced34ad2fcb1aae0ac5c787b",
    2: "9fcfb2c5ec4c2fe5245c8f0ba166aa771f18ea27eb433f618a81c49b2b600ad8",
    3: "05567ed02467a3b7248729e6ca591c26f417274d1a38bbd3c14b5bcc2cc970ce",
    4: "ca995057d4f8848287c0422f307dd31330297eccb82019bbce7b0e7baa7c2354",
    5: "80d54ec7fc3fde9e6ad83a976b3b342c90ad59f06ddc24f59cf7f669b2b823c8",
    6: "e3ec329dc39626391b9315e7ff60bf4c60b9a31b23d903bdcdca22ac5738fc65",
}
EXPECTED_MENU_RGB_SHA256 = {
    "rules": "2354452cb07daab0f7fc454c21cb4315b4fae1d637b230027d9dea42f602e3d7",
    "options": "6221dbafae1202984eec2c1f87cddea6f341c7e24e1c4242fa9503086bf23b8f",
}
EXPECTED_MENU_ACTION_SHA256 = {
    ("rules", 2, 1): "c3075265ec452abd1cec93dabc8285547dba93118d12b4053f008502c0b4ed04",
    ("options", 0, 1): "37681e073428d19d3de63bb3d1ff156395cca02ed54acf2af504f9f0d098ab47",
    ("rules", 9, 0): "db75a123ac48bff3fb9ebf6bd980fb62e904a7f4c7025c0e783c883b40c3adf9",
    ("rules", 12, 0): "77e629913615ee5233c984aa0875b1b36e3c8b47ccb8e1861bbbdb294a656778",
}
MESEN_MENU_PRIMITIVE_SHA256 = {
    "rules_bar": "6a08a0e635e0d0ae5c24cafbaf3507fd16ce61c0f3adc0c282a8b620b36f48c8",
    "options_bar": "bda182a61baf1f225317538bd3e4d4ddae969498ae47fd40794504342a8708cb",
    "down_arrow": "92409e5622a90c322305da2be37e4e34cd4fd895ad008131dbf24e49de94631b",
}
EXPECTED_MENU_SFX_SHA256 = {
    120: "da27ef1aacd1e96b71b40e6b0baacd0a8fa4f02ddae8754d3fc620602670eaae",
    121: "da27ef1aacd1e96b71b40e6b0baacd0a8fa4f02ddae8754d3fc620602670eaae",
    122: "303c2a89000b675eb48a42160c8d233d2a28ab728d8b7645628886bd224cb890",
}
EXPECTED_MENU_ASSET_SHA256 = {
    124: "acc87f5139c463275742a378f966c64cc030b40f9712dc0e7329ddc57e622b31",
    125: "c2a8ce0b568da6af32774eb7fdc6845947841843c46d99e2fa555571d474933b",
    126: "0d25909881fe03449acf046c2d3a8cfaa64172f596864c3523c08806b581f89d",
    127: "c712e398c1060f25428b322a26ba52df8af1c86bf645ba1730c87805b7fb2f33",
}
SETUP_LOOP_START = 2053956
SETUP_LOOP_END = 4048365


def load_pack(path):
    data = Path(path).read_bytes()
    if len(data) < 16 or data[:8] != b"NBA95PAK":
        raise AssertionError("invalid asset pack")
    version, count = struct.unpack_from("<II", data, 8)
    if version != 5 or 16 + count * 24 > len(data):
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
    required = {16, 17, 88, 89, 90, 91, 92, 93, 120, 121, 122,
                124, 125, 126, 127}
    if not required.issubset(assets):
        raise AssertionError(f"missing Setup assets: {sorted(required - assets.keys())}")
    if len(assets[16]) != 0x10000 or len(assets[17]) != 0x200:
        raise AssertionError("invalid Setup VRAM/CGRAM size")
    for asset_id, expected_hash in EXPECTED_MENU_ASSET_SHA256.items():
        actual_hash = hashlib.sha256(assets[asset_id]).hexdigest()
        if actual_hash != expected_hash:
            raise AssertionError(f"Set Rules/Options asset {asset_id} changed")
    for asset_id in (120, 121, 122):
        if assets[asset_id][:4] != b"RIFF":
            raise AssertionError(f"menu SFX asset {asset_id} is not an F11 WAV")
        if hashlib.sha256(assets[asset_id]).hexdigest() != EXPECTED_MENU_SFX_SHA256[asset_id]:
            raise AssertionError(f"menu SFX asset {asset_id} content changed")
    if len(assets[88]) != 0x10000 or len(assets[89]) != 0x80:
        raise AssertionError("invalid Setup SPC RAM/DSP size")
    if assets[90][:8] != b"NBTSSPC1" or assets[91][:8] != b"NBTSAPU1":
        raise AssertionError("invalid Setup SPC asset format")
    if assets[92][:8] != b"NBSPPU1\0":
        raise AssertionError("invalid Setup entrance PPU trace")
    if assets[93][:8] != b"NBTSDSP1":
        raise AssertionError("invalid Setup S-DSP trace")
    if any(blob[:4] == b"RIFF" for blob in
           (assets[88], assets[89], assets[90], assets[91], assets[93])):
        raise AssertionError("recorded Setup WAV returned")

    version, frames, writes = struct.unpack_from("<III", assets[91], 8)
    if version != 1 or frames != 9000 or writes != 289435:
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

    version, dsp_frames, dsp_writes = struct.unpack_from("<III", assets[93], 8)
    if version != 1 or dsp_frames != 9000 or dsp_writes != 114059:
        raise AssertionError(
            f"unexpected Setup S-DSP dimensions: {dsp_frames}, {dsp_writes}"
        )
    if len(assets[93]) != 20 + dsp_writes * 6:
        raise AssertionError("truncated Setup S-DSP trace")
    previous = -1
    for index in range(dsp_writes):
        cycle, register, _ = struct.unpack_from("<IBB", assets[93], 20 + index * 6)
        if cycle < previous or register >= 0x80:
            raise AssertionError(f"invalid S-DSP cycle event {index}")
        previous = cycle

    version, ppu_frames = struct.unpack_from("<II", assets[92], 8)
    if version != 1 or ppu_frames != 61:
        raise AssertionError("unexpected Setup entrance PPU dimensions")
    offset = 16
    ppu_writes = 0
    for _ in range(ppu_frames):
        if offset + 4 > len(assets[92]):
            raise AssertionError("truncated Setup entrance PPU record")
        vram_count, cgram_count = struct.unpack_from("<HH", assets[92], offset)
        offset += 4 + (vram_count + cgram_count) * 3
        ppu_writes += vram_count + cgram_count
    if offset != len(assets[92]) or ppu_writes != 3094:
        raise AssertionError("invalid Setup entrance PPU write stream")
    if b"post_ea_game_setup.wav" in raw:
        raise AssertionError("recorded Setup WAV name returned")

    # F11 must expose the exact streamed BRR directory used by Setup. Asset
    # metadata is width=SRCN, height=start, flags=loop; payload is audition WAV.
    directory = assets[88][0x200:0x278]
    entries = {}
    _, count = struct.unpack_from("<II", raw, 8)
    for index in range(count):
        entry = struct.unpack_from("<6I", raw, 16 + index * 24)
        entries[entry[0]] = entry
    for srcn in range(30):
        asset_id = 94 + srcn
        if asset_id not in assets or assets[asset_id][:4] != b"RIFF":
            raise AssertionError(f"F11 is missing Setup SRCN ${srcn:02X}")
        start, loop = struct.unpack_from("<HH", directory, srcn * 4)
        _, _, _, packed_srcn, packed_start, packed_loop = entries[asset_id]
        if (packed_srcn, packed_start, packed_loop) != (srcn, start, loop):
            raise AssertionError(f"Setup SRCN ${srcn:02X} pointer metadata changed")


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
                r"Synthesized Game Setup through ROM BRR/S-DSP: "
                r"9000 frames, 114059 cycle-timed DSP writes, peak=(\d+); "
                rf"seamless host loop {SETUP_LOOP_START}\.\.{SETUP_LOOP_END} enabled",
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
                    audio_output, 4800000, EXPECTED_AUDIO_RMS_EIGHTHS,
                    EXPECTED_AUDIO_BAND_PPM, EXPECTED_AUDIO_CHANNEL_RMS,
                    0.9966, 25000, 26000
                )
                _, features, _ = wav_fingerprint(audio_output)
                actual = np.asarray(features["rms_eighths"], dtype=float)
                oracle = np.asarray(MESEN_SETUP_RMS_EIGHTHS, dtype=float)
                active = (actual + oracle) > 0
                correlation = np.corrcoef(actual[active], oracle[active])[0, 1]
                normalized_error = np.mean(np.abs(
                    actual[active] / actual[active].mean() -
                    oracle[active] / oracle[active].mean()
                ))
                if correlation < 0.97 or normalized_error > 0.10:
                    raise AssertionError(
                        "Setup PCM no longer follows the Mesen onset oracle: "
                        f"correlation={correlation:.3f}, error={normalized_error:.3f}"
                    )
                with wave.open(str(audio_output), "rb") as wav:
                    wav.setpos(SETUP_LOOP_START - 1)
                    start = np.frombuffer(wav.readframes(2), dtype="<i2").reshape(2, 2)
                    wav.setpos(SETUP_LOOP_END - 1)
                    end = np.frombuffer(wav.readframes(2), dtype="<i2").reshape(2, 2)
                if not (np.array_equal(start[1], end[1]) and
                        np.array_equal(start[1] - start[0], end[1] - end[0])):
                    raise AssertionError("Setup musical loop seam is no longer sample-continuous")

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
        if "Synthesized Game Setup through ROM BRR/S-DSP" not in result.stdout:
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
            if "Synthesized Game Setup through ROM BRR/S-DSP" not in result.stdout:
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

        # The settled Rules/Options pages are rendered from the captured ROM
        # VRAM/CGRAM, not recreated screenshots or host fonts.
        for menu, expected_hash in EXPECTED_MENU_RGB_SHA256.items():
            output = Path(directory) / f"setup_{menu}.bmp"
            result = subprocess.run(
                [str(exe), "--headless", "--setup-only", "--setup-menu", menu,
                 "--rom", str(rom), "--assets", str(pack), "--frames", "220",
                 "--dump-frame", str(output)],
                text=True, capture_output=True, check=True,
            )
            actual = hashlib.sha256(Image.open(output).convert("RGB").tobytes()).hexdigest()
            if actual != expected_hash:
                raise AssertionError(f"Set {menu.title()} settled frame changed")
            image = Image.open(output).convert("RGB")
            bar_boxes = ((144, 82, 192, 90), (144, 100, 192, 108)) if menu == "rules" \
                else ((160, 74, 208, 82), (160, 92, 208, 100))
            bar_hash = MESEN_MENU_PRIMITIVE_SHA256[f"{menu}_bar"]
            for box in bar_boxes:
                if hashlib.sha256(image.crop(box).tobytes()).hexdigest() != bar_hash:
                    raise AssertionError(f"Set {menu.title()} bar differs from Mesen")
            if menu == "rules":
                arrow = hashlib.sha256(image.crop((19, 185, 29, 197)).tobytes()).hexdigest()
                if arrow != MESEN_MENU_PRIMITIVE_SHA256["down_arrow"]:
                    raise AssertionError("Set Rules viewport arrow differs from Mesen")

        for (menu, row, rights), expected_hash in EXPECTED_MENU_ACTION_SHA256.items():
            output = Path(directory) / f"setup_{menu}_{row}_{rights}.bmp"
            result = subprocess.run(
                [str(exe), "--headless", "--setup-only", "--setup-menu", menu,
                 "--setup-menu-row", str(row), "--setup-menu-right", str(rights),
                 "--rom", str(rom), "--assets", str(pack), "--frames", "220",
                 "--dump-frame", str(output)],
                text=True, capture_output=True, check=True,
            )
            if "Menu SFX SRCN $1C (F11 asset 122)" not in result.stdout:
                raise AssertionError(f"Set {menu.title()} did not play confirm SFX")
            if row and "Menu SFX SRCN $1B (F11 asset 121)" not in result.stdout:
                raise AssertionError(f"Set {menu.title()} did not play move SFX")
            if rights and "Menu SFX SRCN $1A (F11 asset 120)" not in result.stdout:
                raise AssertionError(f"Set {menu.title()} did not play adjust SFX")
            actual = hashlib.sha256(Image.open(output).convert("RGB").tobytes()).hexdigest()
            if actual != expected_hash:
                raise AssertionError(
                    f"Set {menu.title()} row {row} action frame changed"
                )

        # $7E:16FB is a working copy: an edit must not alter the committed
        # block until Start runs $81:D516 or $82:8CD9/$82:8D0A.
        cases = (
            ("rules", 2, False, 0, 1),
            ("rules", 2, True, 0, 0),
            ("options", 0, False, 31, 30),
            ("options", 0, True, 31, 31),
        )
        for menu, row, confirm, working, committed in cases:
            command = [
                str(exe), "--headless", "--setup-only", "--setup-menu", menu,
                "--setup-menu-row", str(row), "--setup-menu-right", "1",
                "--rom", str(rom), "--assets", str(pack), "--frames", "220",
            ]
            if confirm:
                command.append("--setup-menu-confirm")
            result = subprocess.run(command, text=True, capture_output=True, check=True)
            match = re.search(
                rf"option_row={row} working=(\d+) committed=(\d+)", result.stdout
            )
            if not match or tuple(map(int, match.groups())) != (working, committed):
                raise AssertionError(
                    f"Set {menu.title()} working/commit behavior changed: {result.stdout}"
                )
            if "Menu SFX SRCN $1A (F11 asset 120)" not in result.stdout:
                raise AssertionError(f"Set {menu.title()} adjustment SFX was not dispatched")

        # Bar rows clamp at their endpoints and do not emit command $49 when
        # the requested direction cannot change the value.
        clamp = subprocess.run(
            [str(exe), "--headless", "--setup-only", "--setup-menu", "rules",
             "--setup-menu-right", "1", "--rom", str(rom), "--assets", str(pack),
             "--frames", "220"],
            text=True, capture_output=True, check=True,
        )
        if "option_row=0 working=45 committed=45" not in clamp.stdout:
            raise AssertionError("Rules slider no longer clamps at 45")
        if "Menu SFX SRCN $1A" in clamp.stdout:
            raise AssertionError("blocked Rules slider adjustment emitted SFX")

        # $82:8DDC -> $87:8C2D applies row 1 immediately. Prove the same
        # SRCN is actually rescaled, not merely that the displayed value moved.
        sfx_peaks = []
        for row in (0, 1):
            volume_run = subprocess.run(
                [str(exe), "--headless", "--setup-only", "--setup-menu", "options",
                 "--setup-menu-row", str(row), "--setup-menu-right", "1",
                 "--rom", str(rom), "--assets", str(pack), "--frames", "220"],
                text=True, capture_output=True, check=True,
            )
            match = re.search(
                r"SRCN \$1A \(F11 asset 120\), volume=(\d+)/45 peak=(\d+)",
                volume_run.stdout,
            )
            if not match:
                raise AssertionError("menu SFX gain telemetry missing")
            sfx_peaks.append(tuple(map(int, match.groups())))
        if sfx_peaks[0][0] != 30 or sfx_peaks[1][0] != 31 or \
           sfx_peaks[1][1] <= sfx_peaks[0][1]:
            raise AssertionError(f"SFX Volume did not change PCM gain: {sfx_peaks}")

        # Thirteen Down presses wrap the 13-row Rules cursor to row zero.
        wrapped = subprocess.run(
            [str(exe), "--headless", "--setup-only", "--setup-menu", "rules",
             "--setup-menu-row", "13", "--rom", str(rom), "--assets", str(pack),
             "--frames", "220"],
            text=True, capture_output=True, check=True,
        )
        if "page=1 menu_row=0" not in wrapped.stdout:
            raise AssertionError("Rules submenu cursor no longer wraps")

        # B is consumed but ignored: the working edit remains uncommitted and
        # the page stays open, exactly as the submenu handler does.
        ignored_b = subprocess.run(
            [str(exe), "--headless", "--setup-only", "--setup-menu", "rules",
             "--setup-menu-row", "2", "--setup-menu-right", "1", "--setup-menu-b",
             "--rom", str(rom), "--assets", str(pack), "--frames", "220"],
            text=True, capture_output=True, check=True,
        )
        if "page=1 menu_row=2" not in ignored_b.stdout or \
           "option_row=2 working=0 committed=1" not in ignored_b.stdout:
            raise AssertionError("B no longer matches the ROM's ignored behavior")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--pack", required=True)
    parser.add_argument("--exe", required=True)
    parser.add_argument("--rom", required=True)
    args = parser.parse_args()
    check_pack(Path(args.pack))
    check_frames(Path(args.exe), Path(args.rom), Path(args.pack))
    print("[TEST] PASS: Setup transition, Rules/Options persistence, menu SFX assets, ROM audio")


if __name__ == "__main__":
    main()
