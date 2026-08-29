# Foul/stat bookkeeping differential

`$86:C493-$C4FD` is the persistent bookkeeping child invoked by the contact
foul classifier. Four controlled native calls reach it through the genuine
`$86:C4FE` classifier; ROM, PC, stack and flags are never patched. The cases
cover an ordinary first foul, a sixth foul with fewer than six active roster
members, a sixth foul with foul-outs disabled, and the complete foul-out path
with mapped game-stat increment.

All five owned outputs replay exactly through
`nba_gameplay_foul_record_bookkeeping`: persistent personal foul `+$14`, team
context active-roster count `+$54`, mapped `$879C71` game-stat record `+$26`,
foul-out state `$09CA`, and typed substitution request `$0A08`. The production
classifier and the separate owned-ball foul caller pass actor `+$16` and
committed rule `$17DF` into this same helper.

`$0A08=1` is intentionally only a request. Bench selection, active-player
replacement, appearance rebinding and substitution presentation remain in the
separate match-lifecycle implementation boundary.
