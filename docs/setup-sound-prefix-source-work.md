# Bounded sound-driver prefix and unresolved SPC response

This component derives the CPU source prefix at `$80:A137`. It is not wired
to production, does not initialize normal audio state, and does not predict
Rules reentry phase. It adds new files only. Every earlier queue, codec,
producer and header freeze remains unchanged.

The first useful hardware boundary depends on carried source state. In the
immutable 46-NMI backdrop trace, 44 calls first read `$2140` at `$80:AAE6`.
Two calls first reach `$80:A9E5` after substantially more sequencer work.
This component covers the former prefix and explicitly stops the latter
at `$80:A2CE`, before executing that sequencer. It does not substitute the
shorter route's work for either longer route.

## Source contract

The native caller is `$80:857A JSR $A137`, returning eventually to `$857D`.
The caller's six CPU cycles and two stack writes are outside this component.
Its entry contract is native mode, decimal clear, DP zero, DB `$80`, M=0,
X=1. A/X/Y/SP and flags are live caller state; high bytes of X/Y must already
be zero for X=1. `$A137 SEP #$30` then sets both width flags. The `$9FA1`
helper saves that M=1/X=1 status with PHP and restores it at `$A0B2`.

The new source-specific coroutine uses the existing bus-cycle representation
but no old implementation file changes. A build-time canonical-ROM decoder
generates 73 static source states. The running C has no opcode decoder,
runtime ROM execution, captured instruction schedule, or fitted delay. Each
accepted event is one ordered source bus or idle cycle with its intrinsic
master-clock cost. Refresh, DMA, interrupt and SPC scheduling remain external.
Sixty-five distinct static states have natural prefix witnesses; eight other
static states are explicitly unwitnessed by this journey.

At `$AAE6`, the three opcode/operand fetch cycles complete, and `peek` exposes
the pending data READ at the live DB mirror of `$2140`. `stop` becomes
`NBA_SOUND_PREFIX_SPC_RESPONSE`. No value is sampled; `accept` refuses both
zero and nonzero responses without changing the continuation. The pending
cycle is marked as the instruction's last cycle, but that cycle has **not**
completed and must not be charged or treated as an interrupt boundary yet.
The component has no API to fabricate completion or advance beyond it.

The mirror distinction matters: `$00/$80/$82/$BF:2140` are hardware mirrors;
`$7E:2140` is WRAM. A synthetic WRAM-bank case performs a WRAM read and stops
as unimplemented source, never masquerading as an SPC response. Other source
branches stop explicitly at their uncovered source PC. An early `$A1BE`
return is reported before its RTS rather than manufacturing caller completion.

`$AAE6 LDA $2140; $AAE9 BNE $AAE6` is an **idle-port** check, not an echo of
the next command. Source after it, not implemented here, writes Y to `$2141`,
the word at DP `$6C` to `$2142/$2143`, then command `$0B` to `$2140` at `$AAF9`.
`$AAFC CMP $2140` waits for the `$0B` echo; `$AB01` clears the command port,
then PLP/RTS restore caller state. Neither a ready zero nor an echo may be
assumed from CPU-side work alone.

## Carried-state ownership

| State | Source ownership and use |
| --- | --- |
| DP `$53` | Nesting/busy guard used by `$9FA1` and `$A13D`. Driver mutation routines decrement it and restore it. A nonzero value can bypass audio work; it is not a visit counter. |
| `$0637`, related `$0638..$0640` | Stream/upload state serviced by `$9FA1`. This prefix follows its inactive path. Nonzero stream work stops at `$9FB6`; eventual `$A047/$A06A/$A08F` handshakes need their own continuation. |
| `$062A/$062B`, word `$0635` | Pause/fade state selects `$A146`, `$A159`, or `$A16D` work. Uncovered routines remain explicit stops. The normal captured prefix has no active fade. |
| DP `$5A/$5B` | `$9B8C LDX $33; $9B8E STX $5A` initializes these from the caller's table bank/pointer state. `$A1AF..A1B2` load live `$5A` into DB; the observed value is `$82`. Do not hardcode that capture value as production initialization. |
| `$07CD..$07CF` | Queued sound-event sentinels. `$9B9B..$9BA3` initialize them to `$FF`; `$A1BF/$A1FA/$A235` test their sign. Pending event paths require further source work. |
| `$073A+Y`, `$0742+Y` | Eight channel activity and classification entries. `$A278..A2CD` scans them in source order, carrying Y=channel and X=2*channel. Classification >=`$10` takes another uncovered branch. |
| `$076A+X`, `$078A+X`, `$07BA+X` | Live per-channel word data. `$A295/$A298` copy the first to DP `$58`; `$A29A/$A29D` copy the next to `$6C`; `$A29F` increments the last before the idle-port check. |
| DP `$55`, `$0648` | Current channel and doubled channel index, written on each source loop iteration. Empty-channel work cannot be skipped when predicting instruction work. |
| `$062C`, `$0643/$0645`, sequence/channel tables | If the channel pass reaches its return, `$A1B6..A1BB` can enter `$A2CE`. This is the explicit boundary for the two longer native routes. Sequence parsing, allocation, modulation and command generation must eventually advance from normal initialization and menu dwell. |

Initialization `$9B90..$9B97` clears `$062A..$07ED`, followed by the three
negative event sentinels. That clear alone does not produce an established
Setup track: source initialization calls and subsequent NMI driver work
populate and mutate these channel tables. Source `$9CC8` and its callees
establish sequence pointers, tempo and active state; they are not implemented
by this prefix. Native snapshots are allowed only in explicitly labeled
isolated differential tests below, never in production or a normal journey.

