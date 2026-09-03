"""Regression checks for checked-in ROM census artifacts."""

import argparse
import json
import subprocess
import sys
import tempfile
from pathlib import Path

from evidence_ranges import (coverage_intervals, parse_range_expression,
                             text_source_sha256, validate_aggregate_opt_out)

ROOT = Path(__file__).resolve().parent.parent


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def verify_evidence_ranges(entries):
    validate_aggregate_opt_out(entries)
    for aggregate in ({
            "range": "83C000-83EFFF",
            "name": "aggregate host equivalents",
        }, {
            "range": "808001-80FFFF",
            "name": "renamed broad boundary",
        }, {
            "range": "858000-85BFFE;85BFFF-85FFFD;85FFFE-85FFFF",
            "name": "split broad boundary",
        }, {
            "range": "858000-859FFF;85A000-85BFFF",
            "name": "adjacent pieces exactly at the limit",
        }, {
            "range": "858000-859FFF;85E000-85FFFF",
            "name": "disjoint pieces exactly at the limit",
        }, {
            "range": "858000-859FFF;868000-869FFF",
            "name": "multi-bank pieces exactly at the limit",
        }, {
            "range": "83C000-83EFFF",
            "name": "event classification aggregate",
        }):
        try:
            coverage_intervals([aggregate])
        except ValueError:
            pass
        else:
            raise AssertionError("aggregate coverage credit was accepted")
        require(coverage_intervals([
            {**aggregate, "coverage_credit": False}
        ]) == [], "explicitly non-credit aggregate was rejected or credited")
    # Overlap/duplication must not inflate the union. The first expression
    # spans exactly 0x3FFF bytes; naive component summation would reject it.
    for precise in (
            "858000-85AFFF;859000-85BFFE",
            "858000-85AFFF;858000-85AFFF",
            "859962-859983;859987-8599DC;8599E0-859A13"):
        require(coverage_intervals([{"range": precise, "name": "bounded routine"}]) ==
                parse_range_expression(precise),
                "precise overlapping/disjoint ranges were falsely aggregated")
    for invalid in (
            "", "879CBF-87A017;", "87A017-879CBF", "87FF00-880100",
            "879CBF-87A017junk", "879CBF-87A017;;87A018-87A045",
            "0x879CBF-87A017", None, True, 123):
        try:
            parse_range_expression(invalid)
        except ValueError:
            pass
        else:
            raise AssertionError(f"invalid evidence range accepted: {invalid!r}")
    try:
        coverage_intervals([{
            "range": "879CBF-87A017", "coverage_credit": "false"
        }])
    except ValueError:
        pass
    else:
        raise AssertionError("non-boolean coverage_credit was accepted")
    require(coverage_intervals([{
        "range": "879CBF-87A017", "coverage_credit": False
    }]) == [], "coverage opt-out still contributed an interval")
    require(len(parse_range_expression(
        "879CBF-87A017;87A15C-87A2FD;859530-859597")) == 3,
        "disjoint verified ranges must remain independently parseable")
    expected_intervals = sum(
        len(parse_range_expression(entry["range"]))
        for entry in entries if entry.get("coverage_credit", True))
    require(len(coverage_intervals(entries)) == expected_intervals,
            "coverage range parsing discarded or invented precise entries")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--capture-root", type=Path, default=ROOT / ".analysis")
    parser.add_argument("--recomp", type=Path, default=ROOT.parent / "NBA-Live-95-Recomp")
    args = parser.parse_args()
    progress_args = ["--capture-root", str(args.capture_root), "--recomp", str(args.recomp)]
    with tempfile.TemporaryDirectory() as temp:
        lf, crlf, changed = (Path(temp) / name for name in ("lf", "crlf", "changed"))
        lf.write_bytes(b"native evidence\nsecond line\n")
        crlf.write_bytes(b"native evidence\r\nsecond line\r\n")
        changed.write_bytes(b"different evidence\nsecond line\n")
        require(text_source_sha256(lf) == text_source_sha256(crlf),
                "source hashes must survive Git newline conversion")
        require(text_source_sha256(lf) != text_source_sha256(changed),
                "source hash normalization must not hide content changes")
    ledger = json.loads((ROOT / "docs/verified-routines.json").read_text())
    entries = ledger["routines"]
    verify_evidence_ranges(entries)

    progress = subprocess.run(
        [sys.executable, str(ROOT / "tools/progress.py"), *progress_args], cwd=ROOT,
        check=True, capture_output=True, text=True)
    require("[PROGRESS]" in progress.stdout,
            "progress evidence report did not complete")
    if any(args.capture_root.rglob("exec_*.txt")):
        with tempfile.TemporaryDirectory() as temp:
            live_progress = Path(temp) / "progress.md"
            subprocess.run([
                sys.executable, str(ROOT / "tools/progress.py"),
                "--write", str(live_progress), *progress_args
            ], cwd=ROOT, check=True, capture_output=True, text=True)
            require(live_progress.read_text(encoding="utf-8") ==
                    (ROOT / "docs/progress.md").read_text(encoding="utf-8"),
                    "checked-in progress.md is stale")

    census = json.loads((ROOT / "docs/full-rom-instruction-census.json").read_text())
    require(census["schema"] == 1, "unexpected census schema")
    expected_inputs = {
        "text_hash_policy": "UTF-8 with LF newlines",
        "verified_routines_sha256": text_source_sha256(
            ROOT / "docs/verified-routines.json"),
        "census_tool_sha256": text_source_sha256(ROOT / "tools/full_rom_census.py"),
        "evidence_policy_sha256": text_source_sha256(ROOT / "tools/evidence_ranges.py"),
    }
    require(census.get("inputs") == expected_inputs,
            "checked-in census was generated from stale evidence or tooling")
    require(len(census["banks"]) == 48, "census must describe all 48 physical banks")
    totals = census["totals"]
    require(totals["rom_bytes"] == 48 * 0x8000, "unexpected US ROM size")
    require(totals["decoded_starts"] > 0, "census decoded no instructions")
    require(totals["verified_starts"] <= totals["decoded_starts"],
            "verified starts exceed the decoded universe")
    require(totals["decoded_bytes"] + totals["undecoded_bytes"] == totals["rom_bytes"],
            "decoded and undecoded byte totals do not cover the ROM")
    listings = args.capture_root / "full-rom-census/listings"
    if any(listings.glob("bank_*_instructions.tsv")):
        with tempfile.TemporaryDirectory() as temp:
            live_md = Path(temp) / "census.md"
            live_json = Path(temp) / "census.json"
            subprocess.run([
                sys.executable, str(ROOT / "tools/full_rom_census.py"),
                "report", "--out", str(listings.parent),
                "--capture-root", str(args.capture_root),
                "--write-md", str(live_md), "--write-json", str(live_json)
            ], cwd=ROOT, check=True, capture_output=True, text=True)
            require(live_md.read_text(encoding="utf-8") ==
                    (ROOT / "docs/full-rom-instruction-census.md").read_text(encoding="utf-8"),
                    "checked-in full-ROM census Markdown is stale")
            require(live_json.read_text(encoding="utf-8") ==
                    (ROOT / "docs/full-rom-instruction-census.json").read_text(encoding="utf-8"),
                    "checked-in full-ROM census JSON is stale")
    print("project census regression: pass")


if __name__ == "__main__":
    main()
