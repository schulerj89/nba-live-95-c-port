# Human B pass receiver-selection checkpoint

This adds eleven new files after the frozen controller19, human-dispatch10
and switch11 checkpoints. It implements `84:DF7A` receiver selection through
the actual `86:AB2D` call boundary, plus the `85:F1C1-F228` distance result.
It deliberately does not execute the AB2D initializer or claim complete human
passing. No prior frozen file, binary, source manifest, production caller or
human-enable policy is changed. Independent review and root integration remain.

Worktree:
`C:\Users\joshs\Projects\nba-live-95-c-port\.analysis\worktrees\completion-controllers`.
All evidence below is in its private `build/human-pass` directory.

## Implemented source contract

`nba_human_pass_select` consumes the current actor, five-player context start,
context group, published controller ID090E, direction, positions, modes and
actor+8C distances. It returns either an explicit initializer continuation or
no receiver. It publishes the original0944 tag `090E|$10` even when no receiver
is selected. That bit retains the original control-acquisition veto; it is not
a request to assign the current controller to an arbitrary receiver.

Directed passing excludes the source and applies the signed wrapped CMP7/BPL
mode gate. `85:F34F` supplies direction/distance; each octant of angular
difference adds256. Equal scores replace the previous candidate. Neutral
passing excludes mode8, applies the original actor+8C preference, and uses
the distinct `85:F1C1` metric with strict score improvement.

F1C1 weights a minor axis by3/8 near the diagonal and1/4 otherwise, retaining
the original shift/add truncation,16-bit wrapping and CMP sign tests. It is
not interchangeable with the existing simple weighted-distance helper. The
new distance API returns AA's mathematical result; volatile AE/B2, CPU flags
and registers are not part of that API or its comparison claim.

Two native quirks are preserved and commented at their PCs:

- At `E0B5-E0E2`, neutral preference searches only the suffix beginning at the
  current candidate. An earlier favored player stops restricting later
  candidates once it leaves that suffix. Four natural left-route calls
  demonstrate a later less-favored candidate replacing an earlier favored
  one. Entry17 has source+8C281; native events22/27 accept distances161 then351,
  with pass scores120 then69. The other witnessed entries are76,106,138.
  `native-preference-observations.json` records these actual accepted events,
  not independently generated expected choices.
- At `E085-E088`, the routine tests whether the final score equals1600,
  rather than whether a candidate was found. A directed candidate accepted
  at exactly1600 still does not enter AB2D. The source implements this, but
  the current natural captures do not witness that boundary case.

The selected local index plus context group becomes AB2D's receiver identity
AA; the selected actor pointer becomes8E. These values are distinct inputs
and are both compared. The selection stage does not modify actors, ball
ownership, current controller assignments or receiver0946. The native BRA at
E03A skips E03C-E081; that alternative block is retained in reference listings
but is not invented as a reachable DF7A route.

## Original evidence and results

ROM: `F:\Games\SNES\NBA Live 95 (USA).sfc`, SHA256
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.
Mesen SHA256:
`d2eb03c2590c648bf329f127ebcfefd70130e7690a9e2ccdba8616faea1fe96b`.

`reference-v2/manifest.json` SHA256:
`c162559af42b8dedbf7badc14217012c261d4a1f1abb774fec92fe57236f80a9`.
It records original bank bytes, bounded recompiler output, independently
seeded Ghidra65816 listings, actual tool-source hashes, commands and logs.
DF7A's bytes through exclusive E141 hash to
`2bc624a44f1d67ef434f358c7bde3381c6fa07a23490e7f5b752ffa7dfdc5f34`.
F1C1 through exclusive F229 hashes to
`3774e5b5b071d2e8bead32614dfb306c7db2aa4dc105287dd2cc29ed95705e30`.
The direction helper's zero return and ROM map are included. Reference-v1
used a larger exploratory F1C1 bound; it remains unchanged, and version2
narrows it to the observed routine end.

