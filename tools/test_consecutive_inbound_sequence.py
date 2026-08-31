"""Lock the natural tip-off pass/catch and first inbound presentation sequence.

The input must come from a test-only ``--tipoff-only --frames 850`` gameplay
trace.  This checker does not seed or mutate normal runtime state.
"""
import argparse
import json
from pathlib import Path


def first(rows, predicate, label):
    for row in rows:
        if predicate(row):
            return row
    raise AssertionError(f"missing {label}")


def ball_xyz(row):
    ball = row["ball"]
    return ball["x"], ball["y"], ball["z"]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--trace", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    rows = [json.loads(line) for line in args.trace.read_text().splitlines()]
    assert len(rows) >= 850

    pass_start = first(rows, lambda r:
        r["possession"]["pass_active_raw"] == 1 and
        r["possession"]["pass_actor_raw"] == 3 and
        r["possession"]["pass_receiver_raw"] == 4, "ordinary pass start")
    pass_release = first(rows[pass_start["frame"]:], lambda r:
        r["possession"]["pass_actor_raw"] == 3 and
        r["possession"]["pass_receiver_raw"] == 4 and
        r["ball"]["owner"] == -1, "ordinary pass release")
    pass_catch = first(rows[pass_release["frame"]:], lambda r:
        r["possession"]["actor"] == 4 and r["ball"]["owner"] == 4,
        "ordinary pass catch")
    assert [pass_start["frame"], pass_release["frame"], pass_catch["frame"]] == [306, 320, 342]
    assert [ball_xyz(row) for row in (pass_start, pass_release, pass_catch)] == [
        (-119, 99, 47), (-142, 106, 67), (-90, 3, 67)]

    dead = first(rows, lambda r: r["match"]["live_state_raw"] == 0x82,
                 "first dead-ball inbound")
    installed = first(rows[dead["frame"]:], lambda r:
        r["possession"]["actor"] == 7 and r["ball"]["owner"] == -1,
        "inbound carrier install")
    ready = first(rows[installed["frame"]:], lambda r:
        r["match"]["inbound_ready_raw"] == 1, "inbound arrival")
    transfer = first(rows[ready["frame"]:], lambda r:
        r["match"]["inbound_transfer_raw"] == 1, "inbound transfer")
    assert [dead["frame"], installed["frame"], ready["frame"], transfer["frame"]] == [506, 526, 546, 674]

    # F58F intentionally refuses a CPU pass while $092E >= 240, then uses
    # the ROM random due gate.  The 128-frame ready hold is original timing,
    # not a port stall.  During it, B649/B832 keeps the visible ball at the
    # carrier's literal actor+$28 attachment point while logical owner stays
    # negative until AB2D.
    ready_hold = rows[ready["frame"] - 1:transfer["frame"] - 1]
    assert len(ready_hold) == 128
    assert all(r["match"]["inbound_ready_raw"] == 1 and
               r["match"]["inbound_transfer_raw"] == 0 and
               r["possession"]["actor"] == 7 and
               r["ball"]["owner"] == -1 and r["ball"]["state"] == 6
               for r in ready_hold)
    # Arrival retains the independent low masks for two submitted frames;
    # the following animation publication clears them and B832 returns to the
    # standing point.  The retired direction-only adapter missed that hand
    # transition and rendered (-375,92,26) for all 128 frames.
    assert [ball_xyz(r) for r in ready_hold[:2]] == [(-391, 76, 26)] * 2
    assert {ball_xyz(r) for r in ready_hold[2:]} == {(-375, 92, 26)}
    assert transfer["ball"]["owner"] == 7 and transfer["ball"]["state"] == 4
    assert transfer["possession"]["pass_actor_raw"] == 7
    assert transfer["possession"]["pass_receiver_raw"] == 9

    # The first receiver is cancelled at the original contact gate.  The
    # carrier remains live, selects actor 8 on its next due gate, and releases
    # that second inbound pass without a host timer shortcut.
    retry = first(rows[transfer["frame"]:], lambda r:
        r["match"]["inbound_transfer_raw"] == 1 and
        r["possession"]["pass_receiver_raw"] == 8, "inbound retry")
    release = first(rows[retry["frame"]:], lambda r:
        r["possession"]["actor"] == -1 and r["ball"]["owner"] == -1,
        "inbound release")
    assert [retry["frame"], release["frame"]] == [726, 752]
    assert ball_xyz(release) == (-382, 90, 62)

    report = {
        "passed": True,
        "ordinary_pass_frames": [306, 320, 342],
        "inbound_frames": [506, 526, 546, 674, 726, 752],
        "ready_hold_frames": 128,
        "arrival_attachment_xyz": [-391, 76, 26],
        "standing_attachment_xyz": [-375, 92, 26],
        "normal_runtime_seeded": False,
        "limits": "Deterministic production-caller regression for the default CPU tip-off journey; not a claim about every inbound layout."
    }
    args.output.mkdir(parents=True, exist_ok=False)
    (args.output / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    print("PASS: ordinary pass/catch and consecutive inbound presentation sequence")


if __name__ == "__main__":
    main()
