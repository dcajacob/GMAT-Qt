Finite-burn reports between propagation commands
================================================

Evidence is stored in
``build/nasa-coverage/complete18-20261112-stm-finite-reports-v1``.

The native Report route reproduced a loss of thrust acceleration after
Propagate removed its transient force. The burn was still active, but
owner-dependent STM frame calculations used coast acceleration.

BeginFiniteBurn and EndFiniteBurn now maintain a private, lifetime-checked
registry for acceleration queries. Active finite-thrust providers participate
alongside the bound numerical model. A provider already present in that model
is counted once. The normal propagation force-list lifecycle is retained.

Two targeted missions verify powered, coast, immediate restart without another
propagation, and second-shutdown reports. The original powered STM reference
is preserved through those transitions. Independent frame differentials
check 1,296 native report entries and 90 direct post-propagation parameter
reads. The missions also retain 90 live-propagation reads. All checks use the
original numerical bounds.

The existing TotalAcceleration1 function-binding test passes three missions,
twelve function calls and 48 Begin/End commands. Original test inputs and the
failed pre-fix report evidence are preserved. The stage verifier replays saved
data without executing native missions.

Mass-depleting finite burns and the wider force, coordinate and augmented-state
conjunction remain open. Canonical coverage is 1,113/1,158, with 12/18 agreed
rows complete.
