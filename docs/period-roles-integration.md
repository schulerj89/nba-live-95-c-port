# Period role continuation integration

The accepted source v2 and verifier v3 are copied unchanged from scheduler
freeze `.analysis/period-roles-freeze-v3.json`, SHA256
`40f6762fa9310fa4ac83f2f8fc427e689594591952c45ce8e71bd1048f928667`.
All 1,056 frozen identities were checked before copying. The independent
source audit and final verifier acceptance are retained beside this note.
Historical checkpoint documents retain their original pending/rejected status;
the final `completion-period-roles-v3-acceptance.md` supersedes those statuses.

Root verification in `build/period-roles-integration-v3` freshly compiles three
C sources with `/W4 /WX`, without borrowed objects. All four native cases
match 892 final typed fields. The 116 controlled original-ROM cases and 13
API contracts pass, exercising 222,877 original instructions. The accepted
v3 verifier also rejects all 17 original local protocol corruptions and all
12 independent malformed output cases; the independent baseline passes.
The first source-test invocation pointed at a capture directory rather than
the four generated typed inputs and refused before running a source case;
that failed directory is retained, and the corrected run is `source-v2`.

The paired BC07 caller visits context zero, then context one. Native captures
witness the cadence early return. Rebuild and planner branches have controlled
original-ROM coverage only. Source comments preserve wrapped arithmetic,
equal-distance selection and other confirmed original behavior. Unrepresented
record aliases and unresolved assignment children stop explicitly; they are
not replaced with guessed actor indices or copied native results.

Use `build_period_roles_probe_v2.ps1 -OutputDirectory` with a fresh directory,
then `verify_period_roles_v3.py` for the four native inputs. V2's verifier is
retained as historical tooling: its first-boundary byte-width omission was
rejected. V3 enforces the declared byte widths on every emitted boundary.

The standalone source is absent from the game source manifest. This work does
not enable production period restart, establish normal branch reachability,
or claim CPU timing or whole-game parity.
