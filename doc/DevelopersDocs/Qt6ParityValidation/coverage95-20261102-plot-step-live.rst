95 percent campaign: live plot membership and redraw retention
==============================================================

Coverage remains **1,009/1,158 (87.1%)**, with 149 original requirement instances
open and **92 more** needed for 95%. Historical-operation coverage remains
1,816/2,367 (76.7%). This continuation adds **zero whole-row credit**. The full
objective, original denominator, source fingerprints and qualification gates
remain unchanged; the goal is active.

New evidence and demonstrated correction
----------------------------------------

Twelve additional live cases exercise explicit IntegrationStepsToSkip=1 with
DifferentialCorrector, Yukon, nested DifferentialCorrector and Yukon/DC, each
in All, Current and None modes. All twelve pass unattended and visibly. The
native observer uses literal fixture grids and actual solver clone states,
not the production integration journal or sampler, to derive expected retained
membership. It checks publications and native flushes throughout execution,
including rejected trials and the accepted pass. OrbitView Current buffers
trial data until native flush; its assertion is at that boundary. GroundTrack
and the other modes are also checked at each publication. Both plots' epochs,
order and trial flags are checked. OrbitView positions are independently
checked against circular motion; GroundTrack geography remains finite-only
in these Earth cases. The prior translated-origin geographic proof is reused
at its own recorded source version.

The additional CadenceRetention case exercises typed primary-form settings,
Apply, exact Undo/Redo and Unicode save/reload. Its split propagation commands
advance 10 then 590 seconds with ten-second primitive steps and one skipped
step between collected points. Both consumers must collect exactly 31 points
on the literal 0,20,...,600 second grid, retain the latest ten, and redraw at
every third (OrbitView) or fourth (GroundTrack) collected point. It checks
retention at every raw publication and actual completed widget refreshes,
separately classifying command-block flushes and end-of-run refreshes.

The first execution exposed a production defect: GroundTrack's forced refresh
at the split command boundary reset its pending redraw counter. At 60 seconds,
collected point four lacked its scheduled GroundTrack redraw. The native
consumer now requests RefreshPreservingCadence for block flushes only when
explicit integration-step sampling is enabled. QtPlotReceiver displays that
forced refresh while preserving the pending count. Legacy Refresh behavior is
unchanged. This modifies GroundTrack.cpp and QtPlotReceiver.cpp; exact preimages
and patches are retained. No numerical integrator, force model or solver
algorithm changed in this continuation.

The unchanged assertion then passed unattended and visibly. Each execution
observes all 61 distinct raw grid positions, the duplicate command seed, every
retention boundary and 17 regular redraws across the two consumers. The latest
ten points are exactly 420,440,...,600 seconds. This proves the regular split
arc conjunction, not every solver/retention/redraw combination.

Source-version and failure boundaries
-------------------------------------

The twelve solver cases ran at source-v2, before the redraw correction. The
passing cadence case ran at source-v4 after that correction. These are bounded
source-versioned witnesses, not an assertion that all cases ran on one final
build. Existing 22 consumer passes and five unchanged original regression
passes remain valid at their sealed versions and were not replayed for context.
The original NasaGroundIntegrationSkipBoundary gate remains enabled, unchanged
and unqualified; the new setting does not silently retire its failure.

Two failures are preserved: build-v1 lacked the fixture's explicit QDialog
include, and cadence-headless-v1 exposed the redraw-counter defect. Failed
sources, compiler/native logs, failed receipt, exact edits and successful
rechecks are retained under::

   build/nasa-coverage/coverage95-20261102-plot-step-live

The authoritative manifest/audit are ``evidence-manifest-v2.json`` and
``evidence-audit-v2.json``. V1 records the original
uncompressed artifact hashes and is preserved as superseded by the lossless
compression binding. Thirty-six JSON records were compressed with byte-for-byte
roundtrip checks, saving 76,968,852 bytes (73.4 MiB). About 36.0 MiB of regular
new-stage evidence remains. There are no retained PNG or ELF copies. The 162
runtime launcher symlinks are inventoried as live replay locations; their target
bytes are not hashed as immutable retained evidence.

AuditPlotStepLiveEvidence.py binds native receipts, JUnit, proof/log equality,
source archives, the preceding sealed audit and unchanged canonical ledger.
It checks 3,836 saved live observation records and 124 cadence publication
records across unattended/visible modes, including 54,898 epoch and 106,410
circular-position-value comparisons. These counts include repeated retained
snapshots, not distinct physical states. The offline membership audit checks
legal phase prefixes and actual values; the native assertions additionally
check exact publication/flush timing. Five coherent altered-evidence controls
are rejected for position, trial membership, redraw, retention and window
policy after updating the altered artifact hashes. Original evidence is intact.

The visible receipts record 592 native xcb exposure events, initially Iconic,
resizable, non-fullscreen windows and zero screenshots. They do not establish
human observation, physical compositor/hardware GPU, all loaded runtime images,
external-plugin ABI or full current-build/scientific/core qualification.

Replay and next coverage-bearing work
------------------------------------

The existing QtGui.PlotIntegrationSteps registration and acceptance build
targets include the new cases. Build and select only affected cases::

   cmake --build build/linux-gui --target GmatQtPlotIntegrationStepTests -j4
   python3 src/qtgui/tests/RunPlotIntegrationStepAcceptance.py --visible \
     --cases LiveSolverAll,LiveSolverCurrent,LiveSolverNone
   python3 src/qtgui/tests/RunPlotIntegrationStepAcceptance.py --visible \
     --cases CadenceRetention

Other live names use LiveOptimizer, LiveNested or LiveMixed followed by
All, Current or None. Omit --visible for private-X11 execution. Use a fresh
--output directory when specifying one. Do not edit sources during a native
acceptance invocation. Audit the saved evidence without running GMAT::

   python3 src/qtgui/tests/AuditPlotStepLiveEvidence.py \
     --manifest build/nasa-coverage/coverage95-20261102-plot-step-live/evidence-manifest-v2.json \
     --output /tmp/plot-step-live-audit.json

FRR!E329/E330/E633/E634 remain open. Next work must resolve the original legacy
boundary gate and conjoin solver redraw/retention combinations, finite-burn
endpoints, unjournaled ephemeris-provider behavior, general spacecraft-dependent
frames and sampled acceleration/attitude, coherent cross-provider epochs,
toggling/absence/same-epoch assignments and repeated-run/function lifetimes.
The preceding report describes these limitations; none is waived here.

All 149 original gaps and 59 prerequisite holds remain. With those 59 unresolved,
the conditional optimistic ceiling is still 1,099/1,158 (94.9%). MATLAB, OEM
expectation authorization, historical OS/hardware and scientific/core approval
constraints remain unchanged. No commits, pushes or external publication were
performed. No build/test handle remains live at this checkpoint.
