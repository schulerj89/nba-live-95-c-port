# Human B-pass initialization through the first AC50

This checkpoint implements the next bounded child of the frozen DF7A pass
selector. It stops at the **first `$86:AC50`**, before the native pass action
branch. It does not close AB2D, release a pass, enable ordinary human gameplay,
change controller ownership, or modify the controller19, human10, switch11, or
pass11 freezes. The existing scene initializer remains unchanged and is not
used as the oracle for this new stage.

`nba_human_pass_init_prefix` implements `$86:AB2D-AB82`, calling the existing
complete `$87:B538-B554` animation cancellation helper. It publishes the
passer's descriptor pointer words from `$7E:3449`, caller identities into
`$0942/$0946`, active flag `$09C4`, conditional inbound `$09B8`, and receiver
mode 10/timer `$28`. It cancels any nonzero upper lock, including a negative
lock, without resolving a pose or changing the lower channel. The source
reads C2 separately from actor+00; the API preserves that distinction.

`nba_human_pass_init_geometry` implements `$86:AB83-AC4F`, using signed
velocity shifts of four bits for the passer and three for the receiver,
wrapped integer positions, and the existing complete `$85:F3C3` fine direction
child. It publishes distance `$09DA`, passer band `+62`, and the explicit
predicted X/Y input plus fine/coarse/distance/band/relative continuation words. It preserves N-based
distance comparisons at ABFE-AC25 and the AC40 `$FFFF` relative direction for
a coincident endpoint. Prefix mutations occur before geometry and there is
no early rejection of an airborne actor or coincident target. These exceptional
cases are source-derived; they were not witnessed in these captures.

`nba_human_pass_prepare` is a real bounded C chain from the frozen DF7A selector
through these two new stages. It preserves DF7A's controller tag and returns
without initializer changes on NO_RECEIVER. The result still says
CONTINUE_INITIALIZER because execution must resume at AC50. Its only current
caller is the private probe; it is deliberately absent from the production
source manifest and normal gameplay dispatch.

Fresh original execution is in private directories
`build/human-pass-initializer/selection0-v2` and `selection2-v2`. Both run the
unchanged ROM through Title, Setup, Player Select and 2400 court frames with
ordinary B/direction input. Each process has a private Mesen executable,
settings, Lua home and initially empty save directory; the runner records the
exact command, environment, actual post-settings hash and save hashes. There
are no emulator state or ROM writes in the capture script. Left uses native
selection 0, right selection 2; their fresh selection state is `[2,1,1,1,1]`.

| Native evidence | Left | Right |
| --- | ---: | ---: |
| Original DF7A / AB2D calls | 16 | 9 |
| Cancel, prefix, fine, geometry, combined-chain checks, each | 16 | 9 |
| DF7A F1C1 distance calls also retained | 26 | 15 |
| Compared values | 138538 | 77928 |
| Upper cancellation lock 0 / lock 1 | 15 / 1 | 7 / 2 |
| Live / inbound initializers | 14 / 2 | 7 / 2 |

All **216466 values pass**. The C probe projects every field in the typed
initializer state, including unchanged lower channels and descriptor tables,
then compares the complete actor/ball, controller, context and selected global
word vectors. It separately compares prefix and geometry continuation words.
Every C replay receives entry state only. Neither native child after-state nor
recorded expected results are supplied to the C module. The combined chain
starts at DF7A entry, not at a substituted native receiver decision. Left entry
166 to ready 183 spans court frames 1471-1472 and passes unchanged.

Both routes witness only grounded actors, noncoincident endpoints, locks 0/1
and bands 0/6/12/18/24. There is no NO_RECEIVER, negative lock, band30, extreme
wrapped-distance, airborne or AC50-revisit witness. No controlled case is
counted as native coverage. Upper cancellation has three actual lock1
witnesses. The inherited DF7A suffix-search quirk remains in the unchanged
pass module with its original PC/native-observation comments. Separately, the
independent switch audit found an extreme-input **port defect** in the frozen
F34F target-direction dependency used by DF7A (signed operand comparisons
instead of the original wrapped subtraction sign). Root is repairing that
dependency elsewhere; it is not changed here. This checkpoint establishes
the captured natural input domain, not arbitrary16-bit DF7A direction cases.
The new geometry child uses the separate F3C3 implementation and its wrapped
comparisons. No earlier switch checkpoint is claimed accepted.

