# Period roles v3 composite acceptance

**PASS for the bounded period role continuation and repaired output schema.** The two original C modules, API, probe and native fixtures are unchanged. The separate v2 source review and its original verifier rejection remain in Git history.

The scheduler freeze `.analysis/period-roles-freeze-v3.json`, SHA-256 `40f6762fa9310fa4ac83f2f8fc427e689594591952c45ce8e71bd1048f928667`, was independently rehashed: all 1,056 identities match, including all 1,013 v2 identities. The complete verifier diff changes one guard: it now applies each field's mapped byte/word width to every output row, preserving exact integer-type requirements.

The unchanged independent 13-case tool passes against the final v3 verifier: the baseline still compares 223 final native fields, and all twelve corruptions reject. Both previously accepted first-boundary byte values, 256 and 65535, now fail at the output schema gate. The transparent test launcher only binds the tool's hardcoded v2 module import to v3; the auditor inspected that binding and did not alter the cases.

A separate four-capture replay against the auditor's fresh `/W4 /WX` executable passes all 892 final field comparisons. All eight binary-input/text-trace artifacts are byte-for-byte identical to the auditor's v2 replay. Exact identities are retained in `build/period-roles-audit-v3/preservation.json`; final rejection results are in `independent-protocol-v1`.

The independently reviewed source behavior and controlled cases remain applicable: 116 original source cases, 640 additional diverse full-word/edge cases, original RNG order, wrapped geometry and comparison behavior, and explicit unresolved record/assignment boundaries are preserved. No source change was needed for this schema repair.

| Object | SHA-256 |
| --- | --- |
| v3 verifier | `d9279be39e4c75592d4672cd93221de24b711db50e37d48397e2438e15026eaa` |
| Unchanged independent protocol tool | `2dfac685d98d119d840f4b47eb3e607baf1f5f164cde155c8e78194008715f19` |
| Fresh private executable | `5c9d6cb1d799e23aa98c841800c079c944c51f916364bf8654762a235e60d7fe` |

Native evidence covers the two early returns and final `$E1F7` typed state only. Intermediate FIRST_RETURN value parity and planner/rebuild branches are supported by controlled original-ROM tests, not native intermediate snapshots or normal reachability. Acceptance excludes arbitrary record aliases, unresolved BAE4 and related children, general live-play BC07, CPU/stack/interrupt residue, timing, whole-period integration and human play.
