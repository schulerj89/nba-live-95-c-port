# Period render tail v2 composite acceptance

**PASS for the bounded owned-data component and repaired verifier.** The original C/header/probe/native inputs are unchanged. This acceptance supplements `completion-period-render-tail-independent-audit.md` (SHA-256 `b144d06298ded876597b46885958ad3589a886a81c9e398170213c5c6c235481`); its original verifier rejection remains valid and retained.

The final owner freeze `build/period-render-tail-freeze-v2.json`, SHA-256 `b9362eab95705e0b0c4fcab5a8f6ce85b846de9f32a28a1dfc90dcdc79e90e96`, was independently rehashed: all 2,100 identities match, including all 2,082 original identities. Results and the exact new verifier snapshot are in auditor `build/period-render-tail-audit-v2`.

The new verifier calls the already accepted `verify_period_restart_v2.read_native`, which validates complete hook sequence identity, consecutive indices, metadata domains, isolation, original raw state bindings and CPU input preconditions. It additionally requires exactly one `roles.after`, exactly one `formation.return`, and adjacent tail indices. The observed frame/court crossings remain explicit; no durations were assigned to C.

The auditor reran the unchanged ten-case full-loop protocol tool against the final frozen verifier. The baseline makes all four independently checked canonical C calls. All nine malformed traces reject before C, including the four duplicate/order cases previously accepted. A separate complete verifier run passes all 12,496 native bytes, eight malformed-output guards and nine invalid-domain refusals. The fresh private source build, 384 controlled original-ROM cases and direct typed-API guards are documented in the preceding source audit and remain applicable because their source identities did not change.

| Object | SHA-256 |
| --- | --- |
| New `verify_strict_v2.py` | `dc7340c28d24a691f7684f6cd279f708a2f08bb392e0772156bdf62d37b07fae` |
| Accepted native reader | `68d22789ecaff106b9b2c773a821a5a3510c3a984dd5d8aeac6e61b03c6f2eca` |
| Unchanged independent protocol tool | `aed2a51e0830426fb9fdf7adc82486de164baf4174f97dc7887ad4ebb1b7b6db` |

Acceptance covers collision/actor-depth/draw-list/counter data only, under the documented canonical pointer, zero-sentinel and binary16/DP0 constraints. It does not accept CPU/register/DP residue, native interrupt or video timing, whole-period integration, role-planner reachability, or human play. The wrapped comparisons, arithmetic shifts, equal-key ordering and overflow behavior remain the original source behavior.
