# Uploaded SPC resident poll and first command acknowledgement

`nba_setup_spc_resident` executes 24 specific instructions from the NBA Live
95 resident program. It supplies two bounded prerequisites for the sound
initializer and later NMI sound driver. It does not execute the SPC IPL,
resident reset initializer, timer service or DSP. There is no production
wiring or normal-state/phase claim.

The verified source stream remains ROM `$00:C683`, descriptor length `$04F0`,
destination `$0380`; payload `$00:C687..CB76` maps byte-for-byte to ARAM
`$0380..086F`. Its SHA256 is
`0559044860666dc3bae509c93a74134d09bf8ccbece26d774876afeec8923fd4`.
The terminating descriptor at `$00:CB77` selects entry `$0380`. The earlier
sound initializer document describes the CPU uploader `$80:AB06` and callers.
The new cold-reset capture independently matches that same 1,264-byte payload
at resident entry, first poll entry, and after eight channel-off commands.

## Implemented boundaries and bus contract

The owner supplies live A/X/Y/SP/PS, uploaded ARAM, separate directional port
latches and the DSP address latch. The bounded entry PCs are `$0441`, `$0443`,
`$0447`, `$044D`, and `$0613`, with PS.P=0, normal internal/external SPC speed
and ARAM writes enabled. No entry function initializes these from a snapshot.
The native prestates used below are expressly isolated component tests.

| Entry/work | Exact stopping point |
| --- | --- |
| `$0441` acknowledgement, `$0443` input-clear wait, `$0447` idle publication | `$044A CALL $048B` pushes its return address; `$048B MOV A,$FD` remains pending before the timer data read. |
| `$044D` after external timer-service return | Input read, zero branch, two NOPs, repeated-input check, command-table lookup and command `$05` execute; `$0622 MOV $F3,A` remains pending before its **read** of DSP data. |
| `$0613` command `$05` directly | Publishes `$05` to output `$F4`, reads channel input `$F5`, computes its DSP address and writes `$F2`, then reaches the same pending DSP read. |

The timer read and DSP read are actual unresolved hardware effects. `accept`
refuses them without charging a cycle or changing state. Their exposed
`instruction_end` describes where a future accepted cycle would complete,
not a completion already reached. The two source bytes fetched before either
read are accepted work. The DSP write itself has not happened.

Each accepted request represents one intrinsic SPC machine cycle. Work exposes
the source PC, opcode/operand fetches, data reads/writes, idles, and instruction
completion. C contains an explicit source-PC switch and the 24 compiled source
byte strings; it has no runtime opcode decoder, recorded instruction schedule,
or branch driven by a native trace. The verifier checks every compiled byte
against the ROM. Native `spc.cycle` advances by two units per accepted machine
cycle in this witness. The implementation does not convert those cycles to
65816 or master-clock timestamps.

The bus holds `cpu_to_spc[4]` and `spc_to_cpu[4]` separately. SPC reads of
`$F4..F7` sample input latches, while writes update output latches and underlying
ARAM. Publishing an already-visible input never overwrites output or ARAM.
This preserves the hardware distinction missing from existing `nba_spc.c`,
whose output-register writes are discarded. That file is unchanged.

`visible_input` is deliberately not a 65816 port-write adapter. Its caller must
derive when a CPU write becomes visible across the CPU/SPC clocks. The local
Mesen reference distinguishes pending CPU writes from currently visible input
latches, with clock-phase-dependent visibility. That adapter and its normal
phase remain unresolved here; neither immediate visibility nor a fixed delay
has been substituted.

The source quirks remain explicit:

- `$0613` acknowledges before the subsequent DSP read/write or channel-body
  work. An adapter must not delay acknowledgement until that body completes.
- Stores to the port/DSP registers include the source read-before-write bus
  access. `$0613 MOV $F4,#$05` reads the **input** port before updating output.
