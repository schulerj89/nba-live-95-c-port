"""C runtime assertions for source-classified interrupted passes, not ROM parity."""


class PassInterruptionGuard:
    def __init__(self):
        self.interrupted = {}
        self.receiver_cancel = None
        self.entries = self.recoveries = self.receiver_clears = 0
        self.release_before_knockdowns = 0

    def observe(self, previous, row):
        p, oldp = row["possession"], previous["possession"]
        contact = row["collision"]
        for actor_id in tuple(self.interrupted):
            episode = self.interrupted[actor_id]
            old = previous["actors"][actor_id]["raw"]
            now = row["actors"][actor_id]["raw"]
            # Cancellation/new initialization has a separate original owner;
            # the current mode8 executor must never manufacture a release.
            if old["control_mode"] == 8:
                new_pass = now["control_mode"] == 15 and \
                    p["pass_active_raw"] and \
                    p["pass_actor_raw"] == actor_id and \
                    p["pass_receiver_raw"] >= 0 and \
                    (p["pass_receiver_raw"] != oldp["pass_receiver_raw"] or
                     p["pass_distance_raw"] != oldp["pass_distance_raw"])
                if new_pass:
                    # AB2D owns a fresh pass initialization and may replace
                    # the metadata retained by the interrupted executor.
                    del self.interrupted[actor_id]
                    continue
                if now["pass_released"] != old["pass_released"]:
                    raise AssertionError("mode8 manufactured a pass release")
                for name in ("pass_band_62", "pass_family_c0"):
                    if now[name] != old[name]:
                        raise AssertionError("mode8 changed preserved pass metadata")
                if now["control_mode"] != 8:
                    if now["control_mode"] not in (1, 2, 11):
                        raise AssertionError("unclassified interrupted-pass recovery")
                    if now["contact_inhibit_5a"] != 0:
                        raise AssertionError("C748 recovery failed to clear contact inhibit")
                    self.recoveries += 1
                    del self.interrupted[actor_id]
                elif row["simulation_tick"] - episode > 180:
                    raise AssertionError("interrupted passer did not recover")

        # Identify the interrupted executor from BEFORE state. Selecting it
        # from after-state lets erased/redirected pass globals evade checks.
        actor_id = oldp["pass_actor_raw"]
        if oldp["pass_active_raw"] and 0 <= actor_id < 10:
            old = previous["actors"][actor_id]["raw"]
            now = row["actors"][actor_id]["raw"]
            if now["control_mode"] == 8 and old["control_mode"] != 8:
                if old["control_mode"] != 15 or old["pass_released"]:
                    # Residual 09C4 may survive a previously completed pass;
                    # this is not a new interrupted mode15 executor.
                    return
                if (contact["player_count"] <= 0 or
                        contact["player_routine"] not in (0x86BFBA, 0x86C91E) or
                        actor_id not in (contact["player_a"], contact["player_b"]) or
                        row["actors"][actor_id]["animation"] not in (0x35, 0x36)):
                    raise AssertionError("pass left mode15 without classified knockdown")
                # Nondeferred mode 15 runs `$86:A6B3->$A749/$99C4`
                # in the common actor sweep before the later player-contact
                # sweep. A completed pass may therefore be followed by BFBA
                # changing that same actor from mode 15 to mode 8.
                released_before_knockdown = (
                    now["pass_released"] and not old["pass_released"] and
                    p["pass_actor_raw"] == oldp["pass_actor_raw"] == actor_id and
                    p["pass_receiver_raw"] == oldp["pass_receiver_raw"] and
                    not p["pass_active_raw"] and
                    oldp["actor"] == actor_id and p["actor"] == -1 and
                    previous["ball"]["owner"] == actor_id and
                    previous["ball"]["state"] == 4 and
                    row["ball"]["owner"] == -1 and
                    row["ball"]["state"] == 3 and
                    row["match"]["live_state_raw"] == 0 and
                    now["pass_band_62"] == old["pass_band_62"] and
                    now["pass_family_c0"] == old["pass_family_c0"])
                if released_before_knockdown:
                    self.release_before_knockdowns += 1
                    return
                for name in ("pass_actor_raw", "pass_receiver_raw", "pass_active_raw"):
                    if p[name] != oldp[name]:
                        raise AssertionError("knockdown changed passing actor's globals")
                for name in ("pass_band_62", "pass_family_c0", "pass_released"):
                    if now[name] != old[name]:
                        raise AssertionError("knockdown changed passing actor's metadata")
                self.interrupted[actor_id] = row["simulation_tick"]
                self.entries += 1

    def receiver_only_clear(self, previous, row):
        p, oldp = row["possession"], previous["possession"]
        key = (p["pass_actor_raw"], p["pass_receiver_raw"])
        if key[0] < 0 or key[1] != -1:
            return False
        if self.receiver_cancel == key and (
                oldp["pass_actor_raw"], oldp["pass_receiver_raw"]) == key:
            return True
        receiver = oldp["pass_receiver_raw"]
        if not 0 <= receiver < 10:
            return False
        contact = row["collision"]
        if (p["pass_actor_raw"] != oldp["pass_actor_raw"] or
                p["pass_active_raw"] != oldp["pass_active_raw"] or
                previous["actors"][receiver]["raw"]["control_mode"] not in (10, 14) or
                row["actors"][receiver]["raw"]["control_mode"] != 8 or
                contact["player_count"] <= 0 or
                contact["player_routine"] not in (0x86BFBA, 0x86C91E) or
                receiver not in (contact["player_a"], contact["player_b"])):
            return False
        # C476/C48F clears 0946 only; it does not call A613 or clear09C4.
        self.receiver_cancel = key
        self.receiver_clears += 1
        return True


