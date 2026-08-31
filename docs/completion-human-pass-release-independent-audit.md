# Independent mode15 release checkpoint review

PASS for the frozen bounded mode15 components and their natural-prestate
comparisons. This does not accept the missing `86:99C4` launch body, general
human play, full animation coverage, normal initializer replacement, or timing
prediction. No original source or native fixture was changed.

## Frozen source and independent builds

Controller `build/human-pass-release/freeze-v1.json` SHA256 is
`684e6dda6a2cdab80cc0b3a2c6a00891fab8d6d5fbe0271e920d7d77b0dcfeda`.
All 218 identities were independently rehashed: 14 earlier freezes, 11 new
sources, 17 dependency sources, nine private objects, three external artifacts,
and 164 artifact entries. The private auditor copy and inventory are under
`build/pass-release-audit-v1`. No objects or large captures were copied.

The unchanged build script ran against copied source, freshly compiling the
new module/probe, accepted pose module, asset loader and its five remaining
render/validation dependencies under `/W4 /WX`. It uses no old gameplay objects
or session ABI. Fresh executable SHA256:
`29a79ba285f808d3dec598b9e2054b32a1815273297403d13ac7d0a910e480c4`.

| Reviewed item | SHA256 |
| --- | --- |
| `src/nba_human_pass_release.c` | `f1e395b197d6252aeb42bd1ce761f1c31976fdd56e98a1ede494db4eb93cca15` |
| `include/nba_human_pass_release.h` | `2399d929421701972b1a4ba16bc542fb145a0cbde3091a30b27b490307e2bed3` |
| `tools/human_pass_release_probe.c` | `693e762679f342fd0f5b970240b1425ea78749497bb4d85d0dfdb33535c07c95` |
| `tools/verify_human_pass_release.py` | `1b7524f44b139ed04d34b54bba4cdb06786d892a795c269af645c1cd382626bd` |

The canonical ROM and asset pack retain SHA256 `2115c39f...7870` and
`951f8233...d4df` respectively (full identities in the inventory and reports).
The probe compares 49,536 pose/attachment asset bytes directly to the ROM and
reads both the eight release thresholds and ten actor pointers from that ROM.
Inputs contain only a component name and its own actual raw entry snapshot;
no expected output or later endpoint is supplied to C.

## Source and caller findings

I read the original routine/table bytes, Ghidra output, recompiler output,
capture script, actual raw boundary data and C/probe mappings. The reference-v4
closure independently rehashes 67 tool sources and 18 output files, and all 13
range/table identities match the original ROM. A separate decoder checks all
525 Ghidra instruction rows with the proper SEP/REP width changes in the IRQ
routine. The first diagnostic rejected the naming difference at `87:9BD0`:
Ghidra calls opcode DC `JML`, while the bounded decoder prints `JMP` with the
indirect operand. The retained corrected diagnostic accepts only that exact
opcode/PC alias; it does not waive other length or mnemonic differences.

`87:9244..9258` indexes mode15 to the real words `9C53/0087`, then JSLs the
`87:9BD0` indirect trampoline. `87:9C53` adds a second JSL frame for `86:A6B3`.
The verifier checks those actual two frames and RTL targets, not a fictional
direct table entry into A6B3. The C dispatch owns only DP8E/90; stack execution
remains outside the typed module.

The nonowner path subtracts actual DP C6 from actor+60 with 16-bit wrap. A zero
timer returns; a negative result calls `9846`, selecting mode1/2 from actor+6E
versus093A, assigning behavior47, and clearing +60/+7E/+28. The shared `9861`
entry used by cancellation does not overwrite mode or behavior itself.

The owner gate correctly uses the sign from **wrapped CMP** at A6D0 and A70C.
Family2 stops before A629; family3 writes family2 and stops before A6F8/AEC3;
family4 reaches launch/cancellation directly. The other family route may turn
at A7A8 before reaching the explicit AD6B steering stop. These missing children
are not approximated. Positive receiver indices outside the actual ten-word
table are a documented rejected domain; invalid calls leave the input intact.

`A7B5` tests the raw facing-minus-direction word before masking with7. Thus a
nonzero difference divisible by8 still turns after the mask produces zero.
The C comment retains that original quirk without claiming its natural
reachability. `A764 CMP4/BPL` likewise preserves wrapped-sign behavior rather
than comparing signed C operands. The independent edge proof below covers both.