- `$0453 CBNE $F4` samples the command again before fetching its relative
  operand, leaves PS unchanged, and returns through timer service on mismatch.
  The two preceding NOPs and the input-clear wait are retained.
- `$0456 ASL A` wraps in eight bits. A synthetic `$85` input reaches the same
  table entry as `$05`, with the source carry behavior preserved. No command
  bounds check was added.
- `$0619 XCN A` uses the entire channel byte. No channel mask/range check was
  inserted; synthetic `$FF` reaches DSP address `$06` through native arithmetic.

## How normal resident initialization reaches this poll

This ownership map is source analysis, not implemented initialization. The
fresh resident-entry state still contains the IPL/upload result: output
`$F4=$F3`, output `$F5=$BB`, SP `$EF`, and CPU inputs `$F6/$F7=$80/$03`.
Those values are observations, not allowed production seeds. They must emerge
from reset/IPL and the real CPU uploader.

1. `$0380 CLRP`, `$0381 MOV X,#$FF`, `$0383 MOV SP,X` establish resident direct
   page and stack. `$0384 MOV $F1,#$30` clears all four CPU input latches and
   disables timers/IPL mapping. It does not clear the output acknowledgement.
2. `$0387..03A2` set A=0 and clear `$0000..007B`, `$0100..01FF`, `$0200..02FF`
   and `$0300..037F` using the original descending/wrapping loops. The source
   leaves `$007C..00FF` outside that ordinary RAM clear; hardware writes have
   their own effects.
3. `$03A3..03D3` derive the free-RAM pointer `$0870` and subtract it from `$FFFF`
   using the source carry rules. The first loop has X=`$8F` and writes
   `$0870..08FE`. It does **not** write `$08FF`. The following full-page loop
   clears `$0900..FFFF`. Preserve this one-byte omission when implementing the
   initializer. The all-zero native power-on fixture cannot dynamically prove
   the omitted byte; this finding follows from source loop bounds, and is not
   permission to normalize the clear to a `memset` through `$FFFF`.
4. `$03D4..03EF` write driver byte `$04=$20`, DSP register `$6C=$20`, DSP
   `$0C/$1C=0`, timer target `$FA=$10`, control `$F1=$01`, and driver byte
   `$05=0`. Timer enable must retain the real divider/output/reset semantics.
5. `$03F2..0436` set sample pointer `$06/$07=$0870` and table `$0200/$0201`
   accordingly. They write DSP `$5D=2`, `$5C=$FF`, `$2C/$3C/$0D=0`,
   `$6D/$7D=1`, and `$2D/$3D/$4D=0`, in source order.
6. `$0436..043B` clear SPC output `$F5..F7`; `$043D JMP $0447` finally reaches
   the idle-port publication. The upload acknowledgement in output `$F4`
   survives until this point. The CPU's initializer waits on this real work.

The next timer path is `$048B MOV A,$FD`. A zero value leads through `$048D`
and `$048F RET`; a nonzero value enters voice service at `$0490`, including
channel state, envelope work and DSP operations through `$0577`. The current
eight-command witness happens to read zero in all eight instances. No zero
value is supplied to the new C continuation, and no conclusion about later
timer reads follows from this fixture.

After the pending `$0622` DSP read/write, command `$05` writes zero to
`$10+X`, selects the next two lower DSP addresses through Y decrements, writes
their data, and returns to the input-clear wait at `$0443`. Later commands,
sample uploads, timer-driven voice state, CPU sequence state and menu dwell
must all execute before this can produce a normal sound state over time.

## Original observations and differential limits

`.analysis/native-spc-resident-v3` starts from the unmodified ROM and cold reset
with isolated all-zero power-on settings. No input, RAM, register, port or state
writes are performed by Lua. It stops after eight source command `$05` bodies
return to `$0443`, recording 270 instruction states and 1,333 mixed SPC/DSP
bus observations. Resident entry/poll/end each include ARAM and exposed SPC
state. The first 120 resident initialization instruction states are recorded
as ownership observations, not a complete initializer trace.

