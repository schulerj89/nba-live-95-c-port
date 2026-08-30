"""Verify native provenance, generated inputs, and production recovery binding."""
import argparse
import base64
import hashlib
import json
from pathlib import Path
import subprocess
import zlib

from normalize_inbound_cancel_recovery import ACTOR, GLOBALS, header, word

ROM_SHA256 = "2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870"
SOURCE_SHA256 = "c938c230cf6a0d86054e030f99d66c9e0a72ea317a2c847254fd1c15837255a2"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--fixture", required=True, type=Path)
    parser.add_argument("--header", required=True, type=Path)
    parser.add_argument("--probe", required=True)
    parser.add_argument("--assets", required=True)
    parser.add_argument("--rom", required=True, type=Path)
    args = parser.parse_args()
    fixture = json.loads(args.fixture.read_text())
    meta = fixture["provenance"]
    assert fixture["schema"] == 1 and meta["controlled"] is True
    assert fixture["globals"] == GLOBALS and fixture["actor_offsets"] == ACTOR
    assert meta["entry_pc"] == "86F43A" and meta["exit_pc"] == "86F58F"
    assert meta["rom_file_sha256"] == hashlib.sha256(args.rom.read_bytes()).hexdigest() == ROM_SHA256
    raw = zlib.decompress(base64.b64decode(fixture["raw_jsonl_zlib_base64"]))
    assert hashlib.sha256(raw).hexdigest() == meta["vectors_sha256"] == SOURCE_SHA256
    calls = [json.loads(line) for line in raw.splitlines()]
    assert len(calls) == meta["cases"] == 4
    assert [row["entry_frame"] for row in calls] == [4805, 4807, 4809, 4812]
    for row, inputs in zip(calls, [(1, 0xFFFF), (1, 0), (0xA5A5, 0xFFFF), (0, 0xFFFF)]):
        assert row["controlled"] is True
        assert row["entry_pc"] == "86f43a" and row["exit_pc"] == "86f58f"
        before = bytes.fromhex(row["entry"]["mem"]["0000"])
        after = bytes.fromhex(row["exit"]["mem"]["0000"])
        assert len(before) == len(after) == 0x4B00
        actor = word(before, 0x96)
        assert actor == 0x34EB + word(before, 0x93E) * 0x100
        assert (word(before, 0x9B8), word(before, 0x946)) == inputs
        assert word(before, 0x936) == 0x82 and word(before, actor + 0x5E) == 11
        for target, position in [(0x958, actor + 4), (0x95A, actor + 8)]:
            delta = (word(before, target) - word(before, position) + 0x8000) % 0x10000 - 0x8000
            assert -9 <= delta <= 8
        assert (0x86F57F in row["executed"]) == (inputs[1] >= 0x8000)
        assert word(after, 0x9B8) == (0 if inputs[1] >= 0x8000 else inputs[0])
        assert word(before, 0x946) == word(after, 0x946)
        assert word(before, 0x7F6) == word(after, 0x7F6)
    assert args.header.read_text() == header(calls), "C inputs differ from native fixture"
    result = subprocess.run([args.probe, args.assets, "--recovery-only"],
                            capture_output=True, text=True, check=False)
    if result.returncode:
        raise AssertionError(result.stdout + result.stderr)
    assert "four controlled native projections + attached whole-update binding" in result.stdout
    print("[INBOUND CANCEL] PASS: four native calls, ROM/source hashes, exact generated inputs, production recovery")


if __name__ == "__main__":
    main()
