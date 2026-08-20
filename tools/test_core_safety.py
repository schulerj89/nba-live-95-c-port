"""Regression checks for asset-pack safety, ROM identity, and host-rate timing."""

import argparse
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

from extract_assets import load_verified_rom


def pack_entry(asset_id, offset, size, width=0, height=0, flags=0):
    return struct.pack("<6I", asset_id, offset, size, width, height, flags)


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
    valid = b"NBA95PAK" + struct.pack("<II", 5, 1)
    valid += pack_entry(18, 40, 1) + b"\0"
    valid_path = directory / "valid.pak"
    valid_path.write_bytes(valid)
    require_success(
        run(exe, "--headless", "--assets", valid_path, "--frames", 0),
        "valid minimal asset pack",
    )

    duplicate = b"NBA95PAK" + struct.pack("<II", 5, 2)
    duplicate += pack_entry(1, 64, 1) + pack_entry(1, 65, 1) + b"\0\0"
    invalid_packs = {
        "bad_magic.pak": b"NOTAPACK" + struct.pack("<II", 5, 1) + pack_entry(1, 40, 1) + b"\0",
        "bad_version.pak": b"NBA95PAK" + struct.pack("<II", 4, 1) + pack_entry(1, 40, 1) + b"\0",
        "too_many.pak": b"NBA95PAK" + struct.pack("<II", 5, 128),
        "truncated_directory.pak": b"NBA95PAK" + struct.pack("<II", 5, 2) + pack_entry(1, 64, 1),
        "duplicate_id.pak": duplicate,
        "directory_overlap.pak": b"NBA95PAK" + struct.pack("<II", 5, 1) + pack_entry(1, 16, 1) + b"\0",
        "wrapped_range.pak": b"NBA95PAK" + struct.pack("<II", 5, 1) + pack_entry(1, 0xFFFFFFF0, 64) + b"\0",
        "bad_id.pak": b"NBA95PAK" + struct.pack("<II", 5, 1) + pack_entry(128, 40, 1) + b"\0",
        "short_license.pak": b"NBA95PAK" + struct.pack("<II", 5, 1) +
            pack_entry(1, 40, 1, 128, 11) + b"\0",
        "short_legal.pak": b"NBA95PAK" + struct.pack("<II", 5, 1) +
            pack_entry(2, 40, 1, 256, 151, 35) + b"\0",
        "short_ea_pixels.pak": b"NBA95PAK" + struct.pack("<II", 5, 1) +
            pack_entry(3, 40, 1, 1, 1) + b"\0",
        "oversized_ea_dimensions.pak": b"NBA95PAK" + struct.pack("<II", 5, 1) +
            pack_entry(3, 40, 1, 0xFFFFFFFF, 0xFFFFFFFF) + b"\0",
        "offscreen_ea_flags.pak": b"NBA95PAK" + struct.pack("<II", 5, 1) +
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
    print("[TEST] PASS: asset-pack safety, ROM identity, and host-rate timing")


if __name__ == "__main__":
    main()
