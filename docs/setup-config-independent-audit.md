# Independent configuration audit

**PASS for the bounded configuration candidate; whole options/game completion
is not established.** Audited freeze:
`setup-config/build/config-freeze-v2.json`, SHA256
`1ff38b2d9ee858bf666ccaa6c77f960027aa6022010b8686100c18bfa2eb45a5`.

The auditor rebuilt all C sources into separate objects/probes under
`.analysis/config-transition-auditor-20260830`, without using implementer
objects or changing implementer files. Fresh replay passed730 stable
checkpoints/33,277 words and1,770 exact adjustment entry/exit observations.
All40 Main and7 Rules value canvases were also compared byte-for-byte over
65,536 bytes each against the actual raw native files, corroborating their
SHA comparisons. The30 protocol/evidence tests pass.

Seven raw Mesen journeys were reread independently. All337 source identities
were checked, and every reconstructed action/state/event row equaled its
retained compact fixture. The auditor inspected original ROM bytes,
Ghidra's `$81:AB58-$AC52` input producer/consumer and `$81:BDA3-$BF68`
dispatch, plus `$81:BFAA-$C00A` presets and Custom/commit references.
The native release, changed-word fast-repeat retention, whole-word dispatch,
cursor policies and working/committed boundaries agree with the bounded C
translation. Native and C factory screenshots were viewed: their background
phases differ, so this is not whole-frame RGB parity; the corrected value
shadows match the independently verified canvases.

One concrete verifier failure was found: the compact-fixture path accepted
`cpu_state_injection=true`, `rom_patch=true`, nonzero process exit and a false
classification while its state-row hashes remained unchanged. Version2 fixes
this with a shared provenance validator and a separately pinned copy of the
original native manifests. The auditor independently reran the four original
mutations; all are rejected. The new registry's manifests equal the actual
retained source manifests and their canonical digests. No C-derived state or
image was substituted for a native oracle.

Six historical primary manifests did not serialize numeric process exit.
Their exact pinned identities are accepted with a narrow, explicit exception:
the retained launcher writes final source hashes and verified-home metadata
only after its exit/completion guard. This is indirect success evidence,
not a retroactively invented exit0. New native captures require typed exit0.
This caveat must remain visible in release evidence.

The C adapter enters the real Setup component directly while native journeys
start at boot. Only button schedules cross into the C process; expected native
values do not. Stable comparisons exclude shared-buffer tails belonging to
other pages and Team Select's working buffer. Adjustment observations cover
the stated dispatcher boundaries and exact input offsets; they do not prove
every internal CPU state or input consumed during a divergent constructor.

Not accepted as complete: C disk-save/reload wiring, every option's gameplay
consumer, live audio commands, production multi-controller routing, upload
timing, complete screen transitions, hidden button sequences, or translation
of the general native font/resource producer. Existing captured glyph packs
remain a documented production-provenance gap. Integration with the resource
and transition candidate still requires root's combined build/regressions.

Auditor records: `final-audit.json`, `raw-source-and-byte-audit.json`,
`metadata-mutations-v2.json`, fresh `*-v2.json` replay reports and all47 raw
C canvases in `.analysis/config-transition-auditor-20260830`.
