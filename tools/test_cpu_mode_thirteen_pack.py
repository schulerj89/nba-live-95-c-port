"""Check mode-14, mode-13 and legacy shot-table pack compatibility."""

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


def range_pack(current: bytearray, directory: int, payload: int, count: int) -> bytearray:
    ranges = []
    for index in range(count):
        address, size, offset = struct.unpack_from("<III", current, payload + 12 + index * 12)
        ranges.append((address, size, bytes(current[payload + offset : payload + offset + size])))
    result_payload = bytearray(b"NBSHOT1\0" + struct.pack("<I", count))
    offset = 12 + count * 12
    for address, size, _ in ranges:
        result_payload.extend(struct.pack("<III", address, size, offset))
        offset += size
    for _, _, data in ranges:
        result_payload.extend(data)
    expected = {5: 528, 7: 620}[count]
    if len(result_payload) != expected:
        raise ValueError(f"{count}-range shot-table payload was {len(result_payload)}, expected {expected}")
    result = bytearray(current)
    result[payload : payload + len(result_payload)] = result_payload
    struct.pack_into("<I", result, directory + 8, len(result_payload))
    struct.pack_into("<I", result, directory + 12, count)
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", type=Path, required=True)
    parser.add_argument("--new-pack", type=Path, required=True)
    parser.add_argument("--old-pack", type=Path)
    args = parser.parse_args()

    current = bytearray(args.new_pack.read_bytes())
    directory, payload, size = entry(current, 277)
    if size != 640 or current[payload : payload + 8] != b"NBSHOT1\0":
        raise ValueError("new pack does not contain the expected shot-table extension")
    run(args.probe, args.new_pack, "--pack-new")
    with tempfile.TemporaryDirectory(prefix="nba95-mode13-pack-") as temp:
        temp_path = Path(temp)

        if args.old_pack:
            mode13_pack_path = args.old_pack
        else:
            mode13_pack_path = temp_path / "derived-mode13-shot-tables.pak"
            mode13_pack_path.write_bytes(range_pack(current, directory, payload, 7))
        run(args.probe, mode13_pack_path, "--pack-mode13")
        mode13_hash = hashlib.sha256(mode13_pack_path.read_bytes()).hexdigest()

        legacy_pack_path = temp_path / "derived-legacy-shot-tables.pak"
        legacy_pack_path.write_bytes(range_pack(current, directory, payload, 5))
        run(args.probe, legacy_pack_path, "--pack-old")
        legacy_hash = hashlib.sha256(legacy_pack_path.read_bytes()).hexdigest()

        malformed_payload = bytearray(current)
        struct.pack_into("<I", malformed_payload, payload + 12, 0)
        malformed_payload_path = temp_path / "malformed-shot-directory.pak"
        malformed_payload_path.write_bytes(malformed_payload)
        run(args.probe, malformed_payload_path, "--pack-reject")

        malformed_metadata = bytearray(current)
        struct.pack_into("<I", malformed_metadata, directory + 12, 7)
        malformed_metadata_path = temp_path / "malformed-pack-directory.pak"
        malformed_metadata_path.write_bytes(malformed_metadata)
        run(args.probe, malformed_metadata_path, "--pack-new", expected=3)

    print(
        "[CPU SHOT TABLE PACK] PASS: mode14=640/8, mode13=620/7, "
        "legacy=528/5, malformed payload and metadata rejected; "
        f"mode14_sha256={hashlib.sha256(args.new_pack.read_bytes()).hexdigest()} "
        f"mode13_sha256={mode13_hash} legacy_sha256={legacy_hash}"
    )


if __name__ == "__main__":
    main()
