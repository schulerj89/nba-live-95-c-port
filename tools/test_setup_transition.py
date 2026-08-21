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
EXPECTED_ASSET_DEBUGGER_SHA256 = "e5a981af3fc5e60c5553c4fa0eb9abbeb276f5e7bf1eedbf6878f26194425d9e"
EXPECTED_OAM_DEBUGGER_SHA256 = "154e59fbf555636e7702fced12ba3cf7b61370a07331a1b80da8a95e2951c0fe"
EXPECTED_RENDERED_MENU_SFX_SHA256 = {
    0x1A: "447a1ea48a94e2036ff0bdf1f4c5248d6284daec0b723b9a966f841976e703c4",
    0x1B: "96de89e954e4e8f75e555625abba5bf4380b8868b3263776a4cc27a6285de664",
    0x1C: "6c2879a2f1b4beda1318c14ae70e352953669dbc0eb60a49a8eec773908e29ee",
}
EXPECTED_MENU_SFX_PARAMETERS = {
    0x1A: ("$05A8", 9823),
    0x1B: ("$050A", 10964),
    0x1C: ("$03C6", 10662),
}

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
    "rules": "903f764776b281bd835d32365882881bdb1f4829bea0ddd1627cfc10aed688db",
    "options": "4a1bc5f69c656dc1a3e5463768a5c2b99927393faa25616dfdbf2dcee1f57a91",
}
EXPECTED_MENU_ACTION_SHA256 = {
    ("rules", 2, 1): "eba0f1d47937e6b43c11bdf964524323f35f6ae248d919dd3dc21a7fda825cfa",
    ("options", 0, 1): "1d7a804a03721d8ce5b3ac0b36c3f1a3319df0ea67f3faf9cc74441994432dd1",
    ("rules", 9, 0): "d002d712beb8840e717285522387fe6572866c986727d7f9956e26343681e29c",
    ("rules", 12, 0): "0521f812b5e52d26ad4bcd5db71539cc3c7ea9698aad9a9bb335335ddd237945",
    ("options", 2, 1): "df7af52eb402106cf05711dae5bcfb34e413594ffbf276f0710daa5f29493627",
    ("options", 2, 2): "6b22913aea79680578fe373403376feb66b654757c7e004bc011e0aa89f21bdb",
    ("options", 5, 1): "4f99b6655e84b48094caad2f01626e6df84783f1f422169b4c76b3421140deec",
}
EXPECTED_SUBMENU_TRANSITION_SHA256 = {
    ("open", 198): "4e111bf6b8ceb0897f12599694924d6cb716c96564f029a3debd07d92c905cd9",
    ("open", 219): "2cbbeef1249170a43854962fa5b19fba628470c70beb9ce23e15a0f05cb891f2",
    ("open", 259): "54e592f2c7ef2881a37e523aab32bc6bc50c2d6bce7d3ccbd3d9a739ff2dcf25",
    ("open", 307): "021df816667350b1bc97c33f6229065fab2260c20a79b8c83e0e302d5d46988c",
    ("close", 329): "2cbd30f0e97d13c73ddb235402b774f41e5a4d56eeb722d9ce92adcd00442fd0",
    ("close", 345): "2cbbeef1249170a43854962fa5b19fba628470c70beb9ce23e15a0f05cb891f2",
    ("close", 382): "672bdcde253f7b6736aa2285991058ae4620cbea2c518113a14f74307a97e92e",
    ("close", 424): "b8fd74eed4359b816bd889221572c51855c38a76569dcb7f3a1892704c1576d7",
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
    125: "1bca42e96ef6f911db8e2e17d804606973b071b96024c5b67b5860c5db983601",
    126: "0d25909881fe03449acf046c2d3a8cfaa64172f596864c3523c08806b581f89d",
    127: "861ed1f614e372b99912fb3f00643019cb2180c3ebe13a048cfbc027df6a9608",
    128: "24ca31b8d80d079fda20bf619c2db522b63781f175bd93ebaaf48ab9950a76a8",
    129: "60b00e46f09e428a9fac44d41dff939c9cfab508f7e4bfa518c5991b086607e4",
    130: "42cd32dc8df82febef1936d3f1c2600e070e32797cee09690b0ea802ff529bf1",
    131: "e98b9ea84551ff95e47d3eb479d7705ba47232d994b40889eedb9762dc0cba06",
    132: "6b184355c9acc37a6c367f0ef4a52aaa6df5feb3cec26757a885cf3d2fb14849",
    133: "e4a55372bda54cce7014e114beb38fe30662f6e15437aaedad9e49d27fe54fbd",
    134: "d1446eb4b7c6f6d275a980f3b39e98129817fc84071f8151ae90b82ff79d8c60",
    135: "69dd72dc97b50cae9817effd52c29f5a9ff1bba5276221a4f5cdba1d7dbf960d",
    136: "b231ec61fa3e439ef7e9600d81d9840dfbbc2595114d4e634e62951320c12c14",
    137: "56e0c8bfb349bc77de2234141861f8fe0736723141bbd71398dc19e56db2f44e",
    138: "a037d5934c9a6b839b167828336af0b9d1bcce4bd98faa13ac32e2578fc5694f",
    139: "6d199ebab13adc93aef1ed3abcab3a77219e8d1e7d7811e89bfab3074d78256d",
    140: "abb16640b4954cc1d968305a345c3703a0447eb47fcbc5c98cc3af7d53e45887",
    141: "ed829c237970e861f130a6b7966ee0ef911623cd39d56c142238ec95432d2102",
    142: "e01ff3a82d09d77d018ed79f2b54fd7620f975b523c680690d15efeea8b6adfa",
}
EXPECTED_MAIN_VALUE_RGB_SHA256 = {
    (0, 1): "e4204d3c33d0bae2680ad2a2ac2f5cc14ceec6e4bc4e8bee2f35cad05a0efdc9",
    (0, 2): "96be8085c9ae412d13fb265a992b99a49dad540aa6a7b8f696c0b348a71c0fb4",
    (0, 3): "2f952c609272ebab6fbc1f8388a1e91671b11001590de6c0a716fbb0ed120ba7",
    (1, 1): "e6607c9e642194429b9e8de48359e82325ea18b08da7e98619f9d77bc3838a0f",
    (1, 2): "a59cc2515140ccd18c36b91bbf74177eb27f8eb552f70e2038f41d86a69bfbcc",
    (2, 1): "0d0eec20edddc55dc5ae41f4595b2b1a30e6c82050b0f14524f4adfe95a8ca04",
    (2, 2): "5a550fa69536f7dea18be1157c7f55365baa31a37db456203851e7cc5b1e0070",
    (3, 1): "e4ae718f8545e18560eebcfebf5e344773bcbda67e04f50b8e194d6671813a91",
    (3, 2): "908bc9d71553bc3809404d8175c59760dfcaad0087eb916a1e0e767ab32ad3a7",
    (3, 3): "9dfd7625c2fa4673816530a112c54d17ed3127e533375aec3a74565918473ad3",
}
MESEN_MAIN_VALUE_GLYPH_SHA256 = {
    (0, 1): "dc35ee3889bed4f3c86a90e3a0acbee1a63d9fdf94d0de173f5437dde770268d",
    (0, 2): "cecbb02cd80d6a4d416881873a326623a3bd7f544d3fba3a86a91110027e2125",
    (0, 3): "b4cd582b37282e5ae24fc9561beb10532290512aa04d2c5e9353744f05ab2f7d",
    (1, 1): "ef4d6d61b67848d8e3d5550403e8a3d95085aa46fa7c00a0e16798034b15dbfd",
    (1, 2): "c99ceee59e9b4ecd50cae573ab869e91780d0e2e4436d85c72df9a6ad62e6a85",
    (2, 1): "f31de1b931dd30ced18dccf02abea99ff8aeaa9ee67027395f91dc99108ed6d4",
    (2, 2): "a5c4bf9f2197084dc28d5001cf6c30cd6c4fc8e4e1d7b064d437770fead998c5",
    (3, 1): "74afa56375a738feda3cf44e8890466735e48d4bb2b82ae40b6f3af6b7ea16a3",
    (3, 2): "35a31c28e8684ccf2240f6e52a61d28bd3a000d929aa5b275d65c52a23fae731",
    (3, 3): "b9f40f561ec4e6e772a6d5aced4207e96412bfd49f1287cae00bf6a9e24d1d80",
}
SETUP_LOOP_START = 2053956
SETUP_LOOP_END = 4048365


