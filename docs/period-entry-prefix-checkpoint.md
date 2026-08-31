# Continuing-period entry prefix DCA6 to DD97

This separate typed data module closes the inline prefix before the accepted
period formation parent. Four frozen controlled-expiry native pairs match
every WRAM word at DD2D, DD47 and DD97:786,432 comparisons from four DCA6
prestates. No native after-state, external child return, historical complete
initializer or phase estimate is supplied to C. There are no JSL/JSR calls
inside DCA6-DD96. Production, main, existing sources and the human launch
freeze remain unchanged. No new Mesen process was needed.

## API and exact data ownership

`nba_period_entry_prefix.h` has ten typed actor records and39 global/input/
carry words. Twelve fields per actor plus39 globals give159 diagnostic fields.
The implementation accepts already incremented0926; it does not advance the
period. The three routines end before the named next instruction:

| Routine | Source | Owned changes |
|---|---|---|
| `nba_period_entry_prefix_reset` | DCA6-DD2C | Clear091C/0948/094A/0962/0964/0966/096A/09BC/097C/094C/09D0/0A02/0A04. Set4015=0822,401B=FFFF,1864=0. Clear ten actors' +30/+32/+38/+42/+44/+3A/+3C/+46/+48/+28, set+18/+1A=FFFF. Clear09B4/0964/09BC/09B6 and4713/4793/4741/47C1. |
| `nba_period_entry_prefix_clock` | DD2D-DD46 | Unsigned period>=4 selects86E392[17B1*2] into0A0C. Regulation preserves0A0C. Copy0A0C into0928. |
| `nba_period_entry_prefix_table` | DD47-DD96 | Set092C/0994=05A0,0996=1. Only period2 negates both46F5/4775 anchors as wrapped16-bit EORFFFF+INC. Copy current46F5 intoB6; set093E/09C0=FFFF,9A=34D3,34D1=0. |

The combined function validates an overtime table index before any mutation.
Valid17B1 is0..3 when period>=4; regulation never reads17B1 and accepts its
carried word. Other period words retain the source unsigned comparison, but
the downstream accepted formation parent has its own0..4 bounded domain.
Out-of-table overtime indices are explicit host input errors, not newly
invented native branches. Tables are passed as typed original ROM data:
86:E392 supplies7200,10800,14400,18000. The helper does not invent seconds,
new UI settings or elapsed-time behavior.

Queue cursors becomeFFFF; their +1C/+22 queue contents remain intact.
Positions, fractional words, ownership fields not listed, controller state,
animation resources and other actor/global fields remain untouched. The
probe projects159 typed fields into its original full131072-byte input and
compares all65536 words at each boundary, including unowned memory.

## Original carry and caller boundary

The continuing-period caller goes87:976E (period increment),9797->8C86,
then8CA6->86:DCA6. It does not call new-game86:DA18 or DA3F's bulk clear.
09BA and09B0/09B2 are preserved exactly. Native ready1 stays1 in all four
cases; dead coordinates are both0 there. Nonzero dead-coordinate preservation
has source-only coverage, not a new natural witness. Source comments retain
this original stale-state behavior instead of substituting a generic inbound
finalizer. Period2 anchor negation preserves wrapped cases0 and8000 as well.

After DD94, the native next-boundary registers are A=34D3,X=0,Y=34EB,
D=0. N/Z reflects LDX0; carry reflects CMP(period,2), which the following
loads, stores, EOR and INC preserve. DD2D's A/X=3EEB,Y=0 and DD47's clock/
index values are independently checked from source. C exposes data, not CPU
registers or an interrupt scheduler. The frozen captures contain A/X/Y/SP/D/P
but not DBR, program bank, emulation flag or cycle counter. Binary16,D0 and
effective absolute-write bank7E are the implementation domain; direct DBR/E
register observation is not claimed. All four prefixes keep SP1FF9 and the
original8CA6 return frame, with no captured outer-frame crossing. No general
IRQ/NMI timing claim follows from that fact. UI/drawing after DD97 and all
earlier8C86 scheduling are separate responsibilities.

## Evidence and strict checks

ROM: `F:/Games/SNES/NBA Live 95 (USA).sfc`,1572864 bytes, SHA256
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
Fresh original bytes, bounded recompiler and Ghidra listings are in
`build/period-entry-prefix/reference-v1/manifest.json`. It binds tool sources,
processor files, commands and the241-byte DCA6-DD96 prefix plus8 clock bytes.
`period_entry_prefix_reference.py` independently reads destinations/literals
from fixed original instruction blocks; it is neither a CPU interpreter nor
a transcription of the C field table.

The native corpus stays in the owner worktree under
`build/period-restart-attribution-v1`. The four selected directories are
period-0/1/2-ready1-children-v2 and period-3-ready1-children-v3. These are
controlled expiry injections after ordinary cold boot/play, not naturally
elapsed full periods. Only0926/0928/4711/4791/09B4/13E7 were injected earlier;
the formation/target/ready states were not seeded. Native freeze SHA256 is
`04e4c13a1b7298b97fd72fac004e73f58cf6f2eb5bcddf0eaf389eeb404f3d2b`.
The verifier imports the exact accepted native contract read-only, SHA256
`68d22789ecaff106b9b2c773a821a5a3510c3a984dd5d8aeac6e61b03c6f2eca`,
then checks the frozen manifest/row/raw identities and additional prefix
clock, CPU, stack, decimal and carry requirements. No shared helper mutates
the attestation. Executable/build sources, all response fields/word types,
integer exit0 and exactly empty stderr are mandatory.

| Already incremented period | Native frame/court | 0A0C after | Anchor behavior | Compared words |
|---:|---|---:|---|---:|
| 1 | 6767/2377 | 43200 | Retained | 196608 |
| 2 | 7127/2737 | 43200 | Both negated | 196608 |
| 3 | 6767/2377 | 43200 | Retained | 196608 |
| 4 | 6947/2557 | 18000 | Retained | 196608 |

All have17B1=3. Other quarter options, high period words, out-of-table error
handling, varied ready/dead words and zero/extreme anchors are among90
source-only C/reference cases (14310 typed words). The independent reference
also exactly matches all four native intermediate sequences. All56 integrity
mutations reject. These counts are not whole-game coverage percentages.

The asset-free probe freshly builds only the new module/probe with /W4 /WX.
No prior objects or CPU emulator are linked. No C/native mismatch or failed
verification occurred in this packet. Both successful build logs and all
input/output artifacts remain available. The separate freeze binds eleven
new files, the native/source dependencies and all sixteen earlier immutable
controller/human freezes. Integration should feed its DD97 result to the
accepted period parent; it must not seed from native DD97 or invoke DA18.
