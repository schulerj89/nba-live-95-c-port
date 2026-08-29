"""Regression checks for asset-pack safety, ROM identity, and host-rate timing."""

import argparse
import hashlib
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

from extract_assets import load_verified_rom

PACK_VERSION = 31

def pack_entry(asset_id, offset, size, width=0, height=0, flags=0):
    return struct.pack("<6I", asset_id, offset, size, width, height, flags)


def write_pack_subset(source, destination, selected_ids):
    raw = source.read_bytes()
    if raw[:8] != b"NBA95PAK":
        raise AssertionError("source asset pack magic changed")
    version, count = struct.unpack_from("<II", raw, 8)
    selected = []
    for index in range(count):
        fields = struct.unpack_from("<6I", raw, 16 + index * 24)
        asset_id, offset, size, width, height, flags = fields
        if asset_id in selected_ids:
            selected.append((asset_id, raw[offset:offset + size], width, height, flags))
    if {item[0] for item in selected} != set(selected_ids):
        raise AssertionError("source pack omitted a requested subset asset")
    payload_offset = 16 + len(selected) * 24
    directory = bytearray()
    payload = bytearray()
    for asset_id, data, width, height, flags in selected:
        directory += pack_entry(asset_id, payload_offset + len(payload), len(data),
                                width, height, flags)
        payload += data
    destination.write_bytes(
        b"NBA95PAK" + struct.pack("<II", version, len(selected)) +
        directory + payload
    )


def run(exe, *args):
    return subprocess.run(
        [str(exe), *map(str, args)], text=True, capture_output=True, check=False
    )


def require_success(result, description):
    if result.returncode != 0:
        raise AssertionError(
            f"{description} failed ({result.returncode}):\n"
            f"{result.stdout}\n{result.stderr}"
        )


def require_failure(result, description):
    if result.returncode == 0:
        raise AssertionError(f"{description} was unexpectedly accepted")


def check_asset_loader(exe, directory):
    valid = b"NBA95PAK" + struct.pack("<II", PACK_VERSION, 1)
    valid += pack_entry(18, 40, 1) + b"\0"
    valid_path = directory / "valid.pak"
    valid_path.write_bytes(valid)
    require_success(
        run(exe, "--headless", "--assets", valid_path, "--frames", 0),
        "valid minimal asset pack",
    )

    duplicate = b"NBA95PAK" + struct.pack("<II", PACK_VERSION, 2)
    duplicate += pack_entry(1, 64, 1) + pack_entry(1, 65, 1) + b"\0\0"
    invalid_packs = {
        "bad_magic.pak": b"NOTAPACK" + struct.pack("<II", PACK_VERSION, 1) + pack_entry(1, 40, 1) + b"\0",
        "bad_version.pak": b"NBA95PAK" + struct.pack("<II", 4, 1) + pack_entry(1, 40, 1) + b"\0",
        "too_many.pak": b"NBA95PAK" + struct.pack("<II", PACK_VERSION, 286),
        "truncated_directory.pak": b"NBA95PAK" + struct.pack("<II", PACK_VERSION, 2) + pack_entry(1, 64, 1),
        "duplicate_id.pak": duplicate,
        "directory_overlap.pak": b"NBA95PAK" + struct.pack("<II", PACK_VERSION, 1) + pack_entry(1, 16, 1) + b"\0",
        "wrapped_range.pak": b"NBA95PAK" + struct.pack("<II", PACK_VERSION, 1) + pack_entry(1, 0xFFFFFFF0, 64) + b"\0",
        "bad_id.pak": b"NBA95PAK" + struct.pack("<II", PACK_VERSION, 1) + pack_entry(286, 40, 1) + b"\0",
        "short_license.pak": b"NBA95PAK" + struct.pack("<II", PACK_VERSION, 1) +
            pack_entry(1, 40, 1, 128, 11) + b"\0",
        "short_court_map.pak": b"NBA95PAK" + struct.pack("<II", PACK_VERSION, 1) +
            pack_entry(279, 40, 2, 148, 52, 0xA08000) + b"\0\0",
        "short_legal.pak": b"NBA95PAK" + struct.pack("<II", PACK_VERSION, 1) +
            pack_entry(2, 40, 1, 256, 151, 35) + b"\0",
        "short_ea_pixels.pak": b"NBA95PAK" + struct.pack("<II", PACK_VERSION, 1) +
            pack_entry(3, 40, 1, 1, 1) + b"\0",
        "oversized_ea_dimensions.pak": b"NBA95PAK" + struct.pack("<II", PACK_VERSION, 1) +
            pack_entry(3, 40, 1, 0xFFFFFFFF, 0xFFFFFFFF) + b"\0",
        "offscreen_ea_flags.pak": b"NBA95PAK" + struct.pack("<II", PACK_VERSION, 1) +
            pack_entry(3, 40, 4, 1, 1, (256 << 16)) + b"\0\0\0\0",
    }
    for name, payload in invalid_packs.items():
        path = directory / name
        path.write_bytes(payload)
        require_failure(
            run(exe, "--headless", "--assets", path, "--frames", 0), name
        )
    return valid_path


