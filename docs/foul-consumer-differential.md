# Pending-foul consumer differential

Four controlled cases run the original ROM from the real `$85:93F5` entry to
its `$945E` return. They cover the busy-whistle and empty early exits, a
defensive event with a negative presentation timer, and an offensive event
with an already-busy presentation plus the short-timer flag. No ROM, program
counter, flags or stack values are patched.

All 13 owned outputs match `nba_gameplay_foul_consume_pending`: event and
latched-event words, inbound readiness, contact/presentation state, shooting
foul, global and side event bits, whistle, signed timer, visual gate and both
whistle-state mirrors. The accepted cases naturally execute nested
`$87:BACB`; negative-timer queueing and nonnegative-timer gate clearing are
therefore verified as part of the same calls. Runtime wiring remains guarded
by the CPU gameplay/debug telemetry tests.
