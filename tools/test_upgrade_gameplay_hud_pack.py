"""Check pack preservation and reject ambiguous or damaged base directories."""
import struct

from upgrade_gameplay_hud_pack import append_hud, unpack


def main():
    first, second = b"first resource", b"second resource"
    start = 16 + 2 * 24
    original = (b"NBA95PAK" + struct.pack("<II", 31, 2) +
                struct.pack("<6I", 145, start, len(first), 256, 224, 7) +
                struct.pack("<6I", 284, start + len(first), len(second), 65536, 512, 29) +
                first + second)
    result, preserved = append_hud(original, b"verified HUD payload")
    assert preserved == [(145, 256, 224, 7, first), (284, 65536, 512, 29, second)]
    assert struct.unpack_from("<II", result, 8) == (31, 3)
    cursor = 16 + 3 * 24
    for index, expected in enumerate(preserved):
        key, offset, size, width, height, flags = struct.unpack_from("<6I", result, 16 + index * 24)
        assert (key, width, height, flags) == expected[:4]
        assert offset == cursor and result[offset:offset + size] == expected[4]
        cursor += size

    mutations = [b"", original[:15], original[:-1], original + b"trailing"]
    for offset, value in [(8, 30), (12, 0), (12, 0xFFFFFFFF),
                          (40, 145), (20, start - 1), (44, start),
                          (24, 0xFFFFFFFF), (48, 0xFFFFFFFF)]:
        corrupt = bytearray(original)
        struct.pack_into("<I", corrupt, offset, value)
        mutations.append(bytes(corrupt))
    for corrupt in mutations:
        try:
            unpack(corrupt)
        except ValueError:
            pass
        else:
            raise AssertionError("damaged base pack was accepted")
    try:
        append_hud(result, b"replacement")
    except ValueError:
        pass
    else:
        raise AssertionError("existing HUD resource was replaced")
    print(f"PASS: payload/metadata preservation and {len(mutations) + 1} rejection cases")


if __name__ == "__main__":
    main()
