# Human pass action checkpoint independent audit

Verdict: **bounded source/native replay passes; original verifier protocol rejected pending its separate repair**. No production enabling or frozen source/evidence changes were made. This closes neither the whole initializer nor normal human gameplay.

Controller `build/human-pass-action/freeze-v1.json` SHA256 is `d1a7045c5d987845832301cf804e28f0d9d57cb35ca9a9f96e31be6cd169d181`. All 121 referenced identities across new/dependency sources, original objects, private objects, external artifacts, references and prior freezes were independently checked. Exact private source copies and results are under auditor `build/pass-action-audit-v1`. Reviewed module SHA256 is `e2c2381e47de25cfdaabf2401add8b886a29fe6e256d8b8e20dafb6a039234fc`; header is `4b5d29c8121393c79f9606166c76c42d9307b3629bcd82545362e5d1f8b81dbe`.

## Source contract and preserved quirk

Actual ROM `$86:AC50..ACA8` hashes to `91874e81af780bf1f416f18155237c32c3687278a02b7c9a3305f6be1c022e51`; `$86:B00B..B04B` to `c64a763a5cc2edb18bfed22a2c0e028f9df8e001a8b4e48c6e84adf720d80e60`. Seven original ranges, ten Ghidra/recompiler reference artifacts, 67 source-tool identities and both completed command records were checked against actual bytes. The canonical ROM identity remains `2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.

The AC50 branch uses N from the preceding relative-direction result stored at AC4C/AC4E. Values 3–5 enter the side/back path; negative or other direction values stop at AD0E. Word Z or vertical velocity being nonzero stops at ACA9. The original profile-byte/lower-state gates precede boost and stationary checks. Both distance comparisons use unsigned carry semantics; any nonzero full-word boost qualifies at distance `$119` or above. These source orders and widths are preserved.

B00B writes receiver timer `$50`, clears passer planar velocity/magnitude, sets family 5, copies coarse BE to passer `+66`, and ORs flags with 6. At distance `$F1` or above it requests upper `$2C`, then unconditionally requests `$2F`. B47A returns unchanged on equal upper state or negative upper lock. Descriptor resolution writes the complete bank word `$0084`, not just its low byte. The shared animation installer remains the frozen semantic implementation; no pose helper is invoked.

The original locked double-request behavior is preserved and commented in the module. Independent raw checks at right court 800/event 84 and court 870/event 110 show DP00 stays `$2F`, upper state stays `$2C`, lock stays `$FFFF`, and DP47/49 stay `D9DC/0084` across the rejected second request. This is a natural source quirk, not a port mismatch to normalize. The controlled equal-first-request case also passes: an already-installed `$2C` with zero lock skips the first installation and permits the later `$2F` installation.

## Fresh replay and controlled checks

Fresh private MSVC `/W4 /WX` compilation of module/probe against immutable controller objects produced `compiled/human_pass_action_probe.exe`, SHA256 `5bf49b135bc37656482395b04fb24738fb9f1508a000f41c56079f458fb41ecb`. All 64,269 native compared values pass: 25 AC50 gates, five complete B00B children and seven B47A children. Twenty gates stop at AD0E and five reach AF1D. No ACA9 or AFC4 route is naturally exercised here. The original left/right manifests remain `4aadfad8306e2b5f7e13f7352384ac6c2995a57fcd32427514a2509b36d9ff79` and `736b35a1b70a598ee027a02e1cad2376d5f2b1be4cb2e49d9672740d960a2692`.

Independent `human_pass_action_audit_probe.c` and `test_human_pass_action_source_audit.py` pass 38 literal source witnesses. They exercise relative-direction boundaries, high-word Z/vertical velocity, profile/lower gate precedence, unsigned distance thresholds, high-bit boost, moving/stationary outcomes, negative existing locks and the double-request behavior. Returning unexecuted routes must leave the typed state unchanged. Grounded outcomes must commit the exact receiver/pass fields and descriptor bank. These are controlled C source-contract checks, not additional naturally reached ROM cases.

Each native probe input is an actual entry raw path plus the immutable ROM and candidate951f assets. Expected exit data is never a C input. The probe reads the profile byte from the original ROM and the animation descriptor from the hashed asset pack. It projects the complete declared arrays, including untouched ball/controller/context/profile fields, alongside six DP words. Earlier selection, cancellation and initializer stages are checked structurally but explicitly not replayed by this component. AF1D pose/attachment, ACA9, AFC4, AD0E bodies, CPU stack/flags and unrelated volatile scratch remain unexecuted/outside this API.

## Original verification-protocol defect

The original verifier `b8c3bcbdf2a20b8eadcdf79d4c9de3a2fb834e075d91255f34b2b3e147160c7f` passes its 45 existing cases. Independent `test_human_action_protocol_audit.py` adds ten cases: seven malformed JSON/schema/vector/value cases reject, but extra error stderr, missing asset-loader stderr and forged different-pack stderr all pass. The probe deliberately redirects one known asset-loader diagnostic to stderr, yet verifier lines 300–303 only store stderr and check the exit code. This leaves that diagnostic stream entirely unchecked.

Require the exact known diagnostic tied to the actual immutable asset argument/pack and reject additional or missing text in a new verifier revision. Retain the original ten-case report under `independent-protocol/report.json`, the rejected verifier and its freeze. This is a host verification defect; source behavior, native fixtures and expected native values must remain unchanged.
