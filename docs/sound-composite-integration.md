# Accepted isolated sound source components

The CPU sound prefix, CPU sound initializer and uploaded SPC resident slices
are independently accepted within their declared boundaries. The prefix and
initializer use verifier revision2; the resident uses revision3. Original
rejected verifiers, their evidence and the independent rejection reports remain
unchanged. Revisions strengthen evidence validation without changing C source,
native fixtures or original arithmetic and memory quirks.

Root rehashed45,45 and50 referenced identities, copied52 new files byte-for-byte,
and verified six existing dependencies. All three probes were rebuilt with
MSVC /W4 /WX. Twelve fresh verification calls pass: the complete differentials,
original local contracts, parsed mutations, unchanged independent protocol
tests and seven fetch-contract checks. The unchanged independent suites reject
9/9 prefix,9/9 initializer and24/24 resident invalid cases. Source comparisons
cover3,673 prefix instruction states/2,392 accesses,7,055 initializer states/
2,450 accesses and182 resident states/175 attributed accesses. All51 CPU WRAM
endpoints match;1,264 uploaded bytes have original ROM provenance.

This integration adds independently callable source components and their
reviewable tests. It does not wire them into the production scheduler or change
the existing `nba_spc.c` and `nba_audio.c`. The CPU slices stop before unresolved
SPC reads or sequencer work; resident slices stop before unresolved timer/DSP
reads. Snapshot prestates remain isolated differential inputs. Normal startup,
cross-clock input visibility, timer/DSP ownership, phase parity and whole-game
audio completion are still unresolved.

Copy identities and commands are under `build/sound-composite-integration-v1`.
The four copied independent reports describe the accepted scope, preserved
source quirks and earlier verifier failures.
