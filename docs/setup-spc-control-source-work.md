# SPC F1 control owner at the write commit boundary

`nba_setup_spc_control_commit` implements the real `$F1` register effects needed
by resident source `$0384 MOV $F1,#$30` and later `$03EC MOV $F1,#$01`. Its input
is explicit carried hardware state **after the write's bus cycle has advanced**.
It charges no cycle, predicts no response and creates no initial state.

The local hardware source `Spc::Write` advances the SPC cycle, timers and DSP
before invoking the memory-write callback. The callback precedes both underlying
ARAM storage and the register effects. The fresh capture uses that callback for
the pre-effect snapshot and the next SPC instruction callback for the post-effect
snapshot. Both snapshots have the same SPC clock in each observed publication.
That isolates the commit without pretending to implement the preceding cycle.

## Carried state and exact effects

The shared resident bus holds visible CPU inputs, separate SPC outputs, ARAM
and the DSP address latch. The new hardware state adds four staged CPU input
bytes, the pending-input-update flag, IPL visibility, ARAM write enable and all
three timers' enable/global-enable, prescaler, edge-history, counter, raw output
and target fields. There is no constructor that seeds any of them.

The commit follows the hardware source order:

1. If ARAM writes are enabled, store the value to underlying ARAM `$00F1`.
   The register's effects still occur when this ARAM write is disabled.
2. Bit `$10` clears visible **and staged** CPU input ports 0/1. Bit `$20` does
   the same for ports 2/3. Neither operation changes SPC output latches or the
   underlying ARAM bytes at `$F4..F7`.
3. Set each timer enable from bits 0/1/2. A disabled-to-enabled edge resets only
   `stage2` and raw `output`. It does not reset `stage0`, `stage1`, previous-edge
   history, target, or global enable. This edge reset occurs even when global
   timer gating is off. Repeated enable and falling enable preserve counter/output.
4. Bit `$80` selects IPL-ROM read visibility. Reads at `$FFC0..FFFF` use IPL
   when enabled; underlying ARAM remains separate and unchanged by this mapping.

Bits `$08/$40` have no register effect here, although the entire byte is stored
to ARAM when writes are enabled. The hardware source leaves the pending CPU
input-update flag set even if a clear erased all staged bytes. That behavior
is preserved and commented; the implementation does not normalize the flag.

The timer output is an eight-bit internal field. Reads elsewhere expose its
low nibble and clear it; the F1 owner neither masks it nor performs a read.
The retained hardware source distinguishes power-on output `$F` from the reset
routine's output zero. The native first F1 write has output `$F` in each timer,
which remains `$F` when the write only disables timers. No blanket timer-reset
or assumed-zero initialization was introduced.

## Native validation and hidden-state limit

`.analysis/native-spc-control-v1` runs the unchanged ROM from cold reset using
isolated settings and read-only Lua observations. It records two before/after
SPC state and full ARAM pairs:

| Source | Value | Same SPC clock | Observed effect |
| --- | --- | --- | --- |
| `$0384` | `$30` | 68,348 | CPU input ports clear and IPL visibility turns off; SPC output `$F4=$F3` survives. |
| `$03EC` | `$01` | 1,738,896 | Timer 0 enables and its output changes `$F`→0; its prescaler phase remains 16. |

These clocks identify the retained observations; they are never supplied to C
or used as delay constants. The verifier checks the source `MOV` bytes in the
original ROM, full uploaded resident identity, exact same-clock boundaries,
all 70 modeled visible hardware fields, both full 64KiB ARAM endpoints and that
all other exposed SPC/DSP state is unchanged across each commit.

Lua's `emu.getState` map does **not** expose the hardware source's staged
`NewCpuRegs` array or pending-update flag. The source serializes these only in
its non-map state representation. Accordingly, those five fields in the test
input use deliberately distinct synthetic sentinels. Their clear/preserve
contracts are source-derived tests, not native observations or normal seeds.
A future normal owner must carry their real values from CPU writes and their
clock-phase-dependent visibility updates. It must not initialize them from
visible input ports or these sentinels.

Thirty-two local tests cover nonzero timer phases/counters/output, rising and
held/falling enable, global gating, selected port pairs, staged versus visible
inputs, pending-flag preservation, output isolation, ARAM write gating, ignored
bits and IPL boundaries. They also reject corrupted native/C fields, missing
identities/settings, non-integer process statuses, invalid boolean/timer domains,
and short/long binary inputs. The C probe checks null-owner rejection and IPL
window boundaries. All native/C mutation tests leave original artifacts untouched.

The retained local hardware source is Mesen commit
`b9fa69ddc6d0a331fb103fdb5eef6904305703c2`, specifically `Spc.cpp` control/port/
visibility handling, `SpcTimer.h::SetEnabled`, and `SpcTypes.h` fields. These
source files and their hashes are included directly in the freeze. The source
revision is not claimed to identify the installed binary; the fresh native
observations independently validate the exposed effects at this boundary.

## Integration boundary and remaining work

The frozen SPC initializer stops before the `$0384` write. A future owner must
first advance that one real SPC bus cycle, including carried timer/DSP work and
CPU input visibility, then call this commit with the pending `$30` value. Only
after it completes may source continue at `$0387` with the original registers.
This task adds no production wiring or workaround that jumps over the cycle.

Clock progression, timer output reads, DSP execution, actual CPU/SPC visibility,
IPL/reset execution and normal upload-state construction remain separate
dependencies. This commit is an independently usable effect owner, not a full
hardware scheduler or normal sound-state predictor. The existing discarded
output-latch behavior in production `nba_spc.c` remains documented and untouched.

Reproduce in fresh private directories:

```powershell
.\tools\build_setup_spc_control_probe.ps1 -OutputDirectory .analysis/spc-control-build-new
python tools/verify_setup_spc_control.py --native .analysis/native-spc-control-v1 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe .analysis/spc-control-build-new/setup_spc_control_probe.exe --output .analysis/spc-control-proof-new
python tools/test_setup_spc_control.py --verifier tools/verify_setup_spc_control.py --native .analysis/native-spc-control-v1 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe .analysis/spc-control-build-new/setup_spc_control_probe.exe --output .analysis/spc-control-tests-new
```

Independent audit is required. All previous freezes and timed-out observations
remain immutable. Production audio, manifests, root files and commits are untouched.
