# Appearance verifier v2: remaining rejection

The bounded C/source acceptance in the original appearance audit remains
unchanged. Verifier v2 fixes all twelve original metadata corruptions, but
its composite is not accepted yet.

Owner freeze `build/period-appearance-freeze-v2.json`, SHA-256
`6b6dcf034a4c71690e33237cb2bde04fbfabad7f7d1b80977e2a7fa4892bacf1`,
contains 1,126 independently rehashed identities, including all 931 unchanged
v1 identities. New verifier sources were privately snapshotted. The unchanged
original twelve-case tool now rejects every case against v2.

Two remaining contracts are absent:

1. `period_capture_contract_v2.validate_rows` returns zero when no recognized
   appearance tags remain. It does not require the hook set promised by the
   enhanced capture revision. Changing every appearance tag to either an
   unknown tag or the existing `formation.entry` tag causes the capture checker
   to accept zero validated children. Both cases alter only parsed rows after
   hashes; original files remain unchanged.
2. `verify_protocol_v2.py` still reads executable/prestate/actor/exit from the
   old report rather than deriving the authoritative forty calls from validated
   native rows. The actual comparison loop accepts a one-call report (130
   words), forty repeats of the first call (5,200 words), an unattested
   executable, and prestate replaced by after-state. The last two tests inject
   unchanged successful process text to isolate command-route validation from
   output validation. This is a verifier-integrity test, not evidence that a
   physical frozen executable or capture was changed.

The six-case tool is `tools/test_period_appearance_route_audit.py`, SHA-256
`3648c618c23fafc94216eb7d00c71eb09eed8ea85059e1749d546010e23188a4`.
All six are accepted by v2; results remain under
`build/period-appearance-audit-v2/independent-routes-v1`. The original twelve
rejections are under `independent-metadata-v1` in the same directory.

A separate revision should bind expected child coverage to the actual capture
revision, derive exact command/actor/prestate/after-state records from those
validated captures, and either stop consuming the old report or verify it
against that derived record list. No C change, native fixture rewrite, wider
CPU guarantee, or production integration is requested by these findings.
