"""Append lossless controlled native context-anchor witnesses, never C goldens."""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path

from normalize_formation_route import memory, projected, word

ROM_SHA256 = "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--capture", required=True)
    parser.add_argument("--fixture", required=True)
    args = parser.parse_args()
    capture = Path(args.capture)
    source = capture / "formation_anchors.vectors.jsonl"
    meta = json.loads((capture / "formation_anchors.meta.json").read_text(encoding="utf-8-sig"))
    if meta.get("entry", "").lower() != "85ad6b" or \
            [x.lower() for x in meta.get("exits", [])] != ["85ad77", "85af5b"] or \
            meta.get("rom_file_sha256") != ROM_SHA256 or \
            meta.get("vectors_sha256") != hashlib.sha256(source.read_bytes()).hexdigest():
        parser.error("native capture identity/boundary mismatch")
    vectors = [json.loads(line) for line in source.read_text().splitlines() if line]
    labels = [json.loads(line) for line in (capture / "formation-anchor-cases.jsonl").read_text().splitlines() if line]
    traces = [json.loads(line) for line in (capture / "formation-anchor-pcs.jsonl").read_text().splitlines() if line]
    if len(vectors) != 32 or len(labels) != 32 or len(traces) != 32:
        parser.error("expected all 32 controlled context-anchor witnesses")
    fixture_path = Path(args.fixture)
    fixture = json.loads(fixture_path.read_text())
    retained = [call for call in fixture["calls"] if call["source"] != "anchor"]
    if Counter(call["source"] for call in retained) != {
            "natural": 24, "early": 8, "inbound": 8,
            "special": 8, "edge": 8, "timer": 8}:
        parser.error("existing 64 native route witnesses changed")
    base = memory(vectors[0]["entry"])
    calls = []
    census = set()
    for vector, label, trace in zip(vectors, labels, traces):
        before, after = memory(vector["entry"]), memory(vector["exit"])
        slot = word(before, 0xC2)
        actor = 0x34EB + slot * 0x100
        context = 0x46EB if slot < 5 else 0x476B
        if vector["entry_pc"].lower() != "85ad6b" or vector["exit_pc"].lower() != "85af5b" or \
                label["case"] != vector["call"] or trace["case"] != vector["call"] or \
                word(before, 0xC6) != 2 or slot != label["slot"] or \
                word(before, 0x96) != actor or word(before, 0x9E) != context or \
                word(before, 0xE0) != word(before, 0x3449 + slot * 4) or \
                word(before, 0xE2) != word(before, 0x344B + slot * 4) or \
                word(before, context + 0x0A) != label["anchor"] & 0xFFFF or \
                word(before, actor + 0x6E) != (0 if slot < 5 else 5) or \
                word(before, 0x996) != label["play"] or word(before, 0x99C) != label["mirror"]:
            parser.error(f"controlled call {vector['call']} input identity mismatch")
        pcs = trace["executed"]
        if pcs[0] != 0x85AD6B or pcs[-1] != 0x85AF5B:
            parser.error("incomplete executed native route")
        if label["kind"] == "special":
            if 0x85AE2C not in pcs or word(after, actor + 0x56) != label["anchor"] & 0xFFFF or \
                    word(after, actor + 0x58) != 0 or word(after, actor + 0x7E) & 8:
                parser.error("special cutter does not preserve native live-anchor contract")
        elif 0x85ADF5 not in pcs or ((0x85ADFC in pcs) != (label["anchor"] < 0)):
            parser.error("ordinary native formation context-sign branch changed")
        census.add((slot, label["anchor"], label["kind"], label["mirror"]))
        calls.append({"source": "anchor", **label, "native_call": vector["call"],
                      "entry_frame": vector["entry_frame"], "exit_frame": vector["exit_frame"],
                      "exit": vector["exit_pc"], "executed": pcs,
                      "patches": [[a, value] for a, value in enumerate(before) if value != base[a]],
                      "expected": projected(after, True)})
    if len(census) != 32:
        parser.error("duplicated controlled anchor branch")
    fixture.update({"schema": "nba95-formation-route-v2",
                    "anchor_base_input": base.hex(),
                    "anchor_provenance": {**meta, "source": source.as_posix()},
                    "calls": retained + calls})
    fixture_path.write_text(json.dumps(fixture, separators=(",", ":")) + "\n", encoding="utf-8")
    print(f"[FORMATION ANCHORS] retained_native=64 controlled_anchor_cases=32 total=96")


if __name__ == "__main__":
    main()
