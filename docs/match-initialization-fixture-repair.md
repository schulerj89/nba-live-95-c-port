# Selected-team-independent boosted-pass self-test

The port's startup self-test rejected every match with Philadelphia as home.
Its synthetic boosted-pass scenario searched the selected home roster for a
player with original profile `+$3E >= $55` and returned failure if none existed.
Philadelphia's twelve original values are67,72,70,72,77,75,75,60,72,60,62,60.
This roster is valid; the precondition belongs to the synthetic test. It is a
port defect, not an original-game bug or a reason to alter player ratings.

The original-ROM table at `$84:E640`, team19 pointer and twelve roster-relative
offsets independently reproduce those values. The report
`build/closure-migration-v1/initialization-comparison-v1.json` records every
original record address and rating, with canonical ROM SHA256
`2115c39f0580ce19885b5459ad708eaa80cc80fabfe5a9325ec2280a5bcd7870`.

`cpu_boosted_pass_self_test` now copies the caller's session locally and selects
the known Orlando test roster only in that copy. It continues to load actual
asset ratings, find a qualifying player and execute every existing boosted
initialization, animation, apex and release assertion. No gameplay branch,
original rating, actual chosen team, human gate or native fixture changes.
The source comment explains why this test setup must be independent of match
selection. No fake rating or successful early-return bypass is introduced.

The new `match_initialization_probe` initializes all29x29 team combinations,
rotating through all four quarter settings, and requires the canonical native
team order, configured regulation clock and byte-identical caller session.
The normal build-Test gate now runs it. Optional complete owned snapshots
exclude only leading host pointers and require a matching compiler/layout.

Fresh private old/current builds show:

- Old:812 combinations initialize; exactly29 with home19 fail at the boosted
  pass self-test. The unchanged earlier three gameplay85 test pairs were not
  sufficient to detect this team-specific startup error.
- New:all841 combinations initialize with every session check passing.
- Every byte of the812 previously successful initialized owned game states
  is unchanged. Only the29 formerly rejected matchups become available.

Private executables, matched headers/objects, probes, logs and state streams
are retained under `build/closure-migration-v1/initialization-matrix-*-v1`.
The older29-home diagnostic records all348 pass profiles. Full-suite v14
reached this failure through real Team Select choices (away28/home19), after
the closure driver's release-frame repair. Its log is retained.

The corrected closure now reaches6000 gameplay frames twice and all semantic
guards; its separate C digest still requires attribution and review. This
initialization fix is not approval to refresh that digest. Independent audit
of this startup-fixture checkpoint remains pending.