The strict verifier requires both native initialization boundaries, every
pass/cancel/prefix/fine/geometry/chain stage, exact JSON types and complete C
responses. It checks all artifact identities, supported script/runner versions,
the command/route/environment, private paths, and the actual initial/post
settings, observed Lua home and saves before passing a **copy** to the shared
isolation verifier. `mutation-final-v4/report.json` records 69 rejected metadata
or C-response mutations, including missing stages, boolean/float words,
16-bit metadata overflow, raw/event contradictions and clock offsets. The
verifier binds the supported route clock anchors (Player2697; court4590 left
or4390 right), checks every subsequent frame/court relationship, and requires
raw-backed metadata to agree with the captured words. These checks incorporate
the independent switch-verifier audit findings received before this freeze;
no earlier switch verifier is represented as accepted.

The earliest initializer closure limit is AC50 itself. Still unexecuted are
the side/back and off-axis branches, AB2D's catch-preinit/RNG/profile decisions,
`$85:F473` aligned selector, `$86:AF66/$B468` children, action installation,
`$87:AEC3/$B649` pose/attachment, mode15/live-state commit, and AF4D-AF65 stack
restoration. CPU registers, flags, cycle timing, transient stack bytes and
unlisted DP scratch words are not modeled by this checkpoint. Observed native
AC50-to-resume state is retained but not replayed. Do not join the new stage
to the old initializer, which would repeat prefix changes and retain later
translation gaps.

The preserved initial failure investigation used the 25 saved pass11 AB2D
entry/resume pairs. The unchanged historical 35-word probe has nine mismatches.
A new private adapter correcting home/right and visitor/left input mapping,
active roster order and animation prestate still has the same nine. The first
is left court270, passer9 to receiver7: old C produces upper `$2F`/family1,
native upper `$2A`/family0. The actual new capture has fine4, relative0,
distance110 at first AC50 and the same later native `$2A`/0. The unchanged C
aligned branch uses a conservative fallback instead of the original AE52
selector. This is an outstanding port continuation gap, not an original game
bug. Other mismatches involve `+66` and upper poses; the old adapter is still
partial and does not establish their exact cause. Both adapters, inputs,
stdout, reports and binaries remain private immutable investigation artifacts.

The first new capture attempt (`selection*-v1`) failed Lua compilation because
a generated edit joined `false` and `end`; no boundary file was created. The
failed processes were stopped and their source, logs, manifests and diagnosis
retained. Reference-v1 mistakenly labeled bytes F453-F472 as the direction
table; reference-v2 corrects the actual long-load target `$85:F16F-F18E`.
The first build warned about a missing arithmetic-shift declaration; the
final build includes the proper header and is warning-free. Earlier reports
and probe executables are retained under their original hashes.

Original identities and source references:

- ROM `F:\Games\SNES\NBA Live 95 (USA).sfc`, SHA256
  `2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
- Mesen SHA256
  `d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b`.
- Fresh ROM/Ghidra/recomp `reference-v2/manifest.json`, SHA256
  `c7af9211f507239db4d68393491799be5e8ea014c71804b1630c674e8724f445`.
  It includes original AB2D, F3BB/F3C3, B538, the later F473 source, exact bytes,
  Ghidra/recompiler tool-source hashes and completed commands. F473 is reference
  material only, not claimed implemented here.
- Left capture manifest
  `c7ea373d787a62dcd006a44dc2c8829364e2af8c186dc73c091e8b28decee3d5`;
  right `ca8de05503580f01c9461955502d8e7f915c32d2cc782d902988f20dad5a6a64`.
- Final probe `human_pass_init_probe.exe`, SHA256
  `74a7dae3d5b395cb53375d8b0a6be7cb8031a53b6cb1f7af66f6ab224c078709`.

Run `tools/build_human_pass_init_probe.ps1` to build only private outputs.
It recompiles the frozen DF7A dependency into this private directory and links
the unchanged, hash-bound production dependency objects. Run
`tools/verify_human_pass_init.py --capture <selection*-v2> --probe
<human_pass_init_probe.exe> --rom <original ROM> --output <new report>`; existing
reports are never overwritten. Run `tools/test_human_pass_init_evidence.py`
with those arguments and a new output directory for rejection checks.

`build/human-pass-initializer/freeze-v1.json` binds the eleven new files,
dependency sources/objects, capture/reference identities, final reports,
failure evidence and patch. Root owns subsequent independent acceptance,
integration and any production enablement. No commit or push was made here.
