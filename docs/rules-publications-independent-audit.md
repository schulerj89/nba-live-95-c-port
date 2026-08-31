# Independent Rules publication audit

Date: 2026-08-30. Auditor: gameplay workstream, separate from the transition
implementer. **The bounded publication repair passes the reviewed behavior;
complete Rules reentry remains FAIL.** Final frozen source identification and
fresh integrity results are recorded in the addendum below.

This review inspected actual ROM/Ghidra/recomp routines, raw native jobs and
VRAM, implementation code, and independently rebuilt C output. It did not
accept the implementer's PASS summary as evidence. Native and C comparisons
use the existing fixed input schedules; no timing search, tolerance, new
golden or expected-frame replacement was accepted.

## What is being repaired

The old transition deltas described changed values, not writes. A native
zero fill could therefore disappear when it wrote an already-zero captured
byte, leaving a changed live value uncleared in C. NBSPPU3 retains observed
DMA destinations, including these zero writes. Font jobs substitute only
current value glyph bits, at the captured publication frame. A subsequent
Rules visit starts with the prior Main constructor's full live VRAM/CGRAM.
The live BG2 phase survives the outgoing prefix.

This is still a capture-scheduled resource implementation. It is not a
complete translation of producer execution, scheduler work, queue servicing
or arbitrary DMA. Its glyph copying also remains a limited renderer contract,
not the original proportional font writer. Captured VRAM/CGRAM are resources;
RGB files are evidence and are not inputs to this production trace builder.

## Independent owner and evidence review

Reviewed fresh generated `phase-probe/reference/bank80.c`, `bank81.c`,
`bank87.c` and the existing `gameplay100-closure-ghidra` bank80/bank81 listings.
`$81:BA8E-$BD22` and `$81:CF62-$D31E` own Main and Rules construction.
The Rules constructor calls `$80:EC68` then `$80:EEC6`, creates the frame
task, and invokes the entrance routines. `$81:F9FC` calls `$87:89D5`:
`$168F=2` clears the phase and increments `$0613`; other normal phases
increment. `$80:EB85-$EB9E` clears the scroll words including `$0613`.
The C three-frame helper is equivalent over the native phase domain0..2.
It does not claim equivalence for arbitrary invalid imported phase values.

The first Rules text loop starts X=4, advances by2 through16 (rows2..8),
then queues a font upload. The later loop starts X=16 and continues through24
(rows8..12). These support the two separately scoped font publications.
The recorded jobs match two4096/4096/2048-byte groups from
`$7F:2360/$3360/$4360`; return has one4096/1024-byte group. The old concurrent
`$80:A2BF` screen-owner attribution was wrong; the new comments correct it.

I read `native-repeat-v1/counter_calls.csv`: dispatch470 enters phase1,
dispatch1100 phase0; both call the same native counter routine. I also read
`native-producer-v1/producer_calls.csv`: the header builder starts541 at
scanline198 versus1171 at172, with its yield reached542/scanline2 versus
1171/224. This explains why a fixed first-visit schedule cannot establish
second-visit timing. It does not authorize an invented visit-specific delay.

Production evidence is the attested, isolated
`phase-probe/native-publications-tracked-hold-v1`. The independent parser
read233 jobs/237 segments: four split jobs, all supported fixed-source mode8.
The repeated diagnostic has466 jobs/473 segments and an additional split
mode1 job412 at1511 (2979 then3357 bytes). That unobserved bus-phase contract
is rejected for production export. The two earlier captures affected by
shared PowerShell environment races are not accepted evidence.

The runner now passes a private child environment, verifies the observed
folder/profile and uses a separate portable Mesen home. The parser attests
raw jobs, segments and observed environment; it checks exact columns,
ordering, complete job coverage, supported addressing and source-font groups.
Its coverage map represents the final writer per address within a frame,
not every intermediate bus operation. That distinction limits the claim.

