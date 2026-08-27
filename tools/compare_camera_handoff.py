"""Diagnostic camera trajectory comparison, NOT a routine parity test.

Uses native routine-entry frame labels and port scene-frame labels directly.
PNG end-frame labels are different; no fitted time offset is applied here.
"""
import argparse
import json
from pathlib import Path


def rows(path):
    return [json.loads(line) for line in Path(path).read_text().splitlines() if line]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("native", "before", "after"):
        parser.add_argument("--" + name, required=True)
    parser.add_argument("--start", type=int, default=201)
    parser.add_argument("--end", type=int, default=239)
    args = parser.parse_args()
    native = [row for row in rows(args.native) if row["kind"] == "core"
              and not row["controlled"] and args.start <= row["frame"] <= args.end]
    assert native, "No natural camera calls in requested range"
    result = {"native_call_frames": [row["frame"] for row in native], "samples": len(native)}
    for name in ("before", "after"):
        port = {row["scene_frame"]: row["camera"] for row in rows(getattr(args, name))}
        result[name] = {}
        for axis, index in (("x", 0), ("y", 1)):
            errors = []
            for row in native:
                expected = row["expected"][index]
                if expected >= 32768:
                    expected -= 65536
                errors.append(abs(port[row["frame"]][axis] - expected))
            result[name][axis] = {"mean_absolute_error": sum(errors) / len(errors),
                                  "max_absolute_error": max(errors)}
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
