"""Prove the mode-fourteen fixture validator rejects representative mutations."""

import argparse
import copy
import hashlib
import json
from pathlib import Path

from verify_cpu_mode_fourteen_vectors import EXPECTED_FIXTURE, validate


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--vectors", type=Path, required=True)
    args = parser.parse_args()
    raw = args.vectors.read_bytes()
    if hashlib.sha256(raw).hexdigest() != EXPECTED_FIXTURE:
        raise ValueError("source fixture hash changed")
    source = json.loads(raw)
    validate(source)
    mutations = []

    def add(name, change):
        mutated = copy.deepcopy(source)
        change(mutated)
        mutations.append((name, mutated))

    add("schema-bool", lambda d: d.__setitem__("schema", True))
    add("extra-top-level", lambda d: d.__setitem__("unexpected", 1))
    add("routine", lambda d: d.__setitem__("routine", "changed"))
    add("provenance", lambda d: d.__setitem__("provenance", "changed"))
    add("domain", lambda d: d.__setitem__("domain", "changed"))
    add("source-hash", lambda d: d["source"].__setitem__("vectors_sha256", "0" * 64))
    add("field-name", lambda d: d["fields"].__setitem__(0, "changed"))
    add("word-range", lambda d: d["calls"][0]["input"].__setitem__(0, 0x10000))
    add("word-type", lambda d: d["calls"][0]["expected"].__setitem__(0, 0.0))
    add("path", lambda d: d["calls"][0]["executed"].__setitem__(0, "86b156"))
    add("child-count", lambda d: d["calls"][0]["child_calls"].__setitem__("finish_86a9d0", 2))
    add("extra-call-key", lambda d: d["calls"][0].__setitem__("unexpected", 1))

    accepted = []
    for name, document in mutations:
        try:
            validate(document)
        except ValueError:
            continue
        accepted.append(name)
    if accepted:
        raise AssertionError(f"validator accepted mutations: {accepted}")
    print(f"[CPU MODE FOURTEEN FIXTURE] PASS: rejected={len(mutations)} before replay")


if __name__ == "__main__":
    main()
