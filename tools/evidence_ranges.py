"""Strict parsing for machine-readable ROM evidence ranges.

The verified-routine ledger permits one entry to name several disjoint ranges,
separated by semicolons.  Aggregate classification entries remain useful
documentation, but must opt out of instruction/address coverage so that a
whole-bank label cannot automatically verify code captured later.
"""

import hashlib
from pathlib import Path
import re


RANGE_SEGMENT = re.compile(r"([0-9A-Fa-f]{6})-([0-9A-Fa-f]{6})")


def text_source_sha256(path):
    """Hash UTF-8 source with LF newlines, independent of Git autocrlf.

    This is only for checked-in text/tool provenance. Native ROM, capture,
    fixture and executable identities must continue to hash their raw bytes.
    """
    text = Path(path).read_text(encoding="utf-8")
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def parse_range_expression(value):
    if not isinstance(value, str) or not value:
        raise ValueError("evidence range must be a non-empty string")
    result = []
    for raw_segment in value.split(";"):
        segment = raw_segment.strip()
        match = RANGE_SEGMENT.fullmatch(segment)
        if not match:
            raise ValueError(f"invalid evidence range segment: {segment!r}")
        start, end = (int(part, 16) for part in match.groups())
        if end < start:
            raise ValueError(f"reversed evidence range segment: {segment!r}")
        if start >> 16 != end >> 16:
            raise ValueError(f"cross-bank evidence range segment: {segment!r}")
        result.append((start, end))
    return result


def coverage_intervals(entries):
    validate_aggregate_opt_out(entries)
    intervals = []
    for entry in entries:
        parsed = parse_range_expression(entry.get("range"))
        if ("coverage_credit" in entry and
                not isinstance(entry["coverage_credit"], bool)):
            raise ValueError("coverage_credit must be a JSON boolean")
        if entry.get("coverage_credit", True):
            intervals.extend(parsed)
    return intervals


def validate_aggregate_opt_out(entries):
    """Reject aggregate credit that can silently absorb future captures.

    Historical ``host equivalent`` rows describe architectural ownership,
    not one native routine with a complete entry/exit oracle.  They remain
    useful ledger notes, but—as with whole-bank closure rows—must never make
    newly observed instructions look verified without new evidence. Count
    each entry's union, not its component widths: semicolon splitting cannot
    turn a broad claim into precise evidence. Names remain a supplementary
    heuristic, not proof that a smaller renamed claim is semantically exact.
    """
    for entry in entries:
        if ("coverage_credit" in entry and
                not isinstance(entry["coverage_credit"], bool)):
            raise ValueError("coverage_credit must be a JSON boolean")
        name = str(entry.get("name", "")).lower()
        aggregate_label = ("host equivalent" in name or
                           re.search(r"\baggregate\b", name) is not None)
        union_width = 0
        merged_end = -1
        for start, end in sorted(parse_range_expression(entry.get("range"))):
            uncovered_start = max(start, merged_end + 1)
            if uncovered_start <= end:
                union_width += end - uncovered_start + 1
            merged_end = max(merged_end, end)
        if ((union_width >= 0x4000 or aggregate_label) and
                entry.get("coverage_credit", True)):
            raise ValueError(
                f"aggregate range {entry['range']} must set "
                "coverage_credit=false"
            )
