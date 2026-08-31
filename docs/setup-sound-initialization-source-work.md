# Normal sound initialization source slice and resident upload provenance

This component produces the initial sound-driver clear and channel-off prefix
from source at `$80:9B73`. It stops before the first unresolved `$80:AACD`
SPC idle-port read. It does not load a captured channel table into production,
complete initialization using assumed acknowledgements, or predict whole-game
phase. All files are new; prior source/capture freezes remain unchanged.

## Reset and normal callers

Reset `$80:800D..8021` enters native CPU mode, establishes DP zero and DB `$80`.
`$8038 LDA #$C683; $803B STA $0C; $803D STZ $0E` supplies the long ROM pointer
`$00:C683` to `$803F JSL $80AB06`. The source uploader waits for the IPL's
combined `$BBAA` port word at `$AB20`, then uses its `$CC` startup handshake,
length/destination descriptors, incrementing byte tokens and a final zero-length
entry descriptor. None of that upload timing is implemented by this slice.

The source stream is exact and uncompressed:

| Source | Meaning | Destination |
| --- | --- | --- |
| ROM `$00:C683..C686` | length `$04F0`, destination `$0380` | descriptor |
| ROM `$00:C687..CB76` | 1,264-byte resident SPC program/data | ARAM `$0380..086F` |
| ROM `$00:CB77..CB7A` | length zero, execution address `$0380` | IPL jump entry |

The full 1,272-byte descriptor stream SHA256 is
`63abcf0382deef058ede935b135222bea146b62bf0d77e529aeb3df5195c9f16`.
The payload SHA256 is
`0559044860666dc3bae509c93a74134d09bf8ccbece26d774876afeec8923fd4`.
The new natural capture dumps ARAM at `$80:AB7D`, before the uploader's RTL;
all 1,264 bytes in the destination span equal the source payload. This proves
ROM-to-ARAM provenance, not instruction timing or a usable snapshot initializer.

The five normal `$9B73` callers in this journey are:

1. Reset `$80:814A`, after the uploader and other reset work. `$8130/$8132`
   previously clear `$31..$34`, so this call correctly initializes `$5A/$5B`
   to zero rather than the later Setup bank.
2. `$82:AD48`. Its preceding `$AD34..AD3C` instructions derive `$33=$0082`
   and `$31=$96A3`; `$AD24` calls `$80:9763` before that setup. `$9763` issues
   resident command `$01`, requiring live SPC acknowledgement.
3. Three calls at `$82:ABF2`, reached from `$82:ABE0` with a nonzero X selector.
   `$ABE7 DEC $53` establishes an outer guard; `$ABEE` calls `$9763` and then
   `$ABF2` initializes the CPU tables. A zero selector skips these calls.

`$82:ABF6` and subsequent source work consume a resource list through `$64`;
`$ABCE` selects descriptors from `$82:AA2F` and calls `$80:9829`. Those later
sample/resource uploads and sequence startup are unresolved dependencies,
not covered by the resident-code identity above.

The native caller records attest each actual JSL, its unchanged A/X/Y/DB/P,
three-byte stack effect, eight CPU cycles and 54 intrinsic master clocks.
Caller work is outside the new `$9B73` component.

## Implemented initialization slice

`nba_setup_sound_init` requires native mode, decimal clear, DP zero and M=0/X=0
on `$9B73` entry. Its bus and registers are supplied by the live caller. The
entry DB must address a WRAM/IO mirror, or be one of the source's `$7E/$7F`
normalization cases. The initial PHP/PLP pair preserves the caller's width;
the source guard changes those two special DB values to PB `$80`.

The continuation executes, with original bus order and instruction work:

- `$9B8A DEC $53`, followed by the **word** copy `$33/$34` to `$5A/$5B` using
  LDX/STX. It does not discard the copied high byte or hardcode bank `$82`.
- The descending byte clear `$9B90..9B97`, covering exactly `$062A..$07ED`.
- Three `$FF` event sentinels at `$07CD..$07CF`, `$062F=$7F`, `$062E=0`, and
  `$062D=$80`. The accumulator's high byte remains source caller state under M=1.
