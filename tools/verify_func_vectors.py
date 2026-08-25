"""Replay mesen_func_vectors.lua captures through a C-port probe.

The probe is a small C executable built against the real port sources. It
reads one entry state per stdin line and prints the state the ported function
produces; this driver feeds every captured vector's entry WRAM word through
it and diffs the probe output against the ROM's recorded exit WRAM word.

Usage:
    python tools/verify_func_vectors.py --vectors rng_next.vectors.jsonl \
        --probe build/rng_vector_probe.exe --word 07f6
"""

import argparse
import json
import subprocess
from pathlib import Path


def le_word(hex_bytes):
    raw = bytes.fromhex(hex_bytes)
    return raw[0] | (raw[1] << 8)


def mem_word(snapshot, address):
    """Read one little-endian word from any captured range containing it."""
    wanted = int(address, 16)
    for base_text, payload in snapshot.items():
        base = int(base_text, 16)
        raw = bytes.fromhex(payload)
        offset = wanted - base
        if 0 <= offset and offset + 1 < len(raw):
            return raw[offset] | (raw[offset + 1] << 8)
    raise KeyError(f"captured memory does not contain word {address}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--vectors", required=True)
    parser.add_argument("--probe", required=True)
    parser.add_argument("--word",
                        help="entry/exit mem key holding the state word, "
                             "e.g. 07f6")
    parser.add_argument("--input-words",
                        help="comma-separated entry word addresses passed to "
                             "the probe, e.g. 00aa,00ae")
    parser.add_argument("--output-word",
                        help="exit word address produced by the probe")
    parser.add_argument("--max-mismatches", type=int, default=10)
    args = parser.parse_args()

    if args.word:
        input_words = [args.word]
        output_word = args.word
    else:
        if not args.input_words or not args.output_word:
            parser.error("use --word or both --input-words and --output-word")
        input_words = [word.strip() for word in args.input_words.split(",")
                       if word.strip()]
        output_word = args.output_word

    vectors = [json.loads(line) for line in
               Path(args.vectors).read_text().splitlines() if line.strip()]
    stdin = "\n".join(
        " ".join(f"{mem_word(v['entry']['mem'], word):04x}"
                 for word in input_words) for v in vectors)
    result = subprocess.run([args.probe], input=stdin + "\n",
                            capture_output=True, text=True, check=True)
    outputs = result.stdout.split()
    if len(outputs) != len(vectors):
        raise SystemExit(f"[FUNC VECTORS] FAIL: probe produced "
                         f"{len(outputs)} outputs for {len(vectors)} vectors")

    mismatches = []
    for vector, produced in zip(vectors, outputs):
        expected = mem_word(vector["exit"]["mem"], output_word)
        if int(produced, 16) != expected:
            entry = ",".join(
                f"{mem_word(vector['entry']['mem'], word):04x}"
                for word in input_words)
            mismatches.append((vector["call"], entry,
                               expected, int(produced, 16)))

    status = "PASS" if not mismatches else "FAIL"
    print(f"[FUNC VECTORS] {status}: vectors={len(vectors)} "
          f"mismatches={len(mismatches)} ({Path(args.vectors).name})")
    for call, entry, expected, produced in mismatches[:args.max_mismatches]:
        print(f"  call={call} entry={entry} rom={expected:04x} "
              f"port={produced:04x}")
    if mismatches:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