def check_rom_identity(exe, rom_path, valid_pack, directory):
    rom = rom_path.read_bytes()
    headered_path = directory / "headered.sfc"
    headered_path.write_bytes(bytes(512) + rom)
    require_success(
        run(exe, "--headless", "--rom", headered_path,
            "--assets", valid_pack, "--frames", 0),
        "headered expected ROM",
    )
    if load_verified_rom(rom_path) != rom or load_verified_rom(headered_path) != rom:
        raise AssertionError("extractor ROM normalization changed clean/headered bytes")

    wrong = bytearray(rom)
    wrong[0] ^= 0x01
    wrong_path = directory / "wrong.sfc"
    wrong_path.write_bytes(wrong)
    require_failure(
        run(exe, "--headless", "--rom", wrong_path,
            "--assets", valid_pack, "--frames", 0),
        "wrong ROM hash",
    )
    try:
        load_verified_rom(wrong_path)
    except RuntimeError:
        pass
    else:
        raise AssertionError("asset extractor accepted the wrong ROM hash")

    extractor = Path(__file__).resolve().parent / "extract_assets.py"
    extracted = []
    for label, candidate in (("clean", rom_path), ("headered", headered_path)):
        output = directory / f"{label}.pak"
        result = subprocess.run(
            [sys.executable, str(extractor), "--rom", str(candidate),
             "--output", str(output)],
            text=True, capture_output=True, check=False,
        )
        require_success(result, f"{label} ROM asset extraction")
        extracted.append(output.read_bytes())
    if extracted[0] != extracted[1]:
        raise AssertionError("clean and copier-headered ROM extraction differs")

    rejected_output = directory / "wrong.pak"
    require_failure(
        subprocess.run(
            [sys.executable, str(extractor), "--rom", str(wrong_path),
             "--output", str(rejected_output)],
            text=True, capture_output=True, check=False,
        ),
        "wrong ROM asset extraction",
    )


def check_host_rate_equivalence(exe, rom, pack, directory):
    frames = []
    for rate in (60.0, 59.94):
        output = directory / f"title_{rate}.bmp"
        result = run(
            exe, "--headless", "--title-only", "--rom", rom,
            "--assets", pack, "--frames", 1320,
            "--tick-rate", rate, "--dump-frame", output,
        )
        require_success(result, f"title render at {rate} Hz")
        frames.append(output.read_bytes())
    if frames[0] != frames[1]:
        raise AssertionError("title animation differs between 60.0 and 59.94 Hz")


def check_asset_debug_cli(exe, pack):
    require_failure(
        run(exe, "--headless", "--assets", pack, "--frames", 0,
            "--asset-debug", "abc"),
        "malformed asset debugger ID",
    )
    require_failure(
        run(exe, "--headless", "--assets", pack, "--frames", 0,
            "--asset-debug", 159),
        "missing asset debugger ID",
    )


