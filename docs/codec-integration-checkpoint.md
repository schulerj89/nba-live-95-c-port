# FB46 and FB30 component integration checkpoint

The original FB46 source component and separately repaired v2 verifier are
independently accepted. Root copied the reviewed source files byte for byte;
its existing strict scheduler verifier already matched. The original v1
verifier and rejection report remain historical evidence. Use
`verify_setup_codec_work_v2.py` for acceptance, not the old verifier.

Root's fresh `/W4 /WX` build in `build/codec-owner-v1` matches112814 native
instruction/register states and durations,28218 ordered CPU write positions,
all16 output payloads and all7102 previous scheduler events. The16 Python
integrity tests,12 C continuation cases and unchanged independent six-case
mutation suite pass. Reports are in `build/codec-owner-proof-v1`,
`build/codec-owner-integrity-v1.log` and
`build/codec-owner-independent-v1`. Both independent audit reports are retained
alongside `setup-codec-verifier-v2.md`.

The separately audited FB30 component is also copied byte for byte. A fresh
root build in `build/fb30-owner-v1` matches36418 native instruction/register
states and durations,9935 ordered CPU write positions and all four native
payloads. Its18 Python tests,14 C continuation cases and six independent
mutation cases pass. Static regeneration reproduces all892 source states
without capture input. Native execution covers676 of those states; the other
216 have no natural witness. Synthetic edge cases are labeled accordingly.
The original zero-count threshold, unconsumed terminator and depth-16 carry
behavior are preserved and commented. See the separate FB30 audit and source
work document for exact boundaries and remaining coverage limits.

The repaired verifier rejects the original backward-clock compensation and
mixed-row reorder attacks. These were verification bugs, not original-game
quirks. No native fixture, source API or C work model changed for the repair.
The capture runner and its two Lua inputs retain their exact audited bytes
through Git attributes: the originals contain CRLF and mixed line endings.
The revision pins must remain valid after a fresh checkout.

This is a source-specific resumable C component with typed bus operations,
not a production timing fix. It remains outside `nba95_sources.txt`. Its
diagnostic native entry registers do not generate the production caller phase.
Refresh/NMI conservation using observed native intervals does not predict the
next interrupt. Audio/SPC work, caller integration and Rules
reentry remain separate work; the original158 reentry differences remain.
