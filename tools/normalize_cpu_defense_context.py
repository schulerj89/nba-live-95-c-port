"""Normalize controlled Mesen CPU defense-context calls into a small fixture."""

import argparse
import hashlib
import json
from pathlib import Path


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def rows(path):
    return [json.loads(line) for line in Path(path).read_text().splitlines()
            if line.strip()]


def memory(snapshot):
    raw = bytearray(0x4900)
    for base, payload in snapshot["mem"].items():
        at = int(base, 16)
        data = bytes.fromhex(payload)
        raw[at:at + len(data)] = data
    return raw


def word(raw, address):
    return raw[address] | raw[address + 1] << 8


def rng_next(seed):
    if seed == 0:
        return 0x9146
    result = (seed << 1) & 0xFFFF
    if seed & 0x8000:
        result ^= 0x1D87
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--vectors", type=Path, required=True)
    parser.add_argument("--cases", type=Path, required=True)
    parser.add_argument("--pcs", type=Path, required=True)
    parser.add_argument("--meta", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    vectors, labels, traces = rows(args.vectors), rows(args.cases), rows(args.pcs)
    if not (len(vectors) == len(labels) == len(traces) == 6):
        raise ValueError("expected six aligned native calls")
    meta = json.loads(args.meta.read_text(encoding="utf-8-sig"))
    calls = []
    for number, (vector, label, trace) in enumerate(
            zip(vectors, labels, traces), 1):
        if label["case"] != number or trace["case"] != number:
            raise ValueError("capture sidecars are not aligned")
        if vector["entry_pc"].lower() != "85b130" or \
                vector["exit_pc"].lower() != "85b176":
            raise ValueError("capture did not use the audited native boundary")
        before, after = memory(vector["entry"]), memory(vector["exit"])
        context = word(before, 0x009E)
        opponent = word(before, context + 2)
        observed = {
            "current_score": word(before, context + 0x26),
            "opponent_score": word(before, opponent + 0x26),
            "period_raw_0926": word(before, 0x0926),
            "opponent_activity_raw_39": word(before, opponent + 0x39),
            "rng_seed": word(before, 0x07F6),
            "initial_mode_raw_30": word(before, opponent + 0x30),
        }
        wanted = {
            "current_score": label["current_score"],
            "opponent_score": label["opponent_score"],
            "period_raw_0926": label["period"],
            "opponent_activity_raw_39": label["activity"],
            "rng_seed": label["rng_seed"],
            "initial_mode_raw_30": label["initial_mode"],
        }
        if observed != wanted:
            raise ValueError(f"case {number} entry differs: {observed} != {wanted}")
        random_word = word(after, 0x07F6)
        if random_word != rng_next(observed["rng_seed"]):
            raise ValueError(f"case {number} native RNG result differs")
        mode = word(after, opponent + 0x30)
        if mode != label["expected_mode"]:
            raise ValueError(f"case {number} native mode differs")
        calls.append({
            "name": label["name"],
            "entry_pc": vector["entry_pc"].lower(),
            "exit_pc": vector["exit_pc"].lower(),
            "input": observed,
            "native": {
                "random_word": random_word,
                "selected": observed["opponent_activity_raw_39"] != 0,
                "mode_raw_30": mode,
                "executed": trace["executed"],
            },
        })
    fixture = {
        "schema": 1,
        "source": {
            "oracle": meta["oracle"],
            "controlled": True,
            "protocol": meta["protocol"],
            "rom_sha256": meta["rom_sha256"],
            "mesen_sha256": meta["mesen_sha256"],
            "capture_sha256": sha(args.vectors),
            "cases_sha256": sha(args.cases),
            "pcs_sha256": sha(args.pcs),
            "meta_sha256": sha(args.meta),
        },
        "calls": calls,
    }
    args.output.write_text(
        json.dumps(fixture, indent=2) + "\n", encoding="utf-8")
    print(f"[CPU DEFENSE CONTEXT NORMALIZE] calls={len(calls)}")


if __name__ == "__main__":
    main()