def load_pack(path):
    data = Path(path).read_bytes()
    if len(data) < 16 or data[:8] != b"NBA95PAK":
        raise AssertionError("invalid asset pack")
    version, count = struct.unpack_from("<II", data, 8)
    if version != 7 or 16 + count * 24 > len(data):
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
                *range(124, 143)}
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
        # Rules/Options must never fall back to host-rendered text when a
        # mandatory game-authored variant is absent. Keep the pack structurally
        # valid but rename OFF's entry to an unused ID, then prove page entry is
        # refused.
        incomplete_raw = bytearray(pack.read_bytes())
        _, incomplete_count = struct.unpack_from("<II", incomplete_raw, 8)
        for index in range(incomplete_count):
            entry_offset = 16 + index * 24
            if struct.unpack_from("<I", incomplete_raw, entry_offset)[0] == 130:
                struct.pack_into("<I", incomplete_raw, entry_offset, 150)
                break
        else:
            raise AssertionError("canonical pack is missing OFF variant asset 130")
        incomplete_pack = Path(directory) / "missing_off_variant.pak"
        incomplete_pack.write_bytes(incomplete_raw)
        incomplete = subprocess.run(
            [str(exe), "--headless", "--setup-only", "--setup-menu", "options",
             "--frames", "450", "--rom", str(rom), "--assets", str(incomplete_pack)],
            text=True, capture_output=True, check=True,
        )
        if "[SETUP TEST] page=0" not in incomplete.stdout:
            raise AssertionError("incomplete pack opened Options with fallback graphics")

        missing_main_raw = bytearray(pack.read_bytes())
        _, main_count = struct.unpack_from("<II", missing_main_raw, 8)
        for index in range(main_count):
            entry_offset = 16 + index * 24
            if struct.unpack_from("<I", missing_main_raw, entry_offset)[0] == 133:
                struct.pack_into("<I", missing_main_raw, entry_offset, 149)
                break
        missing_main_pack = Path(directory) / "missing_season_variant.pak"
        missing_main_pack.write_bytes(missing_main_raw)
        missing_main = subprocess.run(
            [str(exe), "--headless", "--setup-only", "--setup-main-row", "0",
             "--setup-main-right", "1", "--frames", "200", "--rom", str(rom),
             "--assets", str(missing_main_pack)],
            text=True, capture_output=True, check=True,
        )
        if "mode=0 style=1 level=0 quarter=0" not in missing_main.stdout or \
                "DSP menu SFX SRCN $1A" in missing_main.stdout:
            raise AssertionError("missing main value asset used fallback graphics/state")

        asset_debug = Path(directory) / "asset_debug_options_vram.bmp"
        subprocess.run(
            [str(exe), "--headless", "--asset-debug", "126", "--frames", "1",
             "--rom", str(rom), "--assets", str(pack), "--dump-frame", str(asset_debug)],
            text=True, capture_output=True, check=True,
        )
        asset_debug_hash = hashlib.sha256(
            Image.open(asset_debug).convert("RGB").tobytes()
        ).hexdigest()
        if asset_debug_hash != EXPECTED_ASSET_DEBUGGER_SHA256:
            raise AssertionError("F12 ROM asset debugger rendering changed")

        oam_debug = Path(directory) / "asset_debug_rules_oam.bmp"
        subprocess.run(
            [str(exe), "--headless", "--asset-debug", "128", "--frames", "1",
             "--rom", str(rom), "--assets", str(pack), "--dump-frame", str(oam_debug)],
            text=True, capture_output=True, check=True,
        )
        if hashlib.sha256(Image.open(oam_debug).convert("RGB").tobytes()).hexdigest() != \
                EXPECTED_OAM_DEBUGGER_SHA256:
            raise AssertionError("F12 OAM/OBJ asset reconstruction changed")

        for srcn, expected_hash in EXPECTED_RENDERED_MENU_SFX_SHA256.items():
            output = Path(directory) / f"menu_sfx_{srcn:02x}.wav"
            result = subprocess.run(
                [str(exe), "--headless", "--rom", str(rom), "--assets", str(pack),
                 "--frames", "0", "--menu-sfx-srcn", hex(srcn),
                 "--dump-menu-sfx", str(output)],
                text=True, capture_output=True, check=True,
            )
            pitch, peak = EXPECTED_MENU_SFX_PARAMETERS[srcn]
            if (f"SRCN ${srcn:02X} pitch={pitch} ADSR1/2=$8E/$E0, "
                    f"volume=30/45 DSPVOL=$40 peak={peak}.") not in result.stdout:
                raise AssertionError(f"menu SRCN ${srcn:02X} DSP parameters changed")
            if hashlib.sha256(output.read_bytes()).hexdigest() != expected_hash:
                raise AssertionError(f"menu SRCN ${srcn:02X} PCM changed")
            with wave.open(str(output), "rb") as wav:
                if (wav.getnchannels(), wav.getsampwidth(), wav.getframerate(),
                        wav.getnframes()) != (2, 2, 32000, 24000):
                    raise AssertionError(f"menu SRCN ${srcn:02X} WAV shape changed")

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

        # Main Setup writes four 16-bit values at $7E:16FB + row*2. Verify
        # every non-default state, the shared $49 adjustment sound, and a
        # compact pixel oracle derived from the corresponding Mesen capture.
        expected_states = {
            (0, 1): (1, 1, 0, 0), (0, 2): (2, 1, 0, 0),
            (0, 3): (3, 1, 0, 0), (1, 1): (0, 2, 0, 0),
            (1, 2): (0, 0, 0, 0), (2, 1): (0, 1, 1, 0),
            (2, 2): (0, 1, 2, 0), (3, 1): (0, 1, 0, 1),
            (3, 2): (0, 1, 0, 2), (3, 3): (0, 1, 0, 3),
        }
        for (row, rights), expected_hash in EXPECTED_MAIN_VALUE_RGB_SHA256.items():
            output = Path(directory) / f"setup_main_{row}_{rights}.bmp"
            result = subprocess.run(
                [str(exe), "--headless", "--setup-only", "--setup-main-row", str(row),
                 "--setup-main-right", str(rights), "--rom", str(rom),
                 "--assets", str(pack), "--frames", "200", "--dump-frame", str(output)],
                text=True, capture_output=True, check=True,
            )
            if "DSP menu SFX SRCN $1A" not in result.stdout:
                raise AssertionError(f"main Setup row {row} did not dispatch adjust SFX")
            match = re.search(
                r"\[SETUP MAIN TEST\] row=\d+ mode=(\d+) style=(\d+) "
                r"level=(\d+) quarter=(\d+)", result.stdout,
            )
            if not match or tuple(map(int, match.groups())) != expected_states[(row, rights)]:
                raise AssertionError(f"main Setup row {row} state changed: {result.stdout}")
            image = Image.open(output).convert("RGB")
            if hashlib.sha256(image.tobytes()).hexdigest() != expected_hash:
                raise AssertionError(f"main Setup row {row} value {rights} frame changed")
            top = 70 + row * 18
            glyph = bytearray()
            for y in range(top, top + 16):
                for x in range(138, 248):
                    color = image.getpixel((x, y))
                    if color[0] >= 100 and color[1] >= 50:
                        glyph.extend((x - 138, y - top, *color))
            if hashlib.sha256(glyph).hexdigest() != \
                    MESEN_MAIN_VALUE_GLYPH_SHA256[(row, rights)]:
                raise AssertionError(f"main Setup row {row} glyph differs from Mesen")

        for row, maximum, defaults in ((0, 3, (0, 1, 0, 0)),
                                       (1, 2, (0, 1, 0, 0)),
                                       (2, 2, (0, 1, 0, 0)),
                                       (3, 3, (0, 1, 0, 0))):
            wrap = subprocess.run(
                [str(exe), "--headless", "--setup-only", "--setup-main-row", str(row),
                 "--setup-main-right", str(maximum + 1), "--rom", str(rom),
                 "--assets", str(pack), "--frames", "200"],
                text=True, capture_output=True, check=True,
            )
            match = re.search(r"mode=(\d+) style=(\d+) level=(\d+) quarter=(\d+)",
                              wrap.stdout)
            if not match or tuple(map(int, match.groups())) != defaults:
                raise AssertionError(f"main Setup row {row} no longer wraps")

        left_states = {
            0: (3, 1, 0, 0), 1: (0, 0, 0, 0),
            2: (0, 1, 2, 0), 3: (0, 1, 0, 3),
        }
        for row, expected in left_states.items():
            left = subprocess.run(
                [str(exe), "--headless", "--setup-only", "--setup-main-row", str(row),
                 "--setup-main-left", "1", "--rom", str(rom), "--assets", str(pack),
                 "--frames", "200"],
                text=True, capture_output=True, check=True,
            )
            match = re.search(r"mode=(\d+) style=(\d+) level=(\d+) quarter=(\d+)",
                              left.stdout)
            if not match or tuple(map(int, match.groups())) != expected or \
                    "DSP menu SFX SRCN $1A" not in left.stdout:
                raise AssertionError(f"main Setup row {row} left cycle changed")

        persisted = subprocess.run(
            [str(exe), "--headless", "--setup-only", "--setup-main-row", "3",
             "--setup-main-right", "1", "--setup-menu", "rules",
             "--setup-menu-confirm", "--rom", str(rom), "--assets", str(pack),
             "--frames", "650"],
            text=True, capture_output=True, check=True,
        )
        if "mode=0 style=1 level=0 quarter=1" not in persisted.stdout:
            raise AssertionError("main Setup values did not survive a Rules round trip")

        reentered = subprocess.run(
            [str(exe), "--headless", "--setup-only", "--setup-main-row", "3",
             "--setup-main-right", "1", "--setup-reenter", "--rom", str(rom),
             "--assets", str(pack), "--frames", "200"],
            text=True, capture_output=True, check=True,
        )
        if "[SETUP REENTER] mode=0 style=1 level=0 quarter=1" not in reentered.stdout:
            raise AssertionError("session-owned Setup values were reset on scene re-entry")

        mode_routes = ("TEAM_SELECTION", "SEASON", "PLAYOFFS", "LOAD_SERIES")
        for mode, route in enumerate(mode_routes):
            navigation = subprocess.run(
                [str(exe), "--headless", "--setup-only", "--setup-main-row", "0",
                 "--setup-main-right", str(mode), "--setup-main-confirm",
                 "--rom", str(rom), "--assets", str(pack), "--frames", "200"],
                text=True, capture_output=True, check=True,
            )
            marker = f"Mode confirmed: mode={mode} route={route}"
            if "action=4" not in navigation.stdout or marker not in navigation.stdout or \
                    "DSP menu SFX SRCN $1C" not in navigation.stdout:
                raise AssertionError(f"main mode {mode} emitted the wrong route/action")

        # The settled Rules/Options pages are rendered from the captured ROM
        # VRAM/CGRAM, not recreated screenshots or host fonts.
        for menu, expected_hash in EXPECTED_MENU_RGB_SHA256.items():
            output = Path(directory) / f"setup_{menu}.bmp"
            result = subprocess.run(
                [str(exe), "--headless", "--setup-only", "--setup-menu", menu,
                 "--rom", str(rom), "--assets", str(pack), "--frames", "450",
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

        # The ROM does not swap pages immediately. $80:A3B8 first scrolls BG3
        # away, slides BG1/BG2 out under a 15-step fade, holds while the new
        # layer state is built, then runs the standard entrance in reverse.
        for (direction, frame), expected_hash in EXPECTED_SUBMENU_TRANSITION_SHA256.items():
            output = Path(directory) / f"submenu_{direction}_{frame}.bmp"
            command = [str(exe), "--headless", "--setup-only", "--setup-menu", "rules"]
            if direction == "close":
                command.append("--setup-menu-confirm")
            command.extend(["--rom", str(rom), "--assets", str(pack),
                            "--frames", str(frame), "--dump-frame", str(output)])
            subprocess.run(command, text=True, capture_output=True, check=True)
            actual = hashlib.sha256(Image.open(output).convert("RGB").tobytes()).hexdigest()
            if actual != expected_hash:
                raise AssertionError(f"submenu {direction} transition frame {frame} changed")

        for (menu, row, rights), expected_hash in EXPECTED_MENU_ACTION_SHA256.items():
            output = Path(directory) / f"setup_{menu}_{row}_{rights}.bmp"
            result = subprocess.run(
                [str(exe), "--headless", "--setup-only", "--setup-menu", menu,
                 "--setup-menu-row", str(row), "--setup-menu-right", str(rights),
                 "--rom", str(rom), "--assets", str(pack), "--frames", "450",
                 "--dump-frame", str(output)],
                text=True, capture_output=True, check=True,
            )
            if "DSP menu SFX SRCN $1C" not in result.stdout:
                raise AssertionError(f"Set {menu.title()} did not play confirm SFX")
            if row and "DSP menu SFX SRCN $1B" not in result.stdout:
                raise AssertionError(f"Set {menu.title()} did not play move SFX")
            if rights and "DSP menu SFX SRCN $1A" not in result.stdout:
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
                "--rom", str(rom), "--assets", str(pack), "--frames", "450",
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
            if "DSP menu SFX SRCN $1A" not in result.stdout:
                raise AssertionError(f"Set {menu.title()} adjustment SFX was not dispatched")

        # Bar rows clamp at their endpoints and do not emit command $49 when
        # the requested direction cannot change the value.
        clamp = subprocess.run(
            [str(exe), "--headless", "--setup-only", "--setup-menu", "rules",
             "--setup-menu-right", "1", "--rom", str(rom), "--assets", str(pack),
             "--frames", "450"],
            text=True, capture_output=True, check=True,
        )
        if "option_row=0 working=45 committed=45" not in clamp.stdout:
            raise AssertionError("Rules slider no longer clamps at 45")
        if "DSP menu SFX SRCN $1A" in clamp.stdout:
            raise AssertionError("blocked Rules slider adjustment emitted SFX")

        # $82:8DDC -> $87:8C2D applies row 1 immediately. Prove the same
        # SRCN is actually rescaled, not merely that the displayed value moved.
        sfx_peaks = []
        for row in (0, 1):
            volume_run = subprocess.run(
                [str(exe), "--headless", "--setup-only", "--setup-menu", "options",
                 "--setup-menu-row", str(row), "--setup-menu-right", "1",
                 "--rom", str(rom), "--assets", str(pack), "--frames", "450"],
                text=True, capture_output=True, check=True,
            )
            match = re.search(
                r"SRCN \$1A pitch=\$05A8 ADSR1/2=\$8E/\$E0, "
                r"volume=(\d+)/45 DSPVOL=\$([0-9A-F]{2}) peak=(\d+)",
                volume_run.stdout,
            )
            if not match:
                raise AssertionError("menu SFX gain telemetry missing")
            volume, dsp_volume, peak = match.groups()
            sfx_peaks.append((int(volume), int(dsp_volume, 16), int(peak)))
        if sfx_peaks[0][:2] != (30, 0x40) or sfx_peaks[1][:2] != (31, 0x42) or \
           sfx_peaks[1][2] <= sfx_peaks[0][2]:
            raise AssertionError(f"SFX Volume did not change PCM gain: {sfx_peaks}")

        # Thirteen Down presses wrap the 13-row Rules cursor to row zero.
        wrapped = subprocess.run(
            [str(exe), "--headless", "--setup-only", "--setup-menu", "rules",
             "--setup-menu-row", "13", "--rom", str(rom), "--assets", str(pack),
             "--frames", "450"],
            text=True, capture_output=True, check=True,
        )
        if "page=1 menu_row=0" not in wrapped.stdout:
            raise AssertionError("Rules submenu cursor no longer wraps")

        # B is consumed but ignored: the working edit remains uncommitted and
        # the page stays open, exactly as the submenu handler does.
        ignored_b = subprocess.run(
            [str(exe), "--headless", "--setup-only", "--setup-menu", "rules",
             "--setup-menu-row", "2", "--setup-menu-right", "1", "--setup-menu-b",
             "--rom", str(rom), "--assets", str(pack), "--frames", "450"],
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
