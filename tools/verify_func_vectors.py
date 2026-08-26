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
                        help="single exit word address produced by the probe")
    parser.add_argument("--output-words",
                        help="comma-separated exit word addresses produced by "
                             "the probe, e.g. 00aa,00b2")
    parser.add_argument("--max-mismatches", type=int, default=10)
    args = parser.parse_args()

    if args.word:
        input_words = [args.word]
        output_words = [args.word]
    else:
        output_spec = args.output_words or args.output_word
        if not args.input_words or not output_spec:
            parser.error("use --word or --input-words with --output-word(s)")
        input_words = [word.strip() for word in args.input_words.split(",")
                       if word.strip()]
        output_words = [word.strip() for word in output_spec.split(",")
                        if word.strip()]

    vectors = [json.loads(line) for line in
               Path(args.vectors).read_text().splitlines() if line.strip()]
    stdin = "\n".join(
        " ".join(f"{mem_word(v['entry']['mem'], word):04x}"
                 for word in input_words) for v in vectors)
    result = subprocess.run([args.probe], input=stdin + "\n",
                            capture_output=True, text=True, check=True)
    output_lines = [line.split() for line in result.stdout.splitlines()
                    if line.strip()]
    if len(output_lines) != len(vectors) or \
            any(len(line) != len(output_words) for line in output_lines):
        raise SystemExit(f"[FUNC VECTORS] FAIL: probe produced "
                         f"{len(output_lines)} rows x "
                         f"{[len(line) for line in output_lines[:5]]} outputs "
                         f"for {len(vectors)} vectors x {len(output_words)}")

    mismatches = []
    for vector, produced_tokens in zip(vectors, output_lines):
        expected = [mem_word(vector["exit"]["mem"], word)
                    for word in output_words]
        produced = [int(token, 16) for token in produced_tokens]
        if produced != expected:
            entry = ",".join(
                f"{mem_word(vector['entry']['mem'], word):04x}"
                for word in input_words)
            mismatches.append((vector["call"], entry, expected, produced))

    status = "PASS" if not mismatches else "FAIL"
    print(f"[FUNC VECTORS] {status}: vectors={len(vectors)} "
          f"mismatches={len(mismatches)} ({Path(args.vectors).name})")
    for call, entry, expected, produced in mismatches[:args.max_mismatches]:
        expected_text = ",".join(f"{word:04x}" for word in expected)
        produced_text = ",".join(f"{word:04x}" for word in produced)
        print(f"  call={call} entry={entry} rom={expected_text} "
              f"port={produced_text}")
    if mismatches:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