- `$9BB2 JSL $9BD5`, its bank/status stack work and second `$53` decrement.
- The first channel-off call with Y=7. `$A4FC..A505` redundantly writes zero
  to `$073A+7` and `$FF` to `$0742+7`, then jumps to `$AACD`.
- The three fetch cycles of `$AACD LDA $2140`, leaving its data read unresolved.

At that boundary, `peek` exposes the pending `$80:2140` read and `accept`
refuses every supplied response without changing state. The pending read's
`instruction_end` marks where the instruction **will** complete, not a cycle
that has completed already. A scheduler must not charge that cycle or take an
instruction-boundary interrupt prematurely.

The two decrements are native byte arithmetic, not a boolean lock. Starting
from `$53=0`, the first two captured calls end at `$FE`; with the outer music
guard already at `$FF`, the final three calls end at `$FD`. The source's later
increments remain beyond this first pending-read boundary. Preserve this
underflow/nesting, the descending clear, redundant channel write, accumulator
high byte, and DB normalization. None is normalized as a bug fix.

The build-time ROM translator emits 62 static source states, with no trace
input. Running C contains compiled source labels and bus continuations, not
an opcode decoder or recorded instruction schedule. The natural capture
witnesses 58 distinct states; synthetic DB `$7E/$7F` cases exercise the
normalization branches outside the normal captured route.

## Next SPC dependency, mapped from the uploaded source bytes

The uploaded resident code begins at SPC `$0380`. Its source initializes
RAM/driver state, DSP control and timer control, including writes of `$10`
to timer target `$FA` and `$01` to control `$F1`, before entering its service
loop. Those initialization instructions and hardware effects must eventually
execute from the actual upload/reset state, not from the captured ARAM dump.

The following addresses map directly into the verified ROM payload:

| SPC source | Source behavior relevant to the continuation |
| --- | --- |
| `$0441` | Writes A to output latch `$F4` for acknowledgement. |
| `$0443..0445` | Reads the separate CPU input latch `$F4` until the CPU clears it. |
| `$0447` | Writes zero to the SPC-to-CPU output latch, advertising idle. |
| `$044A` | Calls timer/voice service `$048B` before checking new commands. |
| `$044D..0455` | Reads input `$F4`, loops on zero, executes two NOPs and a repeated-input stability check. |
| `$0456..045A` | Doubles the command into X and jumps through the table at `$0461+X`. |
| command `$05` → `$0613` | Immediately writes acknowledgement `$05` to output `$F4`, then performs channel DSP/state changes and returns to `$0443`. |
| command `$0B` → `$06AD` | Immediately acknowledges `$0B`, then writes the selected voice's pitch bytes from `$F6/$F7` and returns to `$0443`. |

The CPU helper `$AACD` first waits for idle zero. Source after this component
then writes Y to `$2141`, issues command `$05`, waits for its echo and clears
the CPU command port. All eight channel-off iterations and later initializer
volume/state calls remain to be implemented. The already-derived `$A137`
prefix later reaches command `$0B` through `$AAE3`; it needs the same carried
consumer state.

The resident command acknowledgement occurs **before** its later DSP work.
The separate input-clear wait, idle publication, timer/voice service and
input stability check all retain source order. A fixed acknowledgement delay
or an adapter that immediately echoes commands would omit this work.

The existing `nba_spc.c` still discards writes to output latches `$F4..$F7`;
this defect remains documented and unchanged. A future source-specific
consumer/adapter needs separate port input/output latches, uploaded ARAM,
SPC registers/stack/PSW, timer state and execution phase, relevant DSP state,
and normal elapsed execution through menu dwell. The current snapshot/DSP-event
playback path is not accepted as that state. No general SPC emulator or
fitted acknowledgement schedule was added here.

## Validation and limitations

`.analysis/native-sound-init-v1` is a new uninterrupted reset/controller-only
run. It preserves all seven scheduler/interrupt/sound-prefix JSON files from
the prior sound-prefix capture byte-for-byte. It records five initializer
entry/exit WRAM pairs and caller records, 7,055 instruction states, 2,455
data-bus observations and one reset upload entry/exit pair with an ARAM dump.
No NMI interrupts these five observed slices. The verifier explicitly rejects
an interrupted witness rather than silently subtracting unvalidated work.