Failed captures v1/v2 are retained unchanged. They timed out while the observer
recorded every CPU busy-port read before the long SPC clear finished. v3 records
CPU writes and bounds CPU-read logging after poll entry; it completed normally.
The capture does not speed up emulated source, edit memory, or inject a port.

Mesen's Lua SPC read callback includes DSP RAM reads. The verifier therefore
attributes only port accesses, source CALL stack writes, and uploaded source/
table reads to this C component. It does not present the other DSP bus rows as
SPC instruction work. All raw rows remain available, with checked chronology,
clock bounds and instruction-PC association.

Sixteen isolated slices use native entry registers/latches and private test
ARAM. Native writes prior to each slice reconstruct only its component prestate.
Input latches are held at those entry values during the slice; every actual
port read consumed by the C work is compared against native. Later native
CPU input changes outside consumed reads are not claimed as C output. All
output latch effects and the selected DSP-address latch are checked separately.
There is no captured timer/DSP response or timestamp fed into C.

Of the 24 compiled states, 23 occur in the native slices; the generic
acknowledgement entry at `$0441` is exercised synthetically. Fresh C matches
182 instruction/register states, 175 attributable data accesses
and their intrinsic cycle positions. Its endpoints match source registers,
output latches and exact source-owned ARAM writes; no whole-ARAM native endpoint
equivalence is claimed across concurrent DSP work. Poll entry `$0447` requires
15 accepted cycles to the pending timer; a cleared `$0443` entry requires 20.
The observed `$044D` command `$05` path requires 53 accepted cycles to the
pending DSP read. These are computed checksums, not scheduling constants.

Twenty-two Python cases include eight malformed native/C trace views, five identity/
schema omissions or corruptions, synthetic zero/busy/wrapped inputs, short
binary protocol rejection and 15 C latch/boundary contracts. Changing only
native timer-read values or DSP-read values to `$FF` produces identical C
traces/endpoints. This demonstrates their exclusion, not response correctness.

The hardware-cycle reference is Mesen source commit
`b9fa69ddc6d0a331fb103fdb5eef6904305703c2`; pinned files and hashes are retained in
`.analysis/spc-resident-reference-v1`. The reference describes read-before-write,
stack/bus cycles and latch semantics; it is not claimed to be the exact source
revision of the installed binary. The fresh installed-binary observation is
the differential timing evidence. See the pinned upstream
[SPC implementation](https://github.com/SourMesen/Mesen2/blob/b9fa69ddc6d0a331fb103fdb5eef6904305703c2/Core/SNES/Spc.cpp) and
[instruction implementation](https://github.com/SourMesen/Mesen2/blob/b9fa69ddc6d0a331fb103fdb5eef6904305703c2/Core/SNES/Spc.Instructions.cpp).

Native manifest SHA256:
`d351968b296dcbf39b2600edf7a7d48f04076032dcbb9eaede10429bf095f24c`.

Use new private build/output directories:

```powershell
.\tools\build_setup_spc_resident_probe.ps1 -OutputDirectory .analysis/spc-resident-build-new
python tools/verify_setup_spc_resident.py --native .analysis/native-spc-resident-v3 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe .analysis/spc-resident-build-new/setup_spc_resident_probe.exe --output .analysis/spc-resident-proof-new
python tools/test_setup_spc_resident.py --verifier tools/verify_setup_spc_resident.py --native .analysis/native-spc-resident-v3 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe .analysis/spc-resident-build-new/setup_spc_resident_probe.exe --output .analysis/spc-resident-tests-new
```

Independent audit is required. No prior source/freeze, `nba_spc.c`, `nba_audio.c`
or production manifest is changed. Whole initialization, CPU/SPC visibility,
timer/DSP execution, carried phase and Rules reentry remain unresolved.