Both captures use fresh private Mesen processes, portable executable/settings,
empty private saves, and explicit per-process environments. They enter via
Title and normal menus, choose left/right normally, then provide directions
and B presses for2400 court frames. No ROM, RAM or CPU state is patched; no
savestate or owner is injected. The original game chooses pass versus switch.

| Capture | DF7A selections / F1C1 calls | Compared values | Manifest SHA256 |
| --- | ---: | ---: | --- |
| `selection0-v1` |16 /26|27,370|`a0818ba5b104f9e7707d922eafc9e7bd2834c004e8a1418745722c9613bf99a3`|
| `selection2-v1` |9 /15|15,396|`be184fca0c73922d590c9f74ca7925afd8baf6a737f5cd2ad8a2973e616d14e4`|

Both final reports pass with zero mismatches or tolerances. All25 selections
reach the actual AB2D entry with matching score, receiver identity and pointer.
The41 metric calls cover all four original F1C1 return PCs: F1F3/F1FF/F21C/
F228. The left pass entry124/court1471 crosses a native frame before its
initializer boundary; this case is retained and passes, not filtered.

Each selection compares route, score, both handoff words,1408 actor/ball
words,160 controller words,128 context words and nine global words including
0944. Each metric compares its distance against the actual native helper
return. The raw snapshot is sparse:0000..00FF,0500..09FF,1600..18FF,3400..49FF,
7936 bytes. Missing raw bytes fail instead of becoming implicit zeros.

The capture also saves every actual AB2D entry, its return at84:E09C, and the
DF7A return at84:E0B4. The verifier confirms the native epilogue restores its
eight saved caller words, but these post-initializer snapshots are not passed
to C and are not counted as implemented AB2D state. They provide exact native
boundaries for the next initializer-integration step. The40 observed switch
entries are route context only; this checkpoint does not revalidate switch.

The private warning-free final probe SHA256 is
`8b0773fd910df0da327b084cd67a61c1e770ee672796167e00e12eaf71f69b14`.
Final reports are `selection0-final-v1.json` and`selection2-final-v1.json`;
the initial executable and earlier reports remain preserved. No native/C
comparison failed during this bounded implementation. That does not imply
that the untouched AB2D continuation or production human route passes.

## Evidence gate and continuation limits

The separate verifier pins executed capture/runner/helper versions, original
ROM/Mesen, exact process command/environment, selection and runner frame
limits. It requires every artifact, paired stages, exactly five candidate
scans, complete native returns, exact scalar types and complete C responses.
Private settings/save/home evidence is compared before the shared helper is
called on copied metadata. The exact initial settings recipe is also checked.
The final rejection suite rejects38 metadata and C-output mutations against
the actual unchanged right capture. It includes omitted pass/metric results;
these are verifier checks, not additional native game coverage.

The captured pass directions cover0/1/2/3/5/6/7 and neutral8; direction4 is not
a witnessed pass in these journeys. No-receiver, exact1600, pass-score ties,
extreme wrapped coordinates and multiplayer cases remain without a natural
witness. F1C1's four return paths are witnessed, but this is not exhaustive
input coverage. The directional F34F dependency retains its previous ordinary
court-domain acceptance; extreme-coordinate parity is not claimed here.

The next concrete continuation is to replay AB2D at these freshly captured
receiver-specific boundaries, then join the new selector to that verified
initializer. The existing `nba_tipoff_begin_rom_pass` covers grounded setup
and some special families but is not assumed to match every human call's
animation, attachment, resource or return effects. This checkpoint does not
use native after-state as a substitute for those missing effects.

Requester processing, the other human actions, processed marking, behavior
dispatch, physical commit, pause/free-throw/multipad lifecycle, and the normal
human-enable gate are still separate work. The frozen human10 carried-X bug
and root's copied repair are untouched. Root owns integration and commits.

Build with `tools/build_human_pass_probe.ps1`; verify with
`tools/verify_human_pass.py --capture <capture> --probe <private probe>
--rom <original ROM> --output <NEW report>`. Capture reproduction uses
`tools/capture_human_pass.py` with a NEW output directory, selection0/2,
frames400..3000 and explicit ROM/Mesen paths. The accepted captures use2400.