Explicitly labeled isolated component differentials may seed a private test
bus from each captured entry WRAM/register state. They supply no captured
clock, port value or acknowledgement. Across all five calls, C matches all
7,055 instruction/register states and CPU positions, 2,450 accepted data
access values/positions, and all complete 128KiB WRAM endpoints. The five
native port reads are excluded from C input because they remain unresolved.
Each observed source slice produces 1,411 instruction entries, 4,703 accepted
CPU cycles and 29,198 intrinsic master clocks, excluding the final port read.
These are computed checksums, not implementation delays or timing tables.

Per-instruction native master intervals conserve intrinsic work plus observed
40-clock refresh quanta. This differential does not predict future refresh,
DMA, NMI or SPC phase. The API allows a future owner to suspend only after
completed instructions; interrupted initialization and the full normal caller
chain remain separate validation work.

Eleven Python integrity/protocol tests cover required capture/build/source
identities, settings, numeric domains, strict numeric tokens, ROM-to-ARAM
payload identity and upload source/cursor identity. C cases prove the clear
bounds from poisoned memory, derived bank/sentinels, guard underflow, four DB
paths, immutable pending read, instruction limit and invalid entry rejection.
Ten malformed parsed trace views reject. Changing only the observed pending
SPC value to `$FF` leaves the C trace and WRAM identical, proving that response
is not supplied to the continuation.

| Evidence | SHA256 |
| --- | --- |
| New native manifest | `6fc9cac155f6f7e3998d0cd12ad1f136084ca1fb842326b2bc81e03f97d6c142` |
| Init instructions | `788385afff141f558176190a83888fa22951c690ead5853882ff424995266b1e` |
| Init data bus | `cdcf2fe932b255db3e9ce3eb9d1a35f441e37a27230e41e45304d083077e00f7` |
| Init boundaries | `89237f4289d56795137849838f1269400d338e544b2e177ccb0f30264e9b4dc3` |
| Natural post-upload ARAM dump | `cd2e713d4e19e0802c0462f62c57af1feee35b4232315b8ed5bcdb200e0128ed` |
| ROM `$80:8036..8042` | `4f375de50a903b43fd55d1a79f077e607168c58341f10d1a4f87b8f783b51a3c` |
| ROM `$80:9B73..9BF3` | `153840ab70cec2c6ec6699b6737ec8b70c505a3c794a7fc3ff40b4bde06dc0f8` |
| ROM `$80:A4FC..A50A` | `f1feedab71a96bba43b011688399890d6b06e2b6c11baaeb032ee683806913e7` |
| ROM `$80:AACD..AAE2` | `b6accfb6a1fd5ec04d34a0cb09af2f53dfde105ca48e8f4b235c1cec909ee9e8` |
| ROM `$80:AB06..AB7D` | `3324d601c5efcec3c5b9cb99d1bc8e6365584d5736f4cb11c00efb94e9875213` |

Use fresh private output directories from the scheduler worktree:

```powershell
python tools/generate_setup_sound_init.py --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --decoder-root 'C:/Users/joshs/Projects/tools/snesrecomp-source-v0.2.0-alpha/recompiler' --output src/nba_setup_sound_init_program.inc --check
.\tools\build_setup_sound_init_probe.ps1 -OutputDirectory .analysis/sound-init-build-new
python tools/verify_setup_sound_init.py --native .analysis/native-sound-init-v1 --previous-native .analysis/native-sound-prefix-v1 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe .analysis/sound-init-build-new/setup_sound_init_probe.exe --output .analysis/sound-init-proof-new
python tools/test_setup_sound_init.py --native .analysis/native-sound-init-v1 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe .analysis/sound-init-build-new/setup_sound_init_probe.exe
```

Independent audit is required. There is no production wiring, configuration
change, source acknowledgement implementation or Rules reentry parity claim.
