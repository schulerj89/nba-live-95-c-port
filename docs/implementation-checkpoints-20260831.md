# Active implementation checkpoints

The user requested execution, delegated fixes, screenshot progress and regular
commit/push checkpoints on 2026-08-31. Implementation begins from `7002fb1`.
This record tracks work actually started, distinct from the completion plan.

| Workstream | Agent/owner | Fresh worktree | Current responsibility |
|---|---|---|---|
| C1 | Controllers | `c1-pass-interruption-20260831` | Attribute and fix the real contact/pass failure, then independently verify runtime behavior |
| S1 | Scheduler | `s1-bootstrap-20260831` | Implement original reset/IPL upload into a persistent CPU/SPC owner and the honest resident DSP stop |
| QA/screenshots | Independent auditor | `checkpoint-qa-20260831` | Inspect baseline/current-port screenshots and independently review each candidate; no production edits |
| Integration/M0 | Root | `completion-owner` | Original modes/SRAM route inventory, shared interface review, accepted integration, commits and pushes |

The three agent slots are allocated to two implementers and one independent
reviewer who also owns screenshots. Screenshot work is a bounded assignment at
each checkpoint, not a separate fourth agent or an unattended scheduled monitor.
Root can review already captured images while the auditor checks source evidence.

## Checkpoint policy

1. Preserve a clean source/build baseline and the existing failing witness.
2. Implement a bounded fix through its real caller. Retain exact source, pack,
   build command and test results, including failures and native evidence limits.
3. Independently audit the candidate and rerun affected integration gates in a
   fresh root build. Do not combine unrelated unreviewed patches into that result.
4. Commit and push the accepted integration source; verify the remote commit.
5. Rebuild that checkpoint for the screenshot agent, inspect its actual output,
   and publish a new dated gallery only when capture and visual checks succeed.
   Link the exact source commit and retain previous galleries. Commit a small
   documentation receipt afterward if needed; a documentation-only commit does
   not require another identical source capture.

If a code candidate fails review, keep it in its private worktree and retain
the failure. A separately labeled WIP checkpoint may preserve source, but does
not promote it to accepted integration. Main and the desktop executable remain
unchanged until release review. ROMs, packs, builds and screenshots stay ignored.

The stable local gallery is primary
`.analysis/progress-screenshots/latest/index.html`. The starting baseline is now
published as `20260831T182933.314971Z` from clean commit7002fb1:13captures, fresh
40-source build and109source/header identities, with all13PNG/BMP pairs equal
and all13views unchanged from the previous gallery. Each was visually inspected;
the clock moves11:32to11:31 and the panel clears at2660. Manifest SHA256 is
`d2e7acb11ad0b483b8b4c994abeb6ccf4ba1b73e565f403b8e29acb6afad8758`;
executable SHA256 is `731003ac8c3a19b0ad058109c0d7c921dcfbc9959ab5007ea255b579e30e18ff`.
The ignored capture helper now refuses publication without QA tied to the exact
manifest, images and logs; four negative publication guards pass. Prior captures
and their script copies are unchanged. The agent has switched to independent
C1 source review. Native emulator
captures are serialized through root and use isolated configurations/save homes;
the screenshot pipeline uses the native C renderer and no emulator.

## Findings at implementation start

The C1 implementer identified an original owner-contact branch that can skip
ball drop, while the current port appears to drop unconditionally. Source/native
attribution and independent review are in progress; this is not yet an accepted fix.

The S1 implementer and root corrected a planning-only mnemonic error: SPC03DB
is `MOV $F3,A` (`C4 F3`), not `OR A,$F3`. Its read-before-write bus cycle still
needs the hardware owner. Frozen source was already correct and remains unchanged.
