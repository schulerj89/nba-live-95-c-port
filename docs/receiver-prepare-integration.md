# Accepted receiver child and selected-parent component

The independently reviewed C2 component is copied into the integration branch
without runtime wiring. It implements the source B468 child and already-selected
AF66 parent, preserving the original address, arithmetic and RNG quirks. The
production source manifest, actor loop and human-control policy are unchanged.
This checkpoint does not complete receiver integration or the game.

All 682 identities in candidate freeze
`d357e61b21bd313e1dc5a51e1ec2b2386d453ecc727fc2031408acd953470561`
were checked before copying its 13 deliverables byte for byte. Independent audit
`596e3fbc78dbfeef6407a9f41372d855c0fb730671133060facaf338f390d311`
accepts the bounded source/native comparison and verifier checks. The original
candidate documents remain unchanged; this receipt states integration status.

Fresh root evidence is retained in ignored
`build/receiver-component-integration-v1`:

| Check | Result |
|---|---|
| Fresh eight-source MSVC /W4 /WX build | Pass; executable SHA256 `504e6a868ae78f7ed680abed3875dc8b28c3553ee64a1157c5238a04f25317b5` |
| Original child entry-to-exit replay | Eight calls, 432 checked words, zero mismatches |
| Original selected AF66 parent replay | Two calls, 112 checked words, zero mismatches |
| Controlled source-dataflow vectors | 512 cases, 27,648 checked words, all eight selectors |
| Component integrity checks | 29 corruptions refused; 42 C domain/no-mutation guards pass |
| Unchanged independent audit, rerun against root | 18 additional corruptions refused and positive replay passes |

The first controlled-test invocation omitted required `--before`; the second
supplied the multi-record input envelope where one 131,072-byte entry WRAM was
required. Both setup failures are retained, and the second was refused before C
execution. Corrected `controlled-v3` uses the frozen left `raw_00002.bin` entry
SHA256 `4a1a476ee5c8425791a59330d6c635587861f7a0a1fd35b3dce4ef09f81f8624`.
Neither production source nor expected results changed to obtain a pass.

All eight natural child calls select variant seven; other variants are controlled
source coverage. One child spans an observed NMI, but matching owned words does
not prove full interrupt or scanout equivalence. These are component tests, not
continuous gameplay acceptance. No duplicate screenshot checkpoint is needed
because this dormant component changes no rendered runtime behavior.

The live adapter must compose the actual catch gate and ordered lane inputs,
preserve inherited passer profile pointers while selecting the receiver, and
read DBR:$012C from the canonical WRAM owner. It cannot substitute receiver+$A8,
a boolean, a capture value or a fresh private queue. See
[Max's graphics-queue consultation](completion-graphics-queue-consultation-20260831.md)
for the source-derived partial initialization cuts and remaining cursor history.
That consultation refines the older candidate document's queue-container
proposal: ring bytes and cursor fields must alias the same WRAM as the codecs.

Separate startup verification is under renewed protocol review: independent QA
found terminal-register and boundary-output validation gaps inherited by newer
startup readers. Additive verifier repairs are in progress; matching C/native
states do not by themselves certify those readers. No startup source or frozen
evidence is changed in this C2 checkpoint.