The ordinary wait route selects upper2A..31 only when the actual byte threshold
is at least phase3A. It reuses B649/B832 with existing pose resources, without
resolving a new pose, and then writes actor-relative ball Z. Its 183 natural
waiting calls match separately at offset and attachment boundaries. Point1,
family2/3/4, cancellation and unassigned steering remain source-only coverage.

For a valid launch candidate, the source clears09C4, stores the receiver word
to AA, loads its real879C7B pointer into8E, and stops before99C4. The separate
`after_launch` component starts at the actual A75F native return prestate; it
does not seed that endpoint into the unfinished launch operation.

## Fresh native and controlled results

| Natural capture | Mode calls | Origin passes | Compared values |
| --- | ---: | ---: | ---: |
| selection0-v3 | 185 | 13 | 1,071,928 |
| selection2-v3 | 332 | 17 | 1,806,536 |
| Total | 517 | 30 | 2,878,464 |

Reports `selection0.json` and `selection2.json` are fresh independent runs.
Both complete stdout/stderr pairs are byte-identical to the frozen final
outputs. Native source/runner versions, actual commands/environment, private
settings/saves, all 5,048 raw event snapshots, numeric domains, source PC and
CPU bank/PC agreement, stack previews, B-origin ownership, call/origin order,
front-end clock anchors and branch boundaries are checked. Source requires
binary 16-bit arithmetic: DP0, DBR7E, PS&38=0 at every runtime row.

The ordinary input script uses real menu input and released Player selection,
then direction choices and short/32-frame B holds. Both held-B and released-B
launch observations occur. These natural mode15 routes are animation-gated;
there is no invented B-up release condition. The C component does not install
controllers or enable human play.

`tools/test_human_release_rom_audit.py` is a separate audit-only original-ROM
byte executor. It compares all 50 mapped C words and the explicit stop result,
not merely hand-entered endpoint expectations. A DLL links the same fresh
objects and checks its struct size with a compiled sizeof helper. It passes
131,733 controlled cases across 118 source PCs: all 65,536 family words for
the A75F suffix, all 65,536 for the owner gate, 256 timer/credit edges, 256
facing/direction edges, negative-receiver and steering cases, 100 normalizations,
and actual mode15 dispatch. It deliberately excludes attachment and external
children from this reference; those natural component comparisons remain
separate. It makes no CPU-cycle or natural-reachability claim.

The first private DLL link put `/EXPORT` directives on a separate response-file
line; MSVC ignored them and the diagnostic could not find the function. That
build and empty output directory are retained. The successful
`controlled-build-v2` has no warnings; `controlled-rom-v2/report.json` is the
completed result. No reviewed source changed to satisfy the reference.

## Verification integrity and scope limits

All 126 unchanged local integrity mutations reject with the fresh executable.
The new independent `tools/test_human_release_protocol_audit.py` rejects all
44 cases: C result/schema/vector/type/process/stderr mutations, decimal-mode
corruption at every observed runtime tag, and active-frame stack corruption
at the dispatch/wrapper boundaries. Its small immutable raw-state cache only
avoids repeated decoding; it does not bypass source/artifact hashes, metadata,
expected outputs, or the executable. Original files remain unchanged.

Native interrupt activity may replace discarded bytes below SP; actual caller
frames stay checked. At right court1461, raw05C8 changes EEEE→EF14 with05CA=85.
Original `85:EF05..EF0E` installs that exact successor and preserves A/P.
The writer PC was **not** captured. This remains explicitly labeled source
attribution of an observed interrupt effect, outside the declared C projection;
unknown pointers/banks are rejected. It is not instruction-level IRQ proof.

Two left launch observations cross a frame inside the unexecuted99C4 child;
every compared C segment stays within its own frame. The final right origin
has timer19/mode15/nonownership at court2399. Its cleanup is unfinished at the
fixed capture end, so only 29 normalizations are accepted. No thirtieth result
is fabricated, and no complete-human-journey claim follows from this checkpoint.

All historical implementation/reference/capture failures remain separate. No
production caller or enabling change was introduced. The next substantive
missing human boundary remains99C4 itself.
