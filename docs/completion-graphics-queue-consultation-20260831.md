# Graphics-queue dependency consultation

Read-only advice from existing Max task **Diagnose native NBA95 Rules scheduler…**,
`01a05634-5316-78c0-bb36-f9cdfd3b562e`, received 2026-08-31. The user clarified
that this consultation should address the graphics-queue dependency; the
controllers sub-agent separately owns the ball/body alignment fix. Max made no
edits, builds, emulator runs, captures or settings changes. This is an attributed
source analysis and implementation contract, not accepted production closure.

Question: can a source-proved new-match/court initialization cut remove the
earlier graphics history needed by B468's DBR:$012C read, without seeding captured
queue state or assuming record five has been overwritten?

## Answer and partial cuts

There is a useful byte/cache cut, but no complete new-match ring reset or proven
bound that makes an arbitrary fresh court queue equivalent. The late court
decoder destroys the old menu value of $012C; carried cursor phase and subsequent
producer order still determine its value when gameplay reads it.

- New-match `87:8C6B` calls court setup at `8CD8`, then `85:8D3A` at `8DBD`.
  `85:8D7A` calls `C62B` on `AE:F35A`, destination `7F:2000`. Its header is
  `46 FB 00 08 06 C3 0F`: FB46, escape C3, fifteen dictionary codes C2 through B4.
  `80:BD1B` clears $0100..$01FF through BD25/BD28/BD2B/BD2E before dictionary
  writes. This resource does not change dictionary flags 2C/2D, so its actual
  execution leaves $012C/$012D zero independent of their earlier contents.
  The 52-byte metadata SHA256 is
  `460d5af15ab42d0cf9eb50d2c195f269a3c65623cd540722940776a935432eb5`.
  Execute this producer; do not install a constant at an invented boundary.
- Earlier `85:8B75` calls `80:AB7E` with A=$6000/X=0/Y=$01E0. It resets active
  allocation count $05F1, free-list/allocator roots, OAM bookkeeping and lifetime
  seed; AC0D invalidates keys $2640..$2E71. Backing records are not all erased.
  `87:AF95`/B041 separately invalidates ten jersey cache words $8E10..$8E22.
  AC1B ages/reclaims allocations rather than clearing all caches each frame.
- These cuts are separate: initial court draw `85:8D15` populates caches before
  the later decoder. A state combining the earlier zero allocation count with
  the later decoder exit would not be an actual source boundary.

## State that still carries forward

Cold boot `80:80C5` clears $0000..$1FFF, and 8134/8136 zero queue cursors $35/$37
before 8145 enables NMI. New-match `86:DA18` clears $3435..$4972 and $08FC..$0A09,
not the ring or its cursors. Scene helper `80:893F` ends with 8962/8964 copying
head $35 to tail $37 without zeroing either or erasing descriptors.

NMI sets budget $39 at 81C2 to $1518 or $16A8 according to $07F0, charges palette
and job work, and publishes head at 82E3/83CE. Exhaustion preserves the stopped
job; consumed bytes remain. The forced-blank nonempty 86DA path at 86F6..8836
instead scans local X, stops on zero or length >=4097 before dispatch, and returns
without publishing $35. It must not be replaced by head=tail or NMI budgeting.
Previously accepted source-work components cover only empty 86DA.

The ring must therefore be a view of canonical WRAM $0100..$02FF, including
decoder writes and $2180 transfers. FB30 at BE6B also uses $0100 onward as symbol
scratch via BEED/BF06. A separately allocated 512-byte publication array misses
these aliases even if all ordinary graphics appenders use that array correctly.

## Terminal writes, overlap and wrap

In the small allocation path B73E writes first length $05FF, then B7B7 writes
$0601 into the next slot unconditionally. When that length is zero, B7BC leaves
tail at this slot: the zero is a real RAM write outside the published interval.
The larger B8D2/B8D5 path checks zero before writing a second descriptor. AD67's
word bank store temporarily clears the next length's low byte before AD6D writes
$0020. Preserve store widths, order and every `(cursor + 8) & $01FF` advance.

For starting tail t, record five is reached after
`k = (($0028 - t) & $01FF) / 8` advances. Normally k+1 appended records write it;
the B7B7 terminal write may touch it without the final append. Sixty-four appends
visit every slot but do not erase phase: differing t assigns different jobs to
physical slot five. Cache hits, culling and rejection prevent inferring append
counts from actor counts, pieces or elapsed frames.

B468 has no overwrite barrier. Both the close-shot call `86:B3D5` and receiver
call AF83 reach it through gameplay gates that do not inspect queue progress.
The observed court-frame 463 call is not an earliest-possible-frame bound.

## Diagnostic chronology and its limits

Max reconstructed actual bus writes, excluding the capture's defective
convenience fields, and checked the existing raw RAM files:

| Boundary | Observed state/write |
|---|---|
| First A47A, court 0 | head=tail $01B8, $012C=0; zero allocation count, invalidated caches |
| Later court 0 | B7B7 writes $0080 to $012C |
| Court 7 | AE:F35A decoder BD25 (post-instruction callback PC BD28) writes zero |
| Court 462 | B7B7 writes zero; AD67 clears low byte; AD6D writes $0020 |
| B468 entry, court 463 | head=tail $0068; $012C=$0020 |

Diagnostic log:
`c2-receiver-integration-20260831/build/c2-native-012c-v1/alternate-writes.jsonl`,
SHA256 `658a620a56d3fccd124b17fff33fc36df814145bb2faff015eec9614610ae973`.
These values are observations, never runtime seeds or certified new corpus data.

## Implementation and verification contract

1. Root owns one game-lifetime WRAM bus. Ring records and $35/$37/$39 are byte
   views shared by codecs, scene/graphics producers, consumers and gameplay.
2. Derive carried cursors and blanking/consumer state through reached source
   helpers and transitions. Earlier pixel work may be omitted only after proving
   it is overwritten before use; append/cursor/discard effects remain necessary.
3. Starting at the court cache reset, execute reached construction, appearance
   and jersey initialization, initial A357, blanking/drain, actual 85:8D3A codecs
   and transfers, and live court/HUD/resource work. ACC2 allocation/reclaim and
   AD2B jersey decisions must derive descriptors from real resources.
4. Feed B468 the actual bus read at `87:B7E1`. A typed live predecessor permits
   bounded component checking now; production closure requires that predecessor
   to be C-derived. S1 owns scanout/master-clock claims, but cursor-affecting
   consumer boundaries cannot be replaced by an invented per-frame drain.

Natural comparison checkpoints should cover boot cursor clears; reached 893F,
86DA and 8964 with interrupt/blanking context; cache and jersey resets; first
A357 and every actual ring write/tail commit; 85:8D7A exit/8D91; NMI queue exits
including budget stops, wraps and unpublished zero terminals; both B468 call
sites and the final B7E1 read. Log every producer effect to establish cursor
provenance. Vary ordinary menu dwell/intro choices and pause/resume/new-match
routes without forcing memory. Source-boundary comparison can precede complete
absolute timing closure.

Source evidence remains in primary `.analysis/full-rom-census/listings/`:
`bank_80_instructions.tsv` (consumer and FB46), `bank_85_instructions.tsv`
(cache setup and late decoder), and the canonical ROM. Actual ROM bytes resolve
disassembler width ambiguities. No runtime integration is claimed by this note.
