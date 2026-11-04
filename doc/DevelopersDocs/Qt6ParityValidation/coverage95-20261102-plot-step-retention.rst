95 percent campaign: solver display with bounded plot history
============================================================

Coverage remains **1,009/1,158 (87.1%)**, with 149 original requirements open
and **92 more** needed for 95%. Historical-operation coverage remains
1,816/2,367 (76.7%). This stage admits **zero whole requirements**. The full
objective and all original obligations, assertions and holds remain unchanged.

Twelve new cases pass unattended and visibly with IntegrationStepsToSkip=1 and
MaxPlotPoints=2 in both native plot consumers. They cover DifferentialCorrector,
Yukon, nested DifferentialCorrector and Yukon/DC with All, Current and None.
Each case edits both primary forms, applies the settings, checks exact Undo/Redo
and Unicode persistence, then observes the real mission's solver clone states,
raw publication grid and retained plots. Existing passing scenarios are reused
at their recorded versions, not replayed for context.

The new conjunction deliberately reaches the retention limit during a rejected
trial. The oracle processes literal selected endpoints as a bounded event
stream. It removes obsolete trial points at iteration boundaries but cannot
restore points already evicted by MaxPlotPoints. Both consumers' exact retained
epochs/order/trial flags are checked throughout execution. OrbitView Current
is checked at native flush, when its buffered trial becomes visible; GroundTrack
and other modes are also checked at each publication. OrbitView Cartesian
positions use an independent circular solution. GroundTrack Earth geography is
finite-only here; the earlier translated-origin geographic witness is separate.
No general frame, attitude, finite-burn or ephemeris claim follows from these
circles.

A failed new oracle, not a production correction
------------------------------------------------

The first new family failed in Current mode at the initial publication of its
second trial: GroundTrack was empty, while the new oracle incorrectly expected
the accepted zero-second point to reappear. The first trial had already evicted
that point under the two-point storage bound. The initial fixture filtered an
unlimited event history before truncation, thereby inventing restoration of
previously discarded history.

Only this newly introduced oracle was corrected: capacity is now applied as
literal selected events arrive, before later trial cleanup. The original
publication, cadence, solver and boundary assertion files are unchanged.
Production files are byte-identical to their stage preimages. The first failure,
its exact source-v1, raw observations and failed family receipt remain retained;
its first passing subcase is not credited as a successful family. Source-v2
contains the corrected new tests and supplies both successful full invocations.
No assertion in an original test was weakened, disabled or relabeled.

The offline audit independently reconstructs the same bounded-event contract
from literal times and recorded numerical solver states, including the actual
publication time at each observation. It rejects an altered record that restores
the evicted zero-second point. This is bounded current implementation evidence,
not an authorization to reinterpret an unresolved NASA source obligation.

Original boundary gate remains open
-----------------------------------

QtGui.NasaGroundIntegrationSkipBoundary remains registered and enabled. Its
unchanged fixture types DataCollectFrequency=2, leaving IntegrationStepsToSkip
at its legacy -1 default, then demands a two-integration-step gap across a
repeated Propagate initial publication. The preserved legacy control counts
publication samples. A new explicit integration-step capability does not
silently retire that test or resolve the legacy migration contract. Neither
the gate nor its source obligation was altered or rerun merely for context.
The exact finding and source identity are in boundary-gate-review.json.

Evidence, audit and replay
--------------------------

Evidence root::

   build/nasa-coverage/coverage95-20261102-plot-step-retention

The canonical evidence-manifest.json binds 269 retained artifacts and links the
preceding sealed live-stage audit. evidence-audit.json verifies successful
native receipts/JUnit, exact proof/log relationships, source archives, unchanged
coverage and both delivery modes. It checks 3,836 saved observation records,
8,626 epoch comparisons and 16,116 circular-position-value comparisons. These
include repeated retained snapshots, not unique physical states.

Four altered-evidence controls are rejected after updating their artifact hashes:
wrong position, resurrection of an evicted point, wrong publication time and
nonresizable-window evidence. Original artifacts remain intact. Fifteen JSON
records were losslessly compressed with roundtrip verification, saving
29,456,423 bytes (28.1 MiB). About 40.0 MiB of regular stage evidence remains,
with no PNG or ELF copies. The 81 launcher symlinks are inventoried as mutable
replay locations, not immutable loaded-image evidence.

The visible invocation passed in 75.565 seconds and records 587 native xcb
exposure events in resizable, non-fullscreen, initially Iconic windows. No
physical compositor, hardware GPU, human observation, external-plugin ABI,
all-loaded-images or complete current-build/scientific/core qualification is
claimed.

The existing QtGui.PlotIntegrationSteps registration and acceptance build targets
include the twelve cases. Select only new or affected cases, for example::

   cmake --build build/linux-gui --target GmatQtPlotIntegrationStepTests -j4
   python3 src/qtgui/tests/RunPlotIntegrationStepAcceptance.py --visible \
     --cases Retain2LiveSolverAll,Retain2LiveSolverCurrent,Retain2LiveSolverNone

Replace Solver with Optimizer, Nested or Mixed for the other contexts. Omit
--visible for private-X11 execution; choose a fresh --output directory when
specifying one. Do not edit sources during native acceptance execution.
Verify saved evidence without launching GMAT::

   python3 src/qtgui/tests/AuditPlotStepRetentionEvidence.py \
     --manifest build/nasa-coverage/coverage95-20261102-plot-step-retention/evidence-manifest.json \
     --output /tmp/plot-step-retention-audit.json

Next coverage-bearing work remains finite-burn endpoint sampling, unjournaled
ephemeris-provider behavior, coherent sampled states/accelerations/attitudes
for general spacecraft-dependent frames, toggle/absence/same-epoch/function/
repeated-run lifetimes, and exact redraw timing plus NumPointsToRedraw rendering
combined with solver iterations. The original boundary contract is also open.
FRR!E329/E330/E633/E634 therefore remain unqualified.

All 59 prerequisite holds remain; with them unresolved the conditional optimistic
ceiling is still 1,099/1,158 (94.9%). MATLAB, OEM expectation authorization,
historical OS/hardware and scientific/core approval constraints remain intact.
No commits, pushes or external publications occurred. All build/test processes
from this stage are terminal. The full goal remains active.
