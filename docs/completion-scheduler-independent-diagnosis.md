# Independent scheduler missing-state diagnosis

Read-only review, 2026-08-31. Production repair remains **FAIL / unimplemented**.
This is an independent diagnosis, not approval of new scheduler primitives.

Sources personally inspected: original ROM; Ghidra
`.analysis/gameplay100-closure-ghidra/gameplay100_bank80_listing.txt`;
bounded recomp `.analysis/transition-ownership-20260830/phase-probe/reference/bank80.c`
(`BuildMenuBackdrop`, `BuildMenuHeader`); present `src/nba_setup_screen.c`;
raw `.analysis/setup-scheduler-20260830/native-v1/scheduler.jsonl`, its copied Lua,
manifest, and header observations. All five manifest source paths exist and
rehash exactly, including canonical ROM and Mesen. Raw scheduler SHA256 is
`df11518835e80b52d5ed807ed36af667d50a5af73cb619a7ce5c6473ebbe10e2`.
The manifest records exit0, accepted natural controller-only journey, no ROM
patch/state injection, and a private portable home; completion says four
headers. The prior report's exact caveats still apply to broad diagnostic hooks.

## Confirmed source correction

ROM `$80:8332` bytes are `C9 D6 F0 42 BD 01 01 8D 02 43 C2 20`.
The original queue path establishes M=1 at `$8252`; the negative-job branch
`$825F->$82AA->$82AE->$82A7->$8332` does not restore M=0. Correct decoding:

```
8332 CMP #$D6
8334 BEQ $8378
8336 LDA $0101,X
8339 STA $4302
833C REP #$20
```

The closure Ghidra listing's `CMP #$F0D6`, WDM and ORA are width errors. The
`$8378` branch must likewise be decoded with M=1 until its own REP. This does
not discredit independently checked branches; it prevents blindly translating
the complete NMI queue from that mixed-width listing.

## Causal check using actual native work

Each header begins with the same two fixed-source4096-byte VRAM transfers,
then14 bytes of palette DMA. Queue read/write cursors are equal throughout
the four header pre-wait paths. First Rules entry is label541/line198; repeated
Rules is1171/line172. The first DMA pair delays NMI entry until line249, so
the counter has already changed71→72 when `$86B4` loads it. Repeated header
loads71 at line224 and resumes after72. Identical logical resource arguments
and equal queue-empty state produce different wait epochs. They do not
justify a visit-number, Style, queue-empty or fixed extra-frame rule.

The headers without an intervening NMI use exactly440 observed CPU instruction
cycles entry→pre-wait. Their master durations are70,526,70,526,70,566. CPU
cycles alone omit DMA/refresh and bus timing. First header duration90,134 also
includes the intervening NMI. Its increment occurs before callback/audio work;
the blocked caller resumes only after interrupt return.

An additional independent subtraction localizes the variable work. For each
backdrop.entry→header.entry interval, subtract every chronological
`nmi.entry`→`nmi.exit` interval from the raw counters:

| Invocation | Total master | Subtracted NMI master / CPU | Remaining master / CPU |
|---|---:|---:|---:|
| First Rules |4,206,738|375,808 /57,573|3,830,930 /550,769|
| First Main return |4,264,380|433,268 /66,009|3,831,112 /550,788|
| Repeated Rules |4,184,562|353,564 /54,210|3,830,998 /550,769|
| Repeated Main return |4,181,414|350,296 /52,841|3,831,118 /550,788|

The same route has exactly equal remaining instruction-cycle count in these
two observations. Much of the different total duration belongs to variable
NMI work; the remaining master difference is still nonzero. These are
diagnostic differences, **not constants to hardcode as resource costs**.
Backdrop already enters at different native phases: first530/line258,
repeat1160/line248. It contains eleven NMI intervals here. Repairing only the
last header wait cannot recover the lost upstream phase.

## Required implementation scope and falsifiable next checks

High confidence: queue-budget plus epoch-wait primitives are necessary but
insufficient for normal-path transition parity. The portable producer needs a
carried scheduling phase that comes from actual prior translated work. The
resource generator, direct DMA, decoder and intervening NMI callbacks must
advance it with source-derived dynamic work. A bounded continuation/event
model can express this without interpreting arbitrary CPU instructions, but
feeding it captured entry timestamps remains a controlled leaf probe only.

No source-backed simpler flag or frame-offset repair was found. A narrower
production model could begin at an independently demonstrated synchronization
boundary only if that boundary's post-NMI phase is itself produced faithfully;
the trace's varying NMI exits prevent assuming a universal return scanline.
The audio callback's variable work must not be replaced by a fixed budget.

1. The scheduler owner's next capture should preserve hClock, native bank/M/X,
   all queue-record bytes at dispatch, and correct fill-return hook `$80:8B34`.
   This tests whether source decoding accounts for each executed job before
   integrating the queue primitive. `$80:8AD1` is not that return.
2. From one last common translated synchronization boundary, predict header
   entry and wait-loaded epochs for **both** first/repeat Rules and both Main
   returns. Use native entry phase only for an explicitly bounded diagnostic;
   full production acceptance requires the caller to produce it.
3. Repeat with a different natural pre-entry idle/input schedule. A schedule
   driven by actual work should predict whether the wait crosses NMI without
   visit-specific branches. Preserve the original failing journey unchanged.
4. Compare hidden VRAM publication bytes as well as RGB: the first Custom
   return's25-byte partial-clear difference is a bus-boundary failure even
   where its complete visible RGB matches.

Confidence is high for the lost-phase diagnosis and the width correction;
full exact bus-cost translation has not been performed by this review.
No production edits, timing compensation, fixture mutation or model/session
escalation was performed.
