"""Independent FB30 byte/codebook witness, deliberately not a timing model."""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path

from verify_setup_codec_work import read_native, native_records, require, digest, ROM_SHA


class Bits:
    def __init__(self, raw):
        self.raw = raw
        self.cursor = 48

    def peek(self):
        require(self.cursor < len(self.raw) * 8, 'truncated FB30 bitstream')
        return (self.raw[self.cursor // 8] >> (7 - self.cursor % 8)) & 1

    def bits(self, width):
        result = 0
        for _ in range(width):
            result = result * 2 + self.peek()
            self.cursor += 1
        return result

    def number(self):
        if self.bits(1): return self.bits(2)
        width = 3
        while self.bits(1) == 0:
            width += 1
            require(width <= 16, 'number exceeds bounded native word width')
        return (1 << width) - 4 + self.bits(width)


def decode(raw):
    require(raw[:2] == bytes.fromhex('30fb'), 'FB30 signature required')
    bits = Bits(raw)
    counts, offsets, thresholds = [], [], []
    first, total = 0, 0
    for length in range(1, 17):
        first *= 2
        offsets.append(first - total)
        count = bits.number()
        counts.append(count)
        total += count
        first += count
        thresholds.append(((first << (16 - length)) & 65535) if count else 0)
        require(total <= 256 and first <= 1 << length, 'invalid bounded canonical tree')
        if first == 1 << length: break
    else: raise ValueError('Huffman tree does not close')
    symbols, used, ranks = [], set(), []
    symbol = 255
    for _ in range(total):
        rank = bits.number()
        require(rank < 256 - len(used), 'cyclic rank exceeds unused symbols')
        ranks.append(rank)
        remaining = rank + 1
        while remaining:
            symbol = (symbol + 1) & 255
            if symbol not in used: remaining -= 1
        used.add(symbol)
        symbols.append(symbol)
    payload_start_bit = bits.cursor
    codes, first, index = {}, 0, 0
    # $0300 starts as the used-symbol bitmap from cyclic rank construction.
    # The native fast-table builder overwrites only the short-code prefixes.
    fast_symbols = bytearray(255 if v in used else 0 for v in range(256))
    fast_lengths = bytearray([0x10] * 256)
    for length, count in enumerate(counts, 1):
        first *= 2
        for code in range(first, first + count):
            value = symbols[index]; index += 1
            codes[length, code] = value
            if length <= 8:
                for prefix in range(code << (8 - length), (code + 1) << (8 - length)):
                    fast_symbols[prefix] = value
                    fast_lengths[prefix] = 0x12 if value == raw[5] else 2 * (length - 1)
        first += count
    output, last, tokens, lengths, runs = bytearray(), 0, Counter(), Counter(), []
    for _ in range(0x20000):
        code = 0
        for length in range(1, len(counts) + 1):
            code = code * 2 + bits.bits(1)
            if (length, code) in codes: break
        else: raise ValueError('unknown code')
        value = codes[length, code]
        lengths[length] += 1
        if value != raw[5]:
            last = value; output.append(last); tokens['literal'] += 1
        else:
            count = bits.number()
            if count:
                output.extend(bytes([last]) * count); runs.append(count); tokens['run'] += 1
            elif bits.peek():
                # Original $80:C44D/C44F peeks at the termination bit. It is
                # deliberately NOT consumed; cursor and prefetch stay native.
                tokens['end'] += 1
                break
            else:
                bits.bits(1)
                last = bits.bits(8); output.append(last); tokens['raw_literal'] += 1
        require(len(output) <= 0x20000, 'output exceeds bounded WRAM destination')
    else: raise ValueError('no bounded terminator')
    require(len(output) == int.from_bytes(raw[3:5], 'big'), 'declared output length differs')
    lookahead = bits.cursor // 8
    require(lookahead + 1 < len(raw), 'missing native prefetch byte')
    buffer = (int.from_bytes(raw[lookahead:lookahead + 2], 'big') << (bits.cursor % 8)) & 65535
    return dict(payload=bytes(output), symbols=bytes(symbols), counts=counts, offsets=offsets,
        thresholds=thresholds, fast_symbols=bytes(fast_symbols), fast_lengths=bytes(fast_lengths),
        ranks=ranks, payload_start_bit=payload_start_bit, end_bit_before_flag=bits.cursor,
        prefetch_offset=lookahead + 2, bit_buffer=buffer, tokens=dict(tokens),
        lengths=dict(sorted(lengths.items())), runs=runs, last=last)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', type=Path, required=True)
    parser.add_argument('--native', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    rom, native, output = args.rom.resolve(), args.native.resolve(), args.output.resolve()
    require(digest(rom) == ROM_SHA, 'canonical ROM required')
    read_native(native, rom)
    boundaries, _, _ = native_records(native)
    blob = rom.read_bytes(); offset = 0x1720af
    result = decode(blob[offset:offset + 719])
    matches = []
    for entry in boundaries[::2]:
        if entry['source'] != 0xaea0af: continue
        wram = (native / f'codec_{entry["call"]:02d}_exit.wram').read_bytes()
        require(wram[0x12000:0x123c0] == result['payload'], 'native FB30 payload differs')
        require(wram[0x100:0x100 + len(result['symbols'])] == result['symbols'], 'native canonical symbols differ')
        for address, key in [(0x500, 'counts'), (0x520, 'offsets'), (0x540, 'thresholds')]:
            packed = b''.join(v.to_bytes(2, 'little') for v in result[key])
            require(wram[address:address + len(packed)] == packed, 'native codebook differs: ' + key)
        require(wram[0x300:0x400] == result['fast_symbols'] and
                wram[0x400:0x500] == result['fast_lengths'], 'native fast table differs')
        require(int.from_bytes(wram[0x0c:0x0e], 'little') == 0xa0af + result['prefetch_offset'] and
                int.from_bytes(wram[0x1e:0x20], 'little') == result['bit_buffer'] and
                wram[8] == result['last'], 'native cursor/prefetch/last-symbol differs')
        matches.append(entry['call'])
    require(matches == [1, 6, 11, 16], 'four native FB30 payloads required')
    output.mkdir(parents=True, exist_ok=False)
    for key in ('payload', 'symbols', 'fast_symbols', 'fast_lengths'):
        data = result.pop(key)
        (output / (key + '.bin')).write_bytes(data)
        result[key] = dict(bytes=len(data), sha256=hashlib.sha256(data).hexdigest())
    report = dict(schema=1, accepted=True, scope='independent semantic byte/codebook proof, no work timing',
        source=0xaea0af, rom_sha256=ROM_SHA, native_manifest_sha256=digest(native/'manifest.json'),
        verifier_sha256=digest(Path(__file__)), input_sha256=hashlib.sha256(blob[offset:offset + 719]).hexdigest(),
        matches=matches, result=result,
        limitation='No native CPU work or scheduling claim. Production must preserve source construction/search/refill work.')
    (output/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    print('PASS: four native FB30 payloads, canonical/fast tables, unconsumed termination cursor and prefetch')


if __name__ == '__main__':
    main()
