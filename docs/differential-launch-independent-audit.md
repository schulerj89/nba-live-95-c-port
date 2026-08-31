# Independent differential launch audit, 2026-08-30

**PASS for private native-launch isolation; FAIL for gameplay trajectory
equivalence.** The gameplay auditor read `tools/mesen_portable.py`, the exact
diffs in `run_differential.py` and `mesen_differential_capture.lua`, and the
fresh native capture and failure report directly.

The launcher copies Mesen into a new private directory, writes portable JSON
settings beside that executable, begins with an empty private save directory,
and explicitly selects controller 1, RAM initialization, frame rendering and
video settings. This matches the exact installed Mesen home selection code
independently inspected during the transition audit. It avoids the ineffective
string command-line save-path option. It does not alter the user's emulator
configuration or saved SRAM.

`verify` checks the unchanged initial settings hash and each required
persisted setting with strict scalar types. The Lua script reports its actual
`emu.getScriptDataFolder()` location; it must resolve inside the private
runtime directory. Final SRAM and persisted settings hashes are retained.
The auditor reran verification on
`.analysis/differential-ownership-20260830-portable` and confirmed the actual
Lua directory resolves under that capture's `portable-mesen/LuaScriptData`.
All three helper tests passed, including deliberate critical-setting drift,
missing/wrong home attestation and initial-settings corruption.

The inspected launch change adds no gameplay state import and changes no
expected values, compared fields or comparison tolerances. The older
disclosed pregame team/Exhibition/CPU-neutral selection injection remains in
the differential driver. This is therefore a controlled launch followed by
natural execution, not an entirely natural menu journey. The separate
controller-ownership captures use actual menu inputs without those writes.

The fresh report still returns `INITIAL_STATE_MISMATCH` at baseline, with
449 projected fields and zero matching checkpoints. Directly inspected
examples are native/C RNG `$0000/$9146`, clock `43200/10800`, shot clock
`1440/0`, and play `1/0`. Fresh SRAM defaults and the selected native boundary
still differ from the C initializer. Isolating Mesen makes this failure
reproducible; it does not justify changing the expected state to make it pass.
The 449-word projection is partial and excludes other gameplay, rendering and
audio state. No whole-game equivalence or completion claim is accepted.
