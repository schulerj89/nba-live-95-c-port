# Independent period composition review

Accepted **the bounded DD97→E207 owned-data composition with verifier v2**. The original v1 verifier is rejected for incomplete build-manifest attestation; its C/source/native comparisons pass. No production code, original capture, frozen packet, or root file was changed by this review.

Reviewed owner packet `build/period-composition-freeze-v1.json` SHA-256 `075660a6bca1e2aac08683bd0f6b5f0267e8d9b016db8ebde3a09e5227e5c32c` and repaired packet `build/period-composition-freeze-v2.json` SHA-256 `c187b92ab3a393899fa6a4b31f42b69bd6e811378fa2de38d330665aebd1548b`. All 2,674 original identities matched both before and after review. All 2,925 v2 identities matched; v2 retains every original identity unchanged. The accepted verifier hash is `f559a7a2e71a0d1b06e1c4c7555e660d8dbf3a1c418a57f67d302d33d347a208`.

## Independent execution

Private evidence is in this worktree's `build/period-composition-independent-audit-v1`. `prepare_build.py` copied identity-bound source/header inputs and compiled **44 C sources** with `/W4 /WX /O2 /MD`, using no old objects. This includes the source equivalents of all 37 objects linked by the original diagnostic. Fresh executable SHA-256: `c8f6f15eff5edc9f849c9e41db5ea696b9f1d0e5f0e432bb0edc55216e8bf6a5`.

`replay.py` ran each of the four original native DD97 before-states in a private working directory with NBA95 environment variables removed. It compared every expected boundary against the independently read native capture. **125 boundaries / 504,500 owned bytes pass** (32, 32, 32, 29 boundaries). Every complete 131,072-byte output also matches the corresponding original frozen C output, including bytes outside the accepted native projection. This latter equality is reproducibility evidence, not native ownership of excluded bytes.

The native input reader and source-domain guard remain the accepted frozen implementations; this review independently rebuilt the C composition and tested its composition/closure, rather than claiming a new independent native capture parser. No Mesen capture or process was launched.

## Concrete v1 defect and v2 repair

V1 iterated only the `sources` and `objects` keys offered by its manifest. The unchanged full verifier accepted nine invalid manifests: empty sources, empty objects, both empty, omission of each of four sampled source/header files, omission of `nba_controller.obj`, and an extra top-level field. These cases still ran the real executable and passed native comparisons. This is an attestation defect, not evidence of a C/native divergence. Wrong executable hash and Boolean compiler exit were already rejected.

`integrity.py` retains those original rejection findings. `integrity_v2.py` uses the **same literal mutation definitions**, changing only verifier/manifest/output paths. V2 rejects all eleven invalid manifests and accepts the unchanged case. It independently fixes the complete four-key schema, exact 14 source/header keys, all 37 object keys derived from a pinned original source-list hash, and a full recheck of the pinned original freeze. Its route-validation function ASTs are identical to v1. Both versions ignore forged prior reports: the interception log records zero old-report reads, while each successful verifier execution performs all 125 comparisons plus the original 27 protocol and eight domain refusals.

## Source, adapters, and before-only closure

`source_review.py` regenerated the adapter text from the frozen template and child probes; it is identical to `driver.c`. Original ROM SHA-256 is `2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`. `bridge-source.txt` records the original E17F→E207 instructions, decoded directly from that ROM.

The parent loads canonical actors at 34EB + slot×100, context0 at 46EB and context1 at 476B, and the ball at 3EEB. The parent state is saved before each boundary and reloaded from the current C-produced raw after each child. Thus the ten appearance calls run in original pair order 0,5,1,6,…,4,9; their channel/RNG/owner effects survive subsequent parent operations. Assignment precedes object sorting, cancellation, target/owner setup, and attachment. No child-return file is read by the C driver.

Original E1AC/E1B2 copy the current ball integer coordinates into 0918/091A. E1B8–E1E1 transfer actors 0/5 only for period0 or ≥4, using contexts 46EB/476B and groups 0/5. E1E5/E1EE call BC07 for context0 then context1; E1F7/E1FB call D5DB then 80:FBFF; E1FF/E204 increment 084A/C. The composition follows this order. The role adapter preserves the source distinction between actor assignments, controller assignments, context pointers, and 0910 ball-pointer geometry. Its original wrapped subtraction tests are retained. The natural role cadence is 12→10→8; rebuild/planner branches remain refused.

`routes.py` independently checks the fresh executable and unchanged validation functions:

- Short/long raw inputs and an extra child-poststate argument exit 2 before producing outputs.
- Substituting the actual DFCF child after-state as the sole initial input fails the owned-actor comparison; substituting E183 or E207 after-states exits 3. These are controlled anti-seeding tests, not natural routes.
- Five controlled markers (09BA, 09B0, 09B2, actor0 fractional X, and unowned 1200) survive all 32 boundaries: 160 checks. No full child-return snapshot replaces current data. The unowned marker checks continuity only.
- All 56 independent malformed trace/raw/diagnostic cases reject, including duplicate JSON keys, scalar rows, reordered boundaries, missing/extra fields, noninteger/negative/overflow identifiers, missing/forged/extra asset diagnostics, raw-length/type violations, and both endpoints of all eleven owned regions.

## Limits

Acceptance covers the four frozen native routes and their explicitly owned data projection from DD97 through E207. It excludes DCA6→DD97, CPU registers/DP/stack/interrupt timing, role rebuild/planning, UI/phase behavior, human enablement, root game wiring, and whole-game correctness. The independent entry-prefix and human-launch packets were left unchanged; their local source/artifact/prior-freeze identities were also rechecked.

The twelfth draw object is carried input here. This review does not assign new basket-position semantics or claim that ordinary live rendering uses the period-tail full sorting routine. Integrating that state into `NbaTipoff` requires the separate source/caller work identified by the parent. No original quirk was normalized or repaired by this review.
