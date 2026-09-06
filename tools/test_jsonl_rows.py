"""Focused contract checks for the bounded CPU gameplay trace sequence."""

import contextlib
import json
import pickle
import tempfile
from pathlib import Path
from unittest import mock

from test_cpu_gameplay import JsonlRows


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def main():
    source_rows = [
        {"frame": frame, "nested": {"values": [frame, frame + 1]}}
        for frame in range(1, 13)
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
        first_iterator = iter(rows)
        second_iterator = iter(rows)
        first_snapshot = next(first_iterator)
        second_snapshot = next(second_iterator)
        first_snapshot["nested"]["values"].append(99)
        require(second_snapshot == source_rows[0] and
                next(iter(rows)) == source_rows[0],
                "default iterators stopped returning independent snapshots")
        first_iterator.close()
        second_iterator.close()
        rows.close()
        require(not spool_path.exists(), "close retained the private spool")

        shared_rows = JsonlRows(trace, read_only=True)
        with mock.patch("test_cpu_gameplay.pickle.load",
                        wraps=pickle.load) as decode:
            adjacent = list(zip(shared_rows[:-1], shared_rows[1:]))
        require(adjacent == list(zip(source_rows[:-1], source_rows[1:])),
                "shared-cache adjacent iteration changed rows")
        require(decode.call_count == len(source_rows),
                "adjacent iterators decoded a shared row more than once")
        require(adjacent[0][1] is adjacent[1][0],
                "read-only iterators did not share decoded rows")
        require(len(shared_rows._cache) <= shared_rows.CACHE_ROWS,
                "adjacent iteration exceeded the bounded cache")

        shared_rows._cache.clear()
        with mock.patch("test_cpu_gameplay.pickle.load",
                        wraps=pickle.load) as decode:
            adjacent_frames = []
            for index, row in enumerate(shared_rows[1:], 1):
                adjacent_frames.append(
                    (shared_rows[index - 1]["frame"], row["frame"]))
        require(adjacent_frames == [
                    (row["frame"], source_rows[index + 1]["frame"])
                    for index, row in enumerate(source_rows[:-1])
                ], "indexed look-behind changed rows")
        require(decode.call_count == len(source_rows),
                "indexed look-behind decoded a shared row more than once")
        require(len(shared_rows._cache) <= shared_rows.CACHE_ROWS,
                "indexed look-behind exceeded the bounded cache")
        shared_spool_path = shared_rows._spool_path
        shared_rows.close()
        require(not shared_spool_path.exists(),
                "shared-cache close retained the private spool")

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
            JsonlRows(malformed, read_only=True)
        except json.JSONDecodeError:
            pass
        else:
            raise AssertionError("malformed JSONL was accepted")

    print("JsonlRows bounded decode-cache checks passed")


if __name__ == "__main__":
    main()
