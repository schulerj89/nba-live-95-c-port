# Header preparation before the epoch wait

The new `nba_setup_header_work` component translates `$80:EEC6` through entry
to `$80:EF1A`, before the JSL to the epoch wait. It produces the source work
of the five-word clear, two4096-byte fill requests, forced-blank write and
14-byte palette publication. Its intrinsic work is440CPU cycles,
2764master clocks and126instructions. It does not sample an epoch or wait.

This is a new standalone component. The accepted queue, FB46, FB30 and frozen
producer modules are unchanged. Production timing/configuration is unchanged.
Root owns integration after independent review; no Rules phase/display parity
is implied by this leaf.

`generate_setup_header_work.py` emits118 static source states from canonical
ROM bytes and the pinned disassembler. The compiled C has direct source
labels and known helper return sites; no runtime opcode decoder or captured
instruction/state/timing input exists. The source's clear loop, separate
low/high fixed-source fills, forced-blank store and selector>=35 fallback
remain intact. The synthetic selector35 route matches selector0 with the
original eight additional routing CPU cycles. No source behavior is shortened
to match a timing target.

The API accepts ordinary typed registers with caller preconditions native
mode, direct-page zero, M=X=decimal=0, FastROM, live WRAM/IO mirrors and
immediate publication. It exposes one read/write/idle at a time and commits
local work at instruction completion. DMA requests are visible at `$420B`;
the external driver owns pending service, alignment, transfer progress,
refresh and interrupt eligibility. This component stops before the wait JSL;
the caller-width behavior of `$80:86B0` remains in the earlier queue contract.

The default diagnostic call derives layout operands from the ROM table
`$81:B8C2` and the `$80:E95B` conversions. Selector34 is the source operand at
`$81:D00A/$D00D`. It starts with ordinary typed fields and zero scratch, with
the ten cleared bytes deliberately set to FF to test the clear. No native
snapshot or clock initializes the work producer. A second run using only
native entry registers produces identical work and DMA payloads.

The new read-only `.analysis/native-header-work-v1` capture loads unchanged
copies of all three earlier observers. All seven prior scheduler, codec and
producer JSON traces remain byte-for-byte identical. It adds eight header
boundaries,127 instruction observations (including one positively identified
NMI-entry hook) and16521 data-bus events. The existing full header snapshots
remain part of the required artifact set. Private settings/save state,
completion sentinels, source revisions and all artifact hashes are rechecked.

Fresh C work matches all126 native PC/A/X/Y/SP/DB/P states, every instruction's
CPU duration and all66 CPU write addresses/values/order/bus positions. Both
4096-byte fixed fills and all14 palette bytes match native DMA source reads
and PPU writes in order. Each byte is tied to its source DMA request. All four
header intervals independently conserve440CPU cycles after removing observed
NMI work and19 entry/vector/RTI CPU cycles per interrupt.

The123 instruction intervals that do not service DMA also conserve intrinsic
master work after observed NMI work plus142 clocks per interrupt and40-clock
refresh quanta. The three intervals servicing DMA have exact source CPU work
and byte effects, but their elapsed service/alignment time is not predicted.
The four NMI hardware stack writes are validated by address, return-PC/status
value and cycle position before separating them from source writes. The raw
read observations remain retained; this is not a full CPU-read timing gate.

Thirteen Python integrity tests with subcases, eleven C continuation cases
and nine in-memory corruption cases pass. These cover relocation at every
bus boundary, source fallback behavior, clear/forced-blank effects, bounded
failure paths, strict identities/numeric tokens, mixed-event chronology,
intrinsic timestamps, DMA associations/payloads and NMI stack exclusions.
The first negative-test runner had a recursive mock bug; its failed output is
retained in `header-mutations-v1`. The corrected runner completes all nine
cases in `header-mutations-v2`; no native fixture was modified.

Canonical ROM SHA256:
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
Header source `$80:EEC6..EF19`,84bytes:
`762dd6415634de0923e9440f87ed1f7331475cbe28e58df53c0ad4da3a2dfbef`.
Fallback `$80:EF8E..EF93`,6bytes:
`fc66680b0791d2d3b00f64e34646b368734dc973e4bb42a45d158e315c9ccd95`.
Helper source identities are retained in the producer work record.

New native manifest SHA256:
`1670fdb5a688f738be33b8f0c17b4d20a926a410957fd73322743f7f2debd1cf`.
Instruction trace SHA256:
`120ec91aa75a38eb8df091ac92ecfaa9bb2d131ef451bc60533ed11b01133c8b`.
Bus trace SHA256:
`7d0b81bc27c4360399c58d0e3fc09c2f2d01e7f7b97fff3817a080d220720403`.
Boundary trace SHA256:
`8bc52dce3ac04f56a380d5e6056a9efe20b4a1d82487c89e6723890cad7b4f9e`.

Reproduce from the scheduler worktree with new output directories:

```powershell
python tools/generate_setup_header_work.py --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --decoder-root 'C:/Users/joshs/Projects/tools/snesrecomp-source-v0.2.0-alpha/recompiler' --output src/nba_setup_header_program.inc --check
.\tools\build_setup_header_work_probe.ps1 -OutputDirectory .analysis/header-build-new
python tools/verify_setup_header_work.py --native .analysis/native-header-work-v1 --previous-native .analysis/native-producer-work-v1 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe .analysis/header-build-new/setup_header_work_probe.exe --output .analysis/header-proof-new
python tools/test_setup_header_work.py --native .analysis/native-header-work-v1 --rom 'F:/Games/SNES/NBA Live 95 (USA).sfc' --exe .analysis/header-build-new/setup_header_work_probe.exe
```

Independent audit is required before acceptance. Forward master phase,
pending-DMA service, refresh/NMI scheduling and audio/SPC continuation from
ordinary initialization/menu dwell remain necessary. In particular, this
does not predict loaded epochs72/15/71/15 or after-wait73/16/72/16.
