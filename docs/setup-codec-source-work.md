# Bounded FB46 source work, 2026-08-31

The new C continuation reproduces the four actual FB46 backdrop resources,
including source instruction completion points and ordered writes. It is not
production-wired and does not predict the Rules reentry epochs. FB30, the
remaining backdrop/helper work, carried hardware phase and native audio/SPC
continuation still need implementation. The separate queue/wait freeze is
unchanged. Independent acceptance of this new component is pending.

`src/nba_setup_codec_work.c` translates `$80:C62B` entry to `$80:C682` entry,
excluding the final RTL, with `$86DA`'s empty-queue path and FB46
`$BD1B..BE6A`. It uses source-specific C control flow and resumable C labels;
it never decodes an opcode or executes an instruction-log script. Actual input
bytes drive dictionary construction, literal/escape handling, the original
unrolled tree expansion and its recursive helper. The original branches,
stack work and flags remain intact. In particular, indexed reads still pay
the native X=0 idle even when no page boundary is crossed.

The public `peek`/`accept` interface exposes individual ordered CPU bus/idle
cycles and native instruction completion points. Reads are resolved by the
caller at their bus access; writes are completed by the caller at that access.
The continuation commits local effects at instruction completion. The state
is relocatable at every bus boundary, including between an RMW read and write.
It contains no saved clock, frame, visit count, native output or port-response
trace. Its bounded preconditions are native mode, M=X=D=0 on entry, FastROM,
WRAM/IO bank mirrors, the covered non-crossing ROM streams and an empty queue.
The caller supplies ordinary live registers, DP operands, memory and stack.
Unsupported formats/nonempty queues and exhausted work limits stop explicitly.

The bus recipes were checked against the canonical ROM and Mesen 2.1.1
[instruction implementation](https://github.com/SourMesen/Mesen2/blob/2.1.1/Core/SNES/SnesCpu.Instructions.h)
and [memory timing](https://github.com/SourMesen/Mesen2/blob/2.1.1/Core/SNES/SnesMemoryManager.cpp).
These external sources are retained with URLs and hashes in
`.analysis/codec-source-v1/manifest.json`; they are diagnostic references only.
No emulator source was added to the production implementation.

## Source work checks

The totals below are verification checksums of emitted work. They are not
constants used by the continuation. There are no resource-address dispatch
tables or fitted timing values in the C module.

| Resource | Output bytes | CPU cycles | Intrinsic master clocks | Instructions |
| --- | ---: | ---: | ---: | ---: |
| `$AE:C446` | 960 | 34826 | 218142 | 9793 |
| `$AE:D153` | 2054 | 94738 | 602298 | 25219 |
| `$A6:C5FC` | 6336 | 246864 | 1556550 | 68978 |
| `$AF:97AA` | 838 | 32091 | 202290 | 8824 |

The independent consultant's source totals agree exactly. Fresh C runs start
with zeroed diagnostic scratch and typed source/destination operands; they do
not load native WRAM. A second leaf run supplies only the observed entry
registers and empty ring cursor to compare intermediate state. Both runs have
identical output and source work. This second run is diagnostic, not a claim
that the production caller now generates those entry states.

The four payload SHA256 values, respectively, are:

- `c95e94d4ab312fb92a6c11b4abac2701ec3fb200bdbe74cd2809fd20909913d1`
- `c6156c87d46b327445a0aa3343cd56949229b94aa1c670ffac94f33cf01d0086`
- `eec3d2898b2fbbd5316812b1338db01f38455d18da0420b50bb7d3681deb4dbd`
- `fa6b880bab74329256f33d260c60e8f9447098b8cbc9089c2c18626ec761b664`

Canonical ROM SHA256 is
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
The 336-byte FB46 routine SHA256 is
`ad45064bb30c6e7ed264f62b36132eb99e36928b7d0b5f06ace8b6f9fd6fbdc2`;
the wrapper's 88 bytes hash to
`ad561c575d19757d4e2fa41d6c267514e5957195eebb33c69b843807c9d3bc98`.

## Separate native validation

`.analysis/native-codec-work-v1` is a new natural controller-only run using
the frozen scheduler input script and private Mesen/home/save state. It adds
20 codec entry/exit WRAM pairs, 149243 first-resource instruction observations
and 49497 write observations. The run completed with exit0, both sentinels,
persisted settings and source/artifact identity checks. All 7102 earlier
scheduler records are unchanged. The four FB30 payloads/instruction records
are retained but are not accepted as an implemented FB30 work model.

For the four distinct FB46 calls, the fresh C run matches every native PC and
A/X/Y/SP/DB/P state at 112814 instruction entries. Every instruction's CPU
duration matches individually after removing independently recorded NMI work
plus the source's 19-cycle entry/vector/RTI overhead. All 28218 CPU writes
match in address, value, order and position within the native instruction.
Every complete output matches all four native exits: 16 FB46 payload matches.

Two observation conventions are handled explicitly, with positive checks:

- The broad exec hook runs before the NMI guard hook and records `$80815A`.
  Four hardware stack writes occur before this hook. The validator requires
  their exact addresses, bank/return-PC/status bytes and cycle positions
  before separating them from producer work. It never skips arbitrary writes.
- Mesen reports the WRAM effect induced by `$2180` as a second write callback.
  The validator checks the paired CPU write, sequential destination, identical
  byte/time and immediate event order. It does not count that induced effect
  as another CPU bus cycle.

Master-time comparisons remain conservation checks using recorded NMI
intervals, 142 intrinsic clocks for entry/vector/RTI, and 40-clock refresh
quanta. The reported residual refresh count is not a forward phase prediction.
These checks cannot substitute for an implemented clock/NMI/audio/SPC driver.

Native manifest SHA256:
`392e653f348441a2e80bb2f8f355b37a284fa34c58c3bf261418ce51dd05b52f`.
Native instructions SHA256:
`687b57de35c91eb6414a730e664799c8ae8264c73e3c72f68bdefb1bf83ad366`.
Native writes SHA256:
`c73001995ef34b1246386620f51c5f6cdfdb901cc76a1f4e58046e84dc04902b`.
The fresh probe `.analysis/codec-build-v7/setup_codec_work_probe.exe` hashes to
`e24fa6e0a9c93d271c3599fe050eaf53a06cc99615765a0a823d36739d778977`.
The build manifest and freeze attest the exact executable and source files.

## Reproduction and limits

From this scheduler worktree, choose new build/output directories:

```powershell
.\tools\build_setup_codec_work_probe.ps1 -OutputDirectory .analysis/codec-build-new
python tools/verify_setup_codec_work.py --native .analysis/native-codec-work-v1 --previous-native .analysis/native-scheduler-v3 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe .analysis/codec-build-new/setup_codec_work_probe.exe --output .analysis/codec-proof-new
python tools/test_setup_codec_work.py --native .analysis/native-codec-work-v1 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe .analysis/codec-build-new/setup_codec_work_probe.exe
```

The fresh compiler uses `/W4 /WX` and no game objects. The 12 C contract cases
cover synthetic dictionary/literal/escape decoding, data resolved at a pending
read, full relocation at every bus boundary, both empty-queue paths, live
register/stack restoration, explicit unsupported states and the instruction
limit. Thirteen Python tests, with multiple subcases, check malformed tokens,
strict identity sets/numeric types/settings and corrupt observation exclusions.

The original Rules entry/return epochs, brightness/RGB divergence and
Custom-return hidden-VRAM discrepancy have not been repaired by this leaf.
Do not wire this to captured phase inputs or call it a scheduler fix.