The source's word INC at `$A29F` wraps at `$FFFF` and writes the high byte
before the low byte. The continuation preserves that order and wrap, along
with the DB mirror, caller widths, stack work and variable empty-channel
scan. No original oddity or timing variation is normalized as a repair.

## Required uploaded SPC consumer state

A future adapter must carry separate 65816-to-SPC input latches and
SPC-to-65816 output latches for `$2140..$2143` / SPC `$F4..$F7`, the uploaded
consumer program and ARAM data, SPC registers/stack/PSW/PC, execution phase,
timers and their control/counters, and relevant DSP state. This state must
derive from normal reset and source upload/initialization, then advance through
the actual menu dwell. A captured port response, command-event timeline,
preloaded ARAM snapshot or measured acknowledgement delay is not that model.

The current `src/nba_spc.c` is insufficient for a CPU acknowledgement adapter:
`spc_write` cases `$F4..$F7` simply return and discard the CPU-facing output
latches, while `spc_read` uses RAM-backed CPU-to-SPC inputs. Its snapshot-loaded
audio path was built for rendering. `nba_audio.c` Setup playback also applies
captured DSP events. Neither is accepted as the carried native sound-driver
state or a phase oracle. This component changes neither file. The exact reset
upload chain, uploaded SPC consumer code and subsequent source sequencer
closure remain to be derived and independently validated; this report does
not claim they have been mapped completely.

## Evidence and reproduction

The new natural capture `.analysis/native-sound-prefix-v1` contains 46 entry
and exit WRAM pairs, 3,675 raw instruction observations, 2,436 native data-bus
observations, and 92 boundaries. Its scheduler and all three prior interrupt
JSON traces are byte-identical to `.analysis/native-interrupt-tail-v2`.
Only normal controller input is supplied; no ROM patch, state injection or
save-state load occurs.

The isolated C differential accepts each immutable entry snapshot/register
state as an explicitly labeled component input. It receives no captured clock
or SPC value. Across all 46 calls it matches 3,673 source instruction/register
states, every instruction's CPU work, 2,392 accepted data access positions,
values/order, and all full 128KiB WRAM endpoints. The two additional raw
instructions are the observed `$A2CE` stop boundaries. The 44 additional raw
bus events are the unresolved SPC reads and are never supplied to C.
Native per-instruction master intervals conserve intrinsic work plus observed
40-clock refresh quanta; this is a differential observation, not forward
refresh or interrupt scheduling.

There are six observed idle-port prefix work variants: 52/78/91/104/117/130
instruction entries and 193/263/298/333/368/403 accepted CPU cycles. The final
read cycle is excluded. The two sequencer-boundary prefixes each have 140
instructions and 412 CPU cycles. These counts are verification outcomes,
not constants used by the implementation.

Nine integrity/probe tests require all capture/build identities, fixed capture
revisions, settings hashes, exact numeric domains and strict numeric token
parsing. C cases cover all eight channels, word wrap, unresolved-read immutability,
mirrors versus WRAM, instruction limit, early return and stream boundaries.
Ten parsed-view corruptions reject (native order/clocks/PC/scope and C
order/clocks/PC/read values/instruction completion). One positive independence
test changes only the observed pending SPC value to `$FF`: C trace and output
WRAM remain byte-identical, proving that response is not a differential input.

| Evidence | SHA256 |
| --- | --- |
| Native manifest | `715f9ea5cdd1657ed5da4eef283ed4f9c8a9e09086bb622e6cf248a632e53734` |
| Native prefix instructions | `6e14a5c012ce9ec32b3c7040d363236b54d4ff95399cf4f95eaca783bd38e54b` |
| Native prefix data bus | `ce03c09ce66a90ddadc0514cbf4277b868dbd4c9fb301d36a0906457e86b7389` |
| Native prefix boundaries | `7dc73600ed88d12a3e2a0f15ae0f65ba2596dfad3a7d94a3ea7d2c72f353eae6` |
| Source `$80:A137..A16C` | `4f1501225722b408c21b2614622f6b8c3a730e5ff10d85b3e6bd43fd9209f130` |
| Source `$80:A1AD..A2CD` | `b63c3d07290b2f26ea550283a5b48052595055809270926cc85ed4a54ae1f4e0` |
| Source `$80:9FA1..9FB5` | `ae9026b4e32bf9b46101b5f455593494e389006b80b9bbc897b26877296d22f1` |
| Source `$80:AAE3..AB05` | `a6267ba79d3b5aa1223a4130b85bb9f7b9476e60008e17ccdb0f0d3e76b82a5e` |
| Initializer `$80:9B73..9BD4` | `423fd72bba205b796bec620c7acbd8731a671c1319a9ed296030a28d03a6325e` |

Choose fresh private output directories from the scheduler worktree:

```powershell
python tools/generate_setup_sound_prefix.py --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --decoder-root 'C:/Users/joshs/Projects/tools/snesrecomp-source-v0.2.0-alpha/recompiler' --output src/nba_setup_sound_prefix_program.inc --check
.\tools\build_setup_sound_prefix_probe.ps1 -OutputDirectory .analysis/sound-prefix-build-new
python tools/verify_setup_sound_prefix.py --native .analysis/native-sound-prefix-v1 --previous-native .analysis/native-interrupt-tail-v2 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe .analysis/sound-prefix-build-new/setup_sound_prefix_probe.exe --output .analysis/sound-prefix-proof-new
python tools/test_setup_sound_prefix.py --native .analysis/native-sound-prefix-v1 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe .analysis/sound-prefix-build-new/setup_sound_prefix_probe.exe
```

Independent component audit is required before acceptance. Production wiring,
full sound prefix coverage, upload/SPC state, DMA/refresh/NMI scheduling and
Rules reentry parity all remain open. No commits, pushes, configuration edits
or additional consultant task were made by this subtask.
