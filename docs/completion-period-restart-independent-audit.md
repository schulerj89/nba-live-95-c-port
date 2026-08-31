# Independent period parent review: v1 rejected

The frozen period parent is not accepted. It contains an original-source
mismatch in the positive-anchor opening/overtime branch, and its native gate
accepts unsupported decimal-mode and invalid entry-register metadata. The
three regulation and one negative-anchor overtime native comparisons still
pass unchanged. This is a port/verifier repair, not permission to change an
original-game quirk or native fixture.

## Reproduction and identities

All 666 identities in scheduler `.analysis/period-restart-freeze-v1.json`
were independently rehashed, including the parent's 573 original native
identities. Freeze SHA256:
`bbb9fddb581a17a43fda57b2647fd149d5480a84f5b179bada7ae802deb233fb`.
Copies, fresh build and results are under `build/period-restart-audit-v1`.

The unchanged module and probe build freshly under `/W4 /WX`, with no reused
objects. C SHA256 is
`91e6add6e74123f4aa600b7ce81d4b5e0e808f4a0b257c197ef60557ba440381`;
verifier SHA256 is
`2404e1778c37eab8d5034d099a781273ab5f18acf8b1060b1136946f841e4e5d`;
fresh executable SHA256 is
`19788fcd6e345795a284677a7c49d10b9c3517822c81422b8794688a4b129b18`.
The canonical original ROM is unchanged, SHA256
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.

The fresh four-capture run passes all 70 parent boundaries and 28,420 typed
word comparisons. All 53 unchanged local checks also pass. Their limits are
material: the controlled-expiry corpus uses tip winner5, and the overtime
capture has a negative team0 anchor. The local branch matrix repeats the
incorrect positive-anchor Y rule, so those passing checks do not refute the
source mismatch.

## Opening/overtime Y mirror mismatch

`src/nba_period_restart.c`, `pair_before`, handles a nonnegative opening/OT
anchor by negating X and rotating direction, while leaving Y unchanged.
The original source also negates Y:

```
86:DDE7  A5 BA       LDA $BA
86:DDE9  49 FF FF    EOR #$FFFF
86:DDEC  1A          INC A
86:DDED  85 BA       STA $BA
```

These exact bytes are at unheadered ROM offsets `35DE7..35DEE`. This is not
inferred solely from a Ghidra listing. `DEDE..DF24` then publishes the first
team's adjusted X/Y and negates both for the paired team in the opening branch.
For period0 or4, anchor0=+336, pair0 must therefore be:

| Actor | Original X/Y/direction | Frozen C X/Y/direction |
| --- | --- | --- |
| 0 | -8, -3, 2 | -8, +3, 2 |
| 5 | +8, +3, 6 | +8, -3, 6 |

Both actual Y and target Y are wrong. There is no claim that the existing
native corpus reaches this branch; the API and local source-only tests support
it, and the original ROM unambiguously defines it.

The new `tools/test_period_formation_rom_audit.py` executes a bounded subset
of the original ROM bytes from DDA7 through DF27, with source-defined pair
registers and carry behavior. It neither reads the C table nor reproduces the
implementation's branch formulas. It compares both teams' eight coordinate,
direction and A6 fields at all five pairs across periods0..4 × winners0/5 ×
anchors±336: 20 cases, 1,600 field comparisons. The frozen C has exactly 80
differences, all Y/target-Y words in the four positive-anchor opening/OT cases.
Report: `independent-formation/report.json`. Tool SHA256:
`881ee55b4e447218353335ca2550ef878124d0d8532314038776f2e69340e8d0`.

Repair the opening positive-anchor branch to negate Y as the source does,
and correct the new revision's documentation/local expectation. Keep this v1
source, its passing-but-insufficient local report, and the independent rejection
unchanged. Rerun the same independent tool on a new C revision.

## Native entry-domain omissions

`verify_period_restart.py:read_native` only checks `ps&30==0` for the parent
domain. The original parent includes ADC/SBC and the C helper implements binary
arithmetic, so D=0 is also a necessary caller precondition. Decimal mode can
change direction subtraction and actor/list pointer arithmetic; it is not a
supported alternative input domain. Add an explicit D0 contract and reject
`ps&38!=0` at the relevant parent boundaries without changing arithmetic.

The native entry at DD97 also depends on the fixed preceding `DD91 LDY #34EB`
and `DD94 LDX #0000`. The current gate accepts X=1 or Y=0 in the captured
`formation.table` row. Those values alter the original table/address path and
do not represent the typed helper's starting contract. Bind X=0 and Y=34EB
at that entry. A is overwritten by the DD97 LDA and must not be needlessly
fixed. Raw scratch B6 should agree with the source-carried context0 anchor,
and raw9A with the DD89 cursor; these are actual preceding-source conditions,
not a new bulk reset.

`tools/test_period_native_domain_audit.py` makes only parsed-view mutations
with explicit hit checks. It accepts all eight D-bit corruptions and both
wrong X/Y entries under v1. The complete 10-case rejection is retained in
`independent-domains-v2/report.json`; the initial eight-case report and original
tool SHA256 are preserved in `independent-domains`. Current tool SHA256:
`68e02c3ec1ddcacbe4bbf582d30eac5ba898b0b1195cb546b6a5dae1cf6399f0`.
None of the original captures, hashes or C outputs was rewritten.

## Other reviewed source behavior and remaining limits

The paired A6 values correctly retain the original E053→DDA7 loop: 0..4 for
both teams. The original reset/list/ball/cancellation fields represented by
the 406-word projection match the four actual parent trajectories. Actor XYZ
fractions remain carried; ball fractions are explicitly reset. The regulation
side/tip/period table selection, layout0 target side, actual actor XY ball
attachment, Z24 terminal value, mode11/owner/camera publication and negative
owner pointer reset agree with the inspected source. No broad inbound
finalizer or new-match bulk clear was substituted.

The original continuing-period caller bypasses the DA18 bulk clear. Ready09BA
and dead-ball09B0/09B2 therefore stay carried, as the C comments and comparisons
require. This source behavior must survive the repairs. The later original
pose child may raise ball Z to39; that result is outside this helper's E183
terminal boundary, not an excuse to change its Z24 output.

The standalone probe imports twelve **labeled typed native child returns**
per capture, at ten AAB2 calls and D85E/D5DB. This validates isolated parent
segments only. It does not implement the child operations, initialize normal
gameplay from snapshots, or justify ignoring unrepresented child fields.
Repeated `advance` is immutable at a boundary; terminal resume is refused.
The implementation has no production caller. Its earlier DCA6..DD97 reset,
clock/anchor work, external children, later BC9B/pose/role/sort work and frame
timing remain separate responsibilities. Native rows do not record DBR or a
full CPU/stack contract; no such proof is claimed here.

A new immutable C/header/verifier/test revision is required before acceptance.
The existing regulation native successes should remain byte-identical; all
source-only opening cases and native-domain mutations must be rerun unchanged.
