"""C runtime assertions for source-classified interrupted passes, not ROM parity."""


class PassInterruptionGuard:
    def __init__(self):
        self.interrupted = {}
        self.receiver_cancel = None
        self.entries = self.recoveries = self.receiver_clears = 0

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