def check_debug_telemetry(exe, rom, pack, directory):
    require_failure(
        run(exe, "--headless", "--debug-every", 0),
        "zero debug sampling interval",
    )
    require_failure(
        run(exe, "--headless", "--debug-hud-page", 3),
        "invalid debug HUD page",
    )
    frame = directory / "debug_setup.bmp"
    result = run(
        exe, "--headless", "--setup-only", "--rom", rom, "--assets", pack,
        "--frames", 170, "--setup-main-row", 3, "--setup-main-right", 1,
        "--debug-state", "--debug-every", 85, "--timing-debug",
        "--dump-frame", frame,
    )
    require_success(result, "expanded CLI/HUD debug telemetry")
    expected = (
        "SCN:GAME_SETUP",
        "GF:000170 SF:00170",
        "IN P:0000 H:0000 R:0000",
        "PG:MAIN ROW:QUARTER MR:00",
        "TR:NONE TF:000 BLK:0 ACT:NONE",
        "CFG M:EXHIB S:SIM L:ROOKIE Q:5MIN",
        "PPU B:15 X1:512 X2:000 Y2:020 Y3:000",
        "AUD:SETUP_SPC ST:READY MV:30 SV:30 SRC:1A",
    )
    for marker in expected:
        if marker not in result.stdout:
            raise AssertionError(f"debug telemetry omitted {marker!r}")
    if result.stdout.count("[DEBUG SAMPLE]") != 2:
        raise AssertionError("periodic CLI debug sampling count changed")
    digest = hashlib.sha256(frame.read_bytes()).hexdigest()
    expected_digest = "6e81d0296a4bf78d1d3c24a071674234408e9bd3592de624a4ea04eb414a5ca0"
    if digest != expected_digest:
        raise AssertionError(f"F10 overview HUD changed: {digest}")

    detail_frame = directory / "debug_setup_detail.bmp"
    detail = run(
        exe, "--headless", "--setup-only", "--rom", rom, "--assets", pack,
        "--frames", 170, "--setup-main-row", 3, "--setup-main-right", 1,
        "--debug-hud-page", 2, "--dump-frame", detail_frame,
    )
    require_success(detail, "compact F10 Setup-detail HUD")
    detail_digest = hashlib.sha256(detail_frame.read_bytes()).hexdigest()
    expected_detail = "e9abbf46b9d0ce04ea3ff2b88662704137ebf5ffc34bef8f4e16b47a5ce4c591"
    if detail_digest != expected_detail:
        raise AssertionError(f"F10 Setup-detail HUD changed: {detail_digest}")


def check_scene_audio_failures(exe, rom, minimal_pack, full_pack, directory):
    title = run(
        exe, "--headless", "--title-only", "--rom", rom,
        "--assets", minimal_pack, "--frames", 0,
    )
    require_failure(title, "missing title audio entry")
    if "Title synthesis failed" not in title.stderr:
        raise AssertionError("title entry did not expose synthesis failure")

    missing_gfx = run(
        exe, "--headless", "--setup-only", "--rom", rom,
        "--assets", minimal_pack, "--frames", 0,
    )
    require_failure(missing_gfx, "missing Setup graphics entry")
    if "graphics initialization failed" not in missing_gfx.stderr or \
            "Game Setup synthesis failed" in missing_gfx.stderr:
        raise AssertionError("Setup graphics failure was misclassified as audio")

    graphics_only = directory / "setup_graphics_only.pak"
    write_pack_subset(full_pack, graphics_only, {16, 17, 92})
    missing_audio = run(
        exe, "--headless", "--setup-only", "--rom", rom,
        "--assets", graphics_only, "--frames", 0,
    )
    require_failure(missing_audio, "missing Setup audio entry")
    if "Game Setup synthesis failed" not in missing_audio.stderr:
        raise AssertionError("valid-gfx Setup audio failure was not exercised")


def check_capture_orchestrator():
    script = (Path(__file__).resolve().parent / "capture_assets.ps1").read_text()
    required = (
        "setup_rules", "setup_options", "setup_main",
        "mesen_setup_menus_capture.lua", "mesen_setup_main_capture.lua",
        "NBA95_CAPTURE_MENU", "NBA95_CAPTURE_VARIANTS", "Required",
    )
    for marker in required:
        if marker not in script:
            raise AssertionError(f"capture orchestrator omitted {marker}")


def check_capture_write_failure(exe, minimal_pack, directory):
    missing_parent = directory / "missing" / "frame.bmp"
    require_failure(
        run(exe, "--headless", "--assets", minimal_pack, "--frames", 0,
            "--dump-frame", missing_parent),
        "unwritable frame capture",
    )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--pack", required=True, type=Path)
    parser.add_argument("--exe", required=True, type=Path)
    parser.add_argument("--rom", required=True, type=Path)
    args = parser.parse_args()

    result = run(args.exe, "--spc-self-test")
    require_success(result, "SPC700/S-DSP self-test")
    if "[SPC TEST] PASS" not in result.stdout:
        raise AssertionError("SPC700/S-DSP self-test did not report PASS")

    with tempfile.TemporaryDirectory(prefix="nba95-core-safety-") as temp:
        directory = Path(temp)
        valid_pack = check_asset_loader(args.exe, directory)
        check_rom_identity(args.exe, args.rom, valid_pack, directory)
        check_host_rate_equivalence(args.exe, args.rom, args.pack, directory)
        check_asset_debug_cli(args.exe, args.pack)
        check_debug_telemetry(args.exe, args.rom, args.pack, directory)
        check_scene_audio_failures(args.exe, args.rom, valid_pack,
                                   args.pack, directory)
        check_capture_orchestrator()
        check_capture_write_failure(args.exe, valid_pack, directory)
    print("[TEST] PASS: core safety, scene/audio lifecycle, captures, and debug telemetry")


if __name__ == "__main__":
    main()
