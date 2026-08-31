# Tipoff C image baseline independent audit

Verdict: **PASS for the three C RGB expectations at frames90/170/220**. The refresh does not accept the captured score panel as correct, native whole-frame parity, normal human play or a completed game.

Root `build/tipoff-image-freeze-v1.json` SHA256 `ef2e075a3a6fc2265ffe33787883da942c52c888178e244918e22da9ca0e4a68` has 70 independently verified identities. The referenced gameplay85 source freeze was already independently checked and rebuilt. The primary/current executable hashes in `captures.json` were also checked against the actual files. The test's Python AST differs only in three expected strings; all asset, schema, court-catalog, debug-state, handoff and selected-home assertions remain identical. Original tests/images remain preserved.

The auditor freshly compiled both CLI entry files against its own fully rebuilt, matched 52c2899 object sets. The two source sets differ only by exact historical caa9134 versus canonical 52c2899 `nba_tipoff.c`; headers, other source inputs and candidate951f pack are held constant. Old-source output equals every primary pixel at all three frames. Canonical-source output equals every current pixel at all three frames:

| Frame | Historical RGB SHA256 | Canonical RGB SHA256 |
| --- | --- | --- |
| 90 | `39d3e6bf4d77fc58280d3e604e03063e82a621124a5d42d3c7f2a1e818b05ae8` | `176c32401147f2222163c745e7e5555fd1604722230d6ad533ebf0bfb89f7b97` |
| 170 | `da83d08b002c789af5b214b02e38d8ee6d1673d87e2d3753c3bb26a0634ff43d` | `95a0a41026b80983219d57bd2c4b310dfa9f72d32299ff9580fcc4bc7aea2764` |
| 220 | `85e8824265afa48bd41005c4d397b0db3e74482a21e11ef96ecfca82453e4e6d` | `97f3f4ff835de26b8ba76e3c3f44c6e224999690301c91a30d77efd05f4eb326` |

This isolates the previously accepted canonical team-context/profile/resource and assignment-rank source change. The original stores at `$86:DA8D..DAAB` and `$86:D789..D7B7` were independently reviewed before these hash changes. Later controller/input, inbound and startup fixture changes are unnecessary to reproduce the images. No asset payload or original ROM expected output was changed.

The auditor also reran the entire frozen `test_tipoff.py` against the hash-verified current v15 executable with a separate output context: PASS, including all original asset digests/schema/court checks, frame phases/debug fields, 5330-frame introduction handoff and 5900-frame selected-home23 journey. Its source-presence checks used the exact separately accepted tipoff source snapshot. This validates the unchanged test contract rather than only the replacement image strings.

Visual inspection of old/current90 and current220 confirms changed player jerseys/identities and later composition. Both still display the captured WEST/ORLANDO score/clock panel. That known stale HUD is explicitly an unresolved port consumer, not an original-game bug to preserve or a newly accepted match-state display. It is not silently fixed or legitimized by these C screenshot goldens.

Auditor `build/tipoff-image-audit-v1` retains the freeze, AST/source contract, fresh CLI builds, six screenshots/logs, replay report and complete test log. No production source, shared build output or native fixture was modified during this review.
