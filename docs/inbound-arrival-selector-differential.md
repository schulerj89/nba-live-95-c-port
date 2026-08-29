# CPU inbound arrival and selector differential

This checkpoint compares the CPU-only continuation of the native inbound
routine with the production C helpers. The source capture contains 500
natural calls at `$86:F43A`: 111 motion calls, 387 arrived/no-launch calls and
two genuine `$86:F653` launches. No program counter, ROM byte, stack or flags
were patched.

`tests/fixtures/inbound-arrival-witnesses.json` retains all 387 native arrived
outputs. The replay compares the owned `$0968/$09F6` ball state, actor flags
and velocities, `$09BA/$09B6/$0964` readiness/event latches, `$09B8` transfer
latch and actor draw direction. It found two production omissions: the host
set `$0968` without the native `$09F6=2` attachment, and did not clear `$09B8`
when signed `$0946` was negative. Both now live in
`nba_gameplay_inbound_arrival_prepare` and all 387 calls match exactly.

`tests/fixtures/inbound-selector-witnesses.json` retains the two natural CPU
launches. The production selector agrees on both receiver actors. Unit guards
also preserve selector order, rejected-self/passive candidates, the exact
timer 60/59 fallback boundary and both baseline-side gates. The 63,800-frame
CPU regression proves that the helper is bound to live inbound arrival,
transfer and resumed play on both teams.

Coverage deliberately excludes the optional human-input branch
`$86:F520-$F54E` and native alternate-selector/fallback instructions
`$86:F5D2-$F60A`, which were not executed by this natural capture. The C
behavior is guarded, but those addresses need controlled native cases before
they receive ledger credit.
