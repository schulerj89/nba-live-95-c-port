# Native timeout and resume evidence

This is a bounded native contract, not a claim that every pause-menu choice is
implemented. `DumpTimeoutResume.java` reproduces the Ghidra listing and
`timeout-resume-evidence.json` pins its critical ROM bytes and two range hashes.

## Confirmed timeout path

- `$86:8300-$8334` enters pause presentation. `$87:8C66` and `$80:CF74` are
  external presentation/audio calls; their internal semantics are not inferred.
- `$86:8338-$8347` saves live state `$0936` to `$4988`, installs pause state
  `$0080`, and starts at menu index two. Game/shot clocks and `$07F6` RNG have no
  writers in the bounded pause dispatcher.
- `$86:83EC-$8408` and `$841E-$843D` navigate five entries. Each candidate tests
  byte `$4983+index`; nonzero entries are skipped. Thus a zero-timeout team must
  be disabled while availability is built, rather than allowed to reach the
  unconditional decrement at `$844E`.
- `$86:843F-$844D` accepts an action button and sends audio command nine through
  `$80:9DF3`. Index zero is TIMEOUT.
- `$86:844E-$8467` copies `$08D2` to `$4933/$4935`, decrements right `$4795`
  when nonzero, otherwise left `$4715`, then falls into the grant.
- `$86:8468-$8496` visits 24 roster records (`X=$05C0..$0000`, stride `$40`),
  performs wrapped 16-bit `+$1000`, and unsigned-clamps results at `$7FFF`.
  The existing controlled native `fixed_grant` witness covers zero, one,
  `$0FFF`, `$1000`, `$6FFE`, `$6FFF`, `$7FFF`, and `$FFFF`.
- `$86:849D` calls `$80:CF1B`, the timeout transition/fade boundary. The routine
  rebuilds presentation through `$84F6` and returns to the pause menu.

## Confirmed resume continuation

Resume is menu index four at `$86:8546`. `$86:854B` calls the same `$80:CF1B`
transition/fade boundary, waits 60 through `$80:86BF`, and rebuilds the court.
When `$7E:492B` is nonzero, `$86:8575-$857B` restores `$0936` from `$4988` and
returns. Therefore the pause state freezes normal gameplay-clock/RNG consumers;
the saved live state is restored rather than reconstructed.

`mesen_timeout_resume_capture.lua` is an isolated controlled-menu recapture for
left, right, zero-left, and zero-right cases. Current headless attempts did not
reach `$86:8300` under the automated Exhibition controller route, so no fabricated
UI capture is checked in. The exact counter/grant evidence remains confirmed by
Ghidra, ROM hashes, and the prior controlled native grant witness. The recapture
script is retained to resolve that harness-entry issue without changing gameplay.

## Deliberately unknown

Indices one through three dispatch at `$86:8498`, `$84F9`, and `$8528`, but this
evidence does not name them. Their external calls, the availability builder
(`$86:8861/$8606/$876F`), the internals of `$80:CF1B`, and the `$492B==0` index-four
branch are outside this bounded claim. They require natural UI captures before
production behavior is assigned.

Run `python tools/verify_timeout_resume_evidence.py` to check the ROM and the
existing native grant witness. Run `tools/capture_timeout_resume.ps1` only when
investigating the outstanding automated pause-entry issue.
