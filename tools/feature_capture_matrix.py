"""Validate and render the whole-game feature/capture matrix."""

import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
LEVELS = {"none", "weak", "partial", "strong", "untriaged"}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", default=ROOT / "docs/feature-capture-matrix.json", type=Path)
    parser.add_argument("--output", default=ROOT / "docs/feature-capture-matrix.md", type=Path)
    args = parser.parse_args()
    data = json.loads(args.input.read_text(encoding="utf-8"))
    features = data["features"]
    if sum(row["weight"] for row in features) != 100:
        raise SystemExit("Feature weights must total 100")
    ids = [row["id"] for row in features]
    if len(ids) != len(set(ids)):
        raise SystemExit("Feature ids must be unique")
    dimensions = ("native_capture", "ghidra", "recomp", "differential", "port_tests")
    for row in features:
        if not 0 <= row["completion"] <= 100:
            raise SystemExit(f"Invalid completion for {row['id']}")
        if any(row[key] not in LEVELS for key in dimensions):
            raise SystemExit(f"Invalid evidence level for {row['id']}")
        for evidence in row["evidence"]:
            if not (ROOT / evidence).exists():
                raise SystemExit(f"Missing evidence for {row['id']}: {evidence}")
    estimate = sum(row["weight"] * row["completion"] for row in features) / 100
    lines = [
        "# Whole-game feature/capture matrix", "",
        "This is the planning view of the retail game. It is deliberately separate from",
        "the native instruction census: feature completion is a weighted engineering",
        "estimate, while capture/Ghidra/recomp/differential columns describe evidence",
        "strength. `strong` does not mean a feature is complete.", "",
        f"**Current weighted whole-game estimate: {estimate:.2f}%**", "",
        "| feature | weight | completion | native | Ghidra | recomp | differential | tests |",
        "|---|---:|---:|---|---|---|---|---|",
    ]
    for row in features:
        lines.append(
            f"| {row['feature']} | {row['weight']}% | {row['completion']}% | "
            f"{row['native_capture']} | {row['ghidra']} | {row['recomp']} | "
            f"{row['differential']} | {row['port_tests']} |"
        )
    lines += ["", "## Remaining work and evidence", ""]
    for row in features:
        links = ", ".join(f"[`{path}`](../{path})" for path in row["evidence"])
        lines += [f"### {row['feature']}", "", row["remaining"], "", f"Evidence: {links}", ""]
    lines += [
        "## Updating this matrix", "",
        "Edit `docs/feature-capture-matrix.json`, cite retained evidence, then run:", "",
        "```powershell", "python tools/feature_capture_matrix.py", "```", "",
        "The generator rejects duplicate IDs, missing evidence, invalid levels, completion",
        "outside 0-100, and weights that do not total 100.", "",
    ]
    args.output.write_text("\n".join(lines), encoding="utf-8")
    print(f"[MATRIX] {len(features)} features; weighted completion={estimate:.2f}%")


if __name__ == "__main__":
    main()
