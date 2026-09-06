"""Focused contract checks for the bounded CPU gameplay trace sequence."""

import contextlib
import json
import tempfile
from pathlib import Path

from test_cpu_gameplay import JsonlRows


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def main():
    source_rows = [
        {"frame": 1, "nested": {"values": [1, 2]}},
        {"frame": 2, "nested": {"values": [3, 4]}},
        {"frame": 3, "nested": {"values": [5, 6]}},
    ]
    with tempfile.TemporaryDirectory() as directory:
        trace = Path(directory) / "trace.jsonl"
        with trace.open("w", encoding="utf-8", newline="\n") as output:
            output.write("\n")
            for row in source_rows:
                output.write(json.dumps(row) + "\n")
            output.write("   \n")

        rows = JsonlRows(trace)
        spool_path = rows._spool_path
        require(list(rows) == source_rows, "full iteration changed rows")
        require(list(rows[1:]) == source_rows[1:], "slice iteration changed rows")
        require(list(rows[::-1]) == source_rows[::-1],
                "strided slice iteration changed rows")
        require(rows[0] == source_rows[0] and rows[-1] == source_rows[-1],
                "random or negative indexing changed rows")
        require(list(rows.where(lambda row: row["frame"] % 2)) ==
                source_rows[::2], "filtered view changed rows")
        require(list(zip(rows[:-1], rows[1:])) ==
                list(zip(source_rows[:-1], source_rows[1:])),
                "concurrent adjacent iterators changed rows")
        rows.close()
        require(not spool_path.exists(), "close retained the private spool")

        try:
            with contextlib.ExitStack() as cleanup:
                rows = JsonlRows(trace)
                cleanup.callback(rows.close)
                failed_spool_path = rows._spool_path
                raise AssertionError("synthetic suite assertion")
        except AssertionError as error:
            require(str(error) == "synthetic suite assertion",
                    "cleanup changed the assertion failure")
        require(not failed_spool_path.exists(),
                "assertion cleanup retained the private spool")

        malformed = Path(directory) / "malformed.jsonl"
        malformed.write_text('{"frame": 1}\n{"frame":', encoding="utf-8")
        try:
            JsonlRows(malformed)
        except json.JSONDecodeError:
            pass
        else:
            raise AssertionError("malformed JSONL was accepted")

    print("JsonlRows bounded decode-cache checks passed")


if __name__ == "__main__":
    main()