The reviewed Mesen commit is `137ae7ce3bf3f539d007e2c4ef3cb3b6c97672a1`.
Its [DMA implementation](https://raw.githubusercontent.com/SourMesen/Mesen2/137ae7ce3bf3f539d007e2c4ef3cb3b6c97672a1/Core/SNES/SnesDmaController.cpp)
decrements remaining size after copying the byte. The
[memory manager](https://raw.githubusercontent.com/SourMesen/Mesen2/137ae7ce3bf3f539d007e2c4ef3cb3b6c97672a1/Core/SNES/SnesMemoryManager.cpp)
advances clocks within DMA reads/writes. The earlier contrary decrement-order
explanation was withdrawn. No uncertain low-byte phase is converted into a
tolerance or an exactness claim.

## Verification performed independently

Built the actual game from the private source list into
`.analysis/rules-resource-independent-20260830/`. The first independent
executable SHA256 is
`45bfcfab4cb987a1fac08e1e82da53250c4adecb472bd528e6543b141def29aa`.
With pack `1f8984f556f03d167a18bc7ccfd85cb26d19c1870c812b198ff0ffe4914fa38a`:

| Gate | Result | Meaning |
|---|---|---|
| First opening | PASS147 RGB frames and147 mapped PPU states | One phase-aligned, naturally configured first entry |
| First return, unchanged | PASS171 RGB,133 mapped PPU | Existing fixed held-menu case |
| First return, row2 OFF | PASS171 RGB,133 mapped PPU | Out Of Bounds changed and Custom committed |
| Held menu | PASS137 RGB frames | Existing settled-menu sequence |
| Reentry gate | FAIL158 RGB/state comparisons | First difference native1176/C873 brightness |

The RGB denominator is57,344 visible pixels per stated frame. Mapped PPU
comparison covers the22 published register fields plus forced blank, not
the full emulator state. These counts exclude other transitions, menu
combinations, scheduler/internal CPU state, audio and gameplay.

I reconstructed the native full64KiB VRAM from raw base plus observed writes
and compared every byte with the private C raw observer, independently of
its telemetry hash. I verified that observer is the actual `main.c` with
only a read-only `fwrite` block added. Results exactly reproduce:

| Native range | Full VRAM | RGB |
|---|---:|---:|
| First Custom return830..962 |132/133 exact|133/133 exact|
| Second opening1101..1246 |136/146 exact|97/146 exact|
| Second return1461..1592 |122/132 exact|132/132 exact|

Raw independent report:
`.analysis/rules-resource-independent-20260830/direct-vram-rgb-audit.json`.
At870 exactly25 values differ between0x8A48 and0x8A80. Thirty completed
write positions differ, but five already held zero; calling all30 byte-value
differences would overstate the result. Second opening's first visible
difference is1180. I viewed its synchronous raw native frame beside C877:
native has begun revealing the right edge, while C is black. Pixel equality
in the second return does not waive its ten frames of VRAM differences.

## Audit corrections and acceptance limits

Review required: reject multiple VRAM channels in one DMA submission;
reject split mode1 export; remove the unsupported decrement-order claim;
and validate the whole NBSPPU3 stream before modifying live resources.
The new C preflight checks magic/version/count, PPU domains, forced-blank
flags, sorted unique VRAM/CGRAM addresses, scopes, CGRAM bounds, truncation
and exact trailing length. Options version2 remains outside this hardening.

The first freeze manifest no longer matched four edited source/doc files
after these corrections. I explicitly rejected that source/executable
identification and requested a new freeze rather than assuming unchanged
behavior. No first-freeze hash is evidence for later source.

I independently decoded every packed baseline publication, applied it to its
raw native base, and compared full VRAM plus CGRAM with the attested native
write stream after each frame. All146 opening and132 return states are
byte-exact. Opening has132,008 writes:111,528 scope0,10,240 scope1,10,240
scope2. Return has103,784 writes:98,664 scope0 and5,120 scope2. This verifies
the pack conversion of this corpus, not portable execution of its original
constructors. Report: `packed-publications-raw-audit.json` in the independent
audit evidence directory.

The production pack changes only resource IDs145 and155. Root must rebuild
the current full pack, preserving the independently integrated intro assets;
the older candidate whole pack is not a deployment replacement. This
checkpoint does not complete Rules reentry, all menu variants, or the game.

## Final frozen addendum

**PASS for the bounded repair and preservation gates; FAIL for complete Rules
reentry.** `resource-freeze-v2.json` SHA256
`828fc6e09f2abe63bc28a4cf6bae27766d41f0d360891718cf9bebb39b0bd658`
was independently checked: all21 attested objects match their bytes/hashes.
I rebuilt the final C sources independently; executable SHA256
`8de57cf76317645ed2b9fdc7a59aa6317d26958dabc905132937306dd758a3a2`.
Fresh147 opening,171+171 return,137 held-menu and four one-frame changed
Rules UI witnesses all PASS. I also rebuilt the public C decoder probe and
ran40 native-positive/corrupted-stream checks, plus all27 existing verifier
tests and11 publication integrity tests; all PASS. These integrity counts
are not ROM behavior coverage.

The final delta reader validates every raw row before clipping the selected
frame range. Duplicate/reordered/out-of-range addresses or values, malformed
columns and invalid relative-frame/type inputs now fail instead of silently
overwriting. The fresh guarded hold-v2 capture independently reproduces all
12 raw resource files and548 synchronous RGB files byte-for-byte from the
accepted hold-v1. Thus the new multi-channel guard did not silently change
its production corpus. The frozen pack hash is unchanged.

The reported158 reentry differences, single-return870 VRAM difference,
incomplete scheduler/constructor translation, Options version2 and limited
glyph-copy provenance remain open acceptance failures. The new strictness
does not make those differences exact or expand this checkpoint to other
transitions. No expected native witness was changed during this audit.
