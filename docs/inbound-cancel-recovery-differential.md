# Canceled inbound transfer recovery

The 63,800-frame `tip_flow_endurance_probe` still requires a period advance,
at least 600 post-restart live frames, and no dead-ball run longer than 2,400
frames. Natural receiver cancellation is reported, but is not required to
happen accidentally in that finite trajectory. A mandatory deterministic
production recovery test runs before endurance.

## Native evidence

`tests/fixtures/inbound-cancel-recovery.json` losslessly embeds four complete
Mesen entry/exit records (WRAM `0000-4AFF`, CPU registers, executed PCs).
These are controlled genuine Exhibition calls, not unmodified gameplay
witnesses. At already-arrived `$86:F43A` entries, the capture changes only
`$09B8` and `$0946`, records through `$86:F58F`, then restores those two
controlled words. It never writes ROM, CPU registers, or the stack.

| Case/frame | Incoming `$09B8/$0946` | Native outgoing `$09B8` |
| --- | --- | --- |
| 1 / 4805 | `0001/FFFF` | `0000` |
| 2 / 4807 | `0001/0000` | `0001` |
| 3 / 4809 | `A5A5/FFFF` | `0000` |
| 4 / 4812 | `0000/FFFF` | `0000` |

The negative-receiver cases execute `$86:F57F`; the valid-receiver control
does not. The unrounded actor coordinates are `386.03125,-55.125`, target
`394,-64`: native integer deltas are `+8,-8`. Fractional bytes are retained.
Case 3 includes an asynchronous NMI timer write `294->293`; it remains in
the raw fixture, but is not attributed to the synchronous arrival routine.

ROM-file SHA-256:
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
Raw capture SHA-256:
`c938c230cf6a0d86054e030f99d66c9e0a72ea317a2c847254fd1c15837255a2`.

## Production binding and scope

The generated C header supplies unchanged captured inputs to
`nba_tipoff_replay_inbound_continuation`, which calls the production
`cpu_update_rom_inbound`. The compared ten fields project the native
`F54F-F587` arrival/ready/receiver gate. The earlier F43A steering prefix,
DP/stack writes, human-direction context storage, and asynchronous NMI are
not claimed as a complete-function comparison. Actor coordinates, RNG,
timer, raw receiver, selector, and ownership also have preservation guards.
The actual captured human-controller word is retained; no CPU label is
substituted to manufacture the expected result.

A separate whole-update binding starts from case 1's relevant native state,
with ordinary initialized teammates and explicit host scheduler/ATTACHED
cache sentinels. It verifies that `nba_tipoff_update` reaches recovery even
when `receiver_actor` still names actor 4 but raw `$0946` is negative.
This is a deterministic host integration check, not native whole-frame
equivalence. It does not claim to reproduce the preceding contact/A613
event that canceled the receiver.

## Reproduce

Run `tools/capture_inbound_cancel_recovery.ps1 -OutputDir <new-directory>`
to capture from Mesen. Then normalize with
`tools/normalize_inbound_cancel_recovery.py --capture-dir <directory>
--fixture tests/fixtures/inbound-cancel-recovery.json
--header tests/fixtures/inbound-cancel-recovery.h`.

After building the port and `tip_flow_endurance_probe`, run:

```powershell
python tools/verify_inbound_cancel_recovery.py --fixture tests/fixtures/inbound-cancel-recovery.json --header tests/fixtures/inbound-cancel-recovery.h --probe build/tip_flow_endurance_probe.exe --assets build/nba95_assets.pak --rom "F:\Games\SNES\NBA Live 95 (USA).sfc"
build/tip_flow_endurance_probe.exe build/nba95_assets.pak
```

`--recovery-only` runs the deterministic bindings without the long journey.