def pass_interruption_guard_self_test():
    """Reject partial lookalikes for the release-before-knockdown ordering."""
    from copy import deepcopy

    actor = lambda mode, released, band=12, family=5: {
        "animation": 0x35 if mode == 8 else 0x2F,
        "raw": {"control_mode": mode, "pass_released": released,
                "pass_band_62": band, "pass_family_c0": family}}
    previous = {
        "simulation_tick": 100,
        "possession": {"pass_actor_raw": 1, "pass_receiver_raw": 4,
                       "pass_active_raw": 1, "pass_distance_raw": 4,
                       "actor": 1},
        "actors": [actor(1, 0), actor(15, 0)],
        "ball": {"owner": 1, "state": 4}}
    row = {
        "simulation_tick": 101,
        "possession": {"pass_actor_raw": 1, "pass_receiver_raw": 4,
                       "pass_active_raw": 0, "pass_distance_raw": 4,
                       "actor": -1},
        "collision": {"player_count": 1, "player_routine": 0x86BFBA,
                      "player_a": 1, "player_b": 6},
        "actors": [actor(1, 0), actor(8, 1)],
        "ball": {"owner": -1, "state": 3},
        "match": {"live_state_raw": 0}}
    positive = PassInterruptionGuard()
    positive.observe(previous, row)
    if positive.release_before_knockdowns != 1:
        raise AssertionError("release-before-knockdown witness was not counted")

    mutations = (
        ("pass actor", lambda old, new: new["possession"].__setitem__(
            "pass_actor_raw", 2)),
        ("pass receiver", lambda old, new: new["possession"].__setitem__(
            "pass_receiver_raw", 3)),
        ("active latch", lambda old, new: new["possession"].__setitem__(
            "pass_active_raw", 1)),
        ("release latch", lambda old, new: new["actors"][1]["raw"].__setitem__(
            "pass_released", 0)),
        ("old owner", lambda old, new: old["ball"].__setitem__("owner", 2)),
        ("old ball state", lambda old, new: old["ball"].__setitem__("state", 3)),
        ("new owner", lambda old, new: new["ball"].__setitem__("owner", 0)),
        ("ball state", lambda old, new: new["ball"].__setitem__("state", 4)),
        ("pass band", lambda old, new: new["actors"][1]["raw"].__setitem__(
            "pass_band_62", 18)),
        ("pass family", lambda old, new: new["actors"][1]["raw"].__setitem__(
            "pass_family_c0", 4)))
    for name, mutate in mutations:
        old = deepcopy(previous)
        new = deepcopy(row)
        mutate(old, new)
        try:
            PassInterruptionGuard().observe(old, new)
        except AssertionError:
            continue
        raise AssertionError(
            f"release-before-knockdown guard accepted changed {name}")
