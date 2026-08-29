import argparse
import hashlib
import struct
from pathlib import Path


def assets(raw):
    assert raw[:8] == b"NBA95PAK"
    assert struct.unpack_from("<I", raw, 8)[0] == 31
    count = struct.unpack_from("<I", raw, 12)[0]
    result = {}
    for index in range(count):
        item = struct.unpack_from("<6I", raw, 16 + index * 24)
        result[item[0]] = item
    return result


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--pack", required=True)
    args = parser.parse_args()
    raw = Path(args.pack).read_bytes()
    items = assets(raw)

    _, off, size, width, height, flags = items[282]
    goal = raw[off:off + size]
    assert (size, width, height, flags) == (35352, 32, 32, 0x87A73B)
    assert goal[:8] == b"NBGOAL2\0" and struct.unpack_from("<4I", goal, 8) == (
        2, 0x800, 0x8000, 0x200)
    palette = struct.unpack_from("<16H", goal, 24 + 0x800 + 0x8000 + 0x1C0)
    assert palette[1] == 0x7FFF and palette[5:11] == (
        0x32BF, 0x0DDE, 0x019B, 0x0177, 0x00F1, 0x00AC)

    _, off, size, width, height, flags = items[283]
    crowd = raw[off:off + size]
    assert (size, width, height, flags) == (3548, 28, 3, 0x805280)
    assert crowd[:8] == b"NBCROWD1" and struct.unpack_from("<4I", crowd, 8) == (
        1, 3, 28, 32)
    ids = struct.unpack_from("<28H", crowd, 24)
    assert ids == tuple(range(808, 821)) + tuple(range(849, 864))
    stride = 4 + 0x100 + 28 * 32
    frames = []
    for index, expected_frame in enumerate((140, 220, 400)):
        start = 80 + index * stride
        assert struct.unpack_from("<I", crowd, start)[0] == expected_frame
        tiles = crowd[start + 4 + 0x100:start + stride]
        frames.append(hashlib.sha256(tiles).digest())
    assert len(set(frames)) >= 2, "fan animation collapsed to one static tile set"

    # Basket/net descriptors remain raw `$89:8000` resource records in the
    # shared animation pack. This prevents flattened goal screenshot art.
    _, off, size, _, _, _ = items[256]
    animation = raw[off:off + size]
    count = struct.unpack_from("<I", animation, 16)[0]
    directory = struct.unpack_from("<I", animation, 28)[0]
    present = {struct.unpack_from("<H", animation, directory + i * 12)[0]
               for i in range(count)}
    assert {0x0822, 0x082C, 0x082D, 0x082E, 0x082F} <= present
    _, off, size, width, height, flags = items[284]
    ppu = raw[off:off + size]
    assert (width, height, flags) == (0x10000, 0x200, 29)
    assert size == 24 + 29 * (0x10000 + 0x200)
    assert ppu[:8] == b"NBPPUIN1" and struct.unpack_from("<4I", ppu, 8) == (
        1, 29, 0x10000, 0x200)
    print("court asset tests passed (map geometry + goal resources + animated fans)")


if __name__ == "__main__":
    main()
