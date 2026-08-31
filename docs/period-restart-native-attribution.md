# Original period restart state and retained readiness

The former CPU test exposes a real port defect at frame49412: its period
restart invokes the generic dead-ball parent without rebuilding formation,
publishing a new inbounder/target, or installing the owner. The original
regulation restart performs those operations. Clearing every stale latch is
not an appropriate repair: original `$09BA` readiness survives this restart.

Ten fresh captures use a private Mesen executable/settings/save directory,
normal cold boot and released menu/controller inputs. At a naturally owned
live-ball `$85:EDC6` clock call, they explicitly seed only period, one remaining
clock tick, scores, the dispatch-busy word and the horn bit. These are controlled
expiry cases, not naturally elapsed full games. No formation, inbound target,
owner or readiness is injected. Complete128KiB before/after snapshots prove
that only the six declared seed words can differ.

The first Q1 case starts with readiness0. Nine later/expanded cases wait for
the original game to produce readiness1 before applying the same expiry seed.
Together they cover raw periods0,1,2,3 advancing to1,2,3,4 and336 complete WRAM
boundaries. In all cases readiness is retained through `$86:E207`. Expanded
captures observe all ten `$87:AAB2` appearance children separately and the
assignment/cancellation boundaries. The final overtime capture additionally
observes `$86:E1AC`, before its later controller/geometry children.

The ROM caller explains the retained flag. `$87:9797` jumps to `$878C86`, then
`$878CA6` calls `$86:DCA6`. It bypasses the new-match `$86:DA18` bulk clear at
`$DA3F–DA47`. The restart does clear transfer `$09B8` through `$E0B4 → A625`.
`$E0FC/E0FF/E102` establish the inbound side, layout0 and call `$85:C37D` to
replace actor/target. `$E165/E176/E17C` establish owner, actor mode11 and camera
side. `$E17F → $87:A9D0` publishes the owner pointer. C37D does not clear `$09BA`.
The native flag carry is preserved as an original quirk without claiming the
original authors intended it. Existing generic foul-finalizer clears must not
be imported into this distinct caller.

All ten cases have the naturally determined tip winner5. Other winner/anchor
combinations require source-contract or separately controlled coverage; these
captures do not claim to witness them. Appearance, animation, camera and
hardware timing remain distinct child responsibilities. No C parity verdict
or production repair is claimed by this capture-only checkpoint.

Original earlier lifecycle evidence stops at `$DD47` and remains unchanged.
Capture scripts, manifests, settings, byte checks and raw states are under
`build/period-restart-attribution-v1`; the completed corpus is frozen separately
from subsequent implementation and review work.
