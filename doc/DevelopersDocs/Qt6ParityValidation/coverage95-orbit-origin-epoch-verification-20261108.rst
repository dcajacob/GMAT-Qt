Origin and covariance epoch repair, 2026-11-08
=============================================

The current FRR!E13 review found rounded epochs in origin-relative orbit
parameters and covariance rotation. GetCartState already preserves a precise
spacecraft or published-sample epoch, but subsequent origin subtraction/addition
and Sun-direction geometry used mCartEpoch as a Real MJD. Covariance rotation
also reduced the live spacecraft epoch to Real.

OrbitData now uses the same precise epoch selection for origin state and
position queries. Ordinary spacecraft retain their original Real/A1 overloads.
Covariance uses the live spacecraft's GmatTime when precision time is enabled;
it remains live covariance rather than acquiring a sampled Cartesian override.
The existing block-diagonal rotation formula is unchanged. No public signature,
class layout, orbital formula, physical convention or numerical bound changed.

Diagnostic and final native checks
----------------------------------

Stage: ``build/nasa-coverage/coverage95-20261108-orbit-origin-epoch``.
New ``OrbitOriginEpochTests.cpp`` and ``RunOrbitOriginEpoch.py`` register
``QtAcceptance.OrbitOriginEpoch`` with twelve isolated cases. A prescribed
linearly moving celestial origin isolates epoch dispatch. Elliptic perifocal
equations and an explicit rotation basis independently predict SMA/RMAG reads
and six-state SMA/MA writes. BetaAngle uses independent angular-momentum and
Sun-direction geometry, with the native Sun ephemeris as a shared input. A
prescribed rotating frame supplies an analytic diagonal-covariance rotation.
These fixtures qualify timestamp behavior, not physical planetary ephemerides
or a general covariance convention.

All limits were fixed before the product change: 1e-8 km for SMA/RMAG and write
positions, 1e-11 km/s for write velocities, 1e-10 degrees for MA/BetaAngle reads,
and 1e-12 for covariance entries. Exact epoch and overload checks distinguish
ordinary from precise calls. The 100 ns offset cannot be retained by Real MJD.

The first diagnostic stopped after three cases because its readiness file was
visible before its PID bytes. Only the new worker changed to atomic QSaveFile
publication; the sealed process-map helper was not edited. The second diagnostic
exposed a new witness selector collision: SMAWrite matched the MAWrite suffix.
Exact operation matching corrected that fixture routing. Earlier runs and
witness versions remain retained; neither issue changed original assertions.

The corrected pre-repair ``native-diagnostic-v3`` has six passing ordinary
controls and six precise failures. ``native-final-v4`` passes all twelve with
identical expected values and limits. It also passes all five preceding inverse
frame-parameter cases. Both registered tests completed serially; all seventeen
per-case receipts are preserved in ``native-all-results-v4.json`` because the
shared helper's root results.json contains its last registered test only.
Each native process recorded 139 loaded ELF identities.

Existing checks and failure attribution
----------------------------------------

The unchanged original ``QtGui.Covariance`` passes headless and visible modes,
including pending edits, validation, source history, Unicode reopen and its
byte-identical filter state/covariance comparison. No new GUI worker was added.

The protected ``QtGui.OriginRelativeStateContract`` fails at its published-sample
comparison. A controlled pre-repair rebuild reproduces the exact same assertion
failure. The original test forms its expected state using sampleEpoch.GetMjd();
the existing precise sampled read uses sampleEpoch as GmatTime. This reference
path mismatch and the original failure remain OPEN, without assertion changes
or a waiver. Repaired sources and every guarded runtime image were restored
byte-identically after the attribution run. No test or build overlapped an
owning-source edit. The preceding component's intermittent visible report-viewer
activation failure also remains open and was not replayed here.

All 1,334 protected original inputs/assertions remain intact. Source and binary
guards, retained preimages, diagnostic witness versions and a verified source
archive are recorded in ``results-summary-v1.json``. Builds and tests used the
private campaign runtime; installed runtimes and packages were not updated.

Campaign disposition
----------------------

The demonstrated epoch gaps are repaired. Full FRR!E13 representation/body/frame,
domain, rebinding and rollback qualification remains open. These live-epoch
diagnostics do not prove sampled-origin writes, every parameter family or any
physical covariance convention. Source dispatch presence does not establish
the full original obligation.

No new acceptance credit is claimed: 1,078/1,158 (93.1%), 80 open, 23 more needed
for the unchanged 95% target. The implementation review continues with the
remaining original groups; the goal is active.
