"""Check new/legacy shot-table packs and reject malformed local mutations."""

import argparse
import hashlib
import struct
import subprocess
import tempfile
from pathlib import Path


def entry(blob: bytearray, asset_id: int) -> tuple[int, int, int]:
    if blob[:8] != b"NBA95PAK":
        raise ValueError("invalid pack magic")
    count = struct.unpack_from("<I", blob, 12)[0]
    for index in range(count):
        position = 16 + index * 24
        if struct.unpack_from("<I", blob, position)[0] == asset_id:
            offset, size = struct.unpack_from("<II", blob, position + 4)
            return position, offset, size
    raise ValueError(f"asset {asset_id} is absent")


def run(probe: Path, pack: Path, option: str, expected: int = 0) -> None:
    result = subprocess.run([str(probe), str(pack), option], capture_output=True)
    if result.returncode != expected:
        raise AssertionError(
            f"{option} returned {result.returncode}, expected {expected}\n"
            f"{result.stdout.decode(errors='replace')}"
            f"{result.stderr.decode(errors='replace')}"
        )


def legacy_pack(current: bytearray, directory: int, payload: int) -> bytearray:
    ranges = []
    for index in range(5):
        address, size, offset = struct.unpack_from("<III", current, payload + 12 + index * 12)
        ranges.append((address, size, bytes(current[payload + offset : payload + offset + size])))
    legacy = bytearray(b"NBSHOT1\0" + struct.pack("<I", 5))
    offset = 72
    for address, size, _ in ranges:
        legacy.extend(struct.pack("<III", address, size, offset))
        offset += size
    for _, _, data in ranges:
        legacy.extend(data)
    if len(legacy) != 528:
        raise ValueError("legacy shot-table payload did not rebuild to 528 bytes")
    result = bytearray(current)
    result[payload : payload + len(legacy)] = legacy
    struct.pack_into("<I", result, directory + 8, len(legacy))
    struct.pack_into("<I", result, directory + 12, 5)
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", type=Path, required=True)
    parser.add_argument("--new-pack", type=Path, required=True)
    parser.add_argument("--old-pack", type=Path)
    args = parser.parse_args()

    current = bytearray(args.new_pack.read_bytes())
    directory, payload, size = entry(current, 277)
    if size != 620 or current[payload : payload + 8] != b"NBSHOT1\0":
        raise ValueError("new pack does not contain the expected shot-table extension")
    run(args.probe, args.new_pack, "--pack-new")
    with tempfile.TemporaryDirectory(prefix="nba95-mode13-pack-") as temp:
        temp_path = Path(temp)

        if args.old_pack:
            old_pack_path = args.old_pack
        else:
            old_pack_path = temp_path / "derived-old-shot-tables.pak"
            old_pack_path.write_bytes(legacy_pack(current, directory, payload))
        run(args.probe, old_pack_path, "--pack-old")
        old_hash = hashlib.sha256(old_pack_path.read_bytes()).hexdigest()

        malformed_payload = bytearray(current)
        struct.pack_into("<I", malformed_payload, payload + 12, 0)
        malformed_payload_path = temp_path / "malformed-shot-directory.pak"
        malformed_payload_path.write_bytes(malformed_payload)
        run(args.probe, malformed_payload_path, "--pack-reject")

        malformed_metadata = bytearray(current)
        struct.pack_into("<I", malformed_metadata, directory + 12, 6)
        malformed_metadata_path = temp_path / "malformed-pack-directory.pak"
        malformed_metadata_path.write_bytes(malformed_metadata)
        run(args.probe, malformed_metadata_path, "--pack-new", expected=3)

    print(
        "[CPU MODE THIRTEEN PACK] PASS: new620/7, old528/5 launch, "
        "missing close tables, malformed payload and malformed metadata; "
        f"new_sha256={hashlib.sha256(args.new_pack.read_bytes()).hexdigest()} "
        f"old_sha256={old_hash}"
    )


if __name__ == "__main__":
    main()
