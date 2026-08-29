"""ROM gameplay-audio bank, mixer overlap, and live event regression."""

import argparse
import hashlib
import re
import struct
import subprocess
from pathlib import Path


EXPECTED_SHA256 = "e86b1d5aeff025eb7d42875772bf56c6d120adbdad0daa76071f1ee36cf5f727"
REQUIRED_SOURCES = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
                    0x0A, 0x0B, 0x0C, 0x0D, 0x10, 0x11, 0x12,
                    0x13, 0x14, 0x15, 0x1C}


def words(values):
    return b"".join(struct.pack("<H", value) for value in values)


ROM_TABLES = {
    0x17822: words([0x23, 0x2B, 0x33, 0x23]),
    0x1782A: words([0x09,0x0A,0x0B,0x11,0x12,0x13,0x19,0x1A,
                    0x1B,0x0B,0x19,0x09,0x1A,0x13,0x0A,0x1B]),
    0x1784A: words([0x0C,0x14,0x1C,0x0C]),
    0x17852: words([0x08,0x10,0x18,0x08]),
    0x1785A: words([0x24,0x2C,0x34,0x24]),
    0x17862: words([0x0D,0x15,0x1D,0x0D]),
    0x1786A: words([0x0E,0x1E,0x1E,0x0E]),
    0x17872: words([0x21,0x29,0x31,0x22,0x2A,0x32,0x21,0x29]),
    0x17882: words([0x20,0x28,0x30,0x20]),
}


def asset(path, wanted):
    raw = path.read_bytes()
    if raw[:8] != b"NBA95PAK" or struct.unpack_from("<I", raw, 8)[0] != 31:
        raise AssertionError("invalid gameplay-audio asset pack")
    count = struct.unpack_from("<I", raw, 12)[0]
    for index in range(count):
        asset_id, offset, size, _, _, _ = struct.unpack_from(
            "<6I", raw, 16 + index * 24)
        if asset_id == wanted:
            return raw[offset:offset + size]
    raise AssertionError(f"missing asset {wanted}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--pack", required=True)
    parser.add_argument("--exe", required=True)
    parser.add_argument("--rom", required=True)
    args = parser.parse_args()

    rom = Path(args.rom).read_bytes()
    header = 512 if len(rom) % 0x8000 == 512 else 0
    for offset, wanted in ROM_TABLES.items():
        if rom[header + offset:header + offset + len(wanted)] != wanted:
            raise AssertionError(f"ROM gameplay command table changed at ${offset:06X}")

    bank = asset(Path(args.pack), 285)
    if hashlib.sha256(bank).hexdigest() != EXPECTED_SHA256:
        raise AssertionError("ROM gameplay BRR bank changed")
    if bank[:8] != b"NBGAUD1\0" or struct.unpack_from("<I", bank, 8)[0] != 1:
        raise AssertionError("gameplay-audio schema changed")
    count = struct.unpack_from("<I", bank, 12)[0]
    sources = {}
    for index in range(count):
        srcn, rate, samples, loop, offset = struct.unpack_from(
            "<5I", bank, 16 + index * 20)
        if rate != 32000 or not samples or offset + samples * 2 > len(bank):
            raise AssertionError(f"bad SRCN ${srcn:02X} PCM bounds")
        sources[srcn] = (samples, loop)
    if not REQUIRED_SOURCES <= sources.keys():
        raise AssertionError("gameplay command source family is incomplete")
    if sources[0x0C][1] != 16 or sources[0x0D][1] != 16:
        raise AssertionError("continuous crowd BRR loops changed")

    result = subprocess.run([
        args.exe, "--headless", "--rom", args.rom, "--assets", args.pack,
        "--tipoff-only", "--frames", "1200"], text=True,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=True)
    if "crowd SRCN $0C/$0D and six overlapping effect voices ready" not in result.stdout:
        raise AssertionError("live gameplay mixer did not start")
    events = re.findall(r"command=\$([0-9A-F]{2}) SRCN=\$([0-9A-F]{2})", result.stdout)
    event_set = set(events)
    required_events = {("39", "14")}
    required_sources = {"0B", "04", "0A"}
    if (not required_events <= event_set or
            not required_sources <= {srcn for _, srcn in event_set} or
            not ({"01", "02", "03"} & {srcn for _, srcn in event_set})):
        raise AssertionError(f"live native event dispatch incomplete: {events}")
    print("[PASS] ROM command tables, gameplay bank, crowd overlap, and live court events")


if __name__ == "__main__":
    main()
