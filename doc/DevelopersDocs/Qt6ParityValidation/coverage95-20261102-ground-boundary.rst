GroundTrack collection across a repeated Propagate start
======================================================

The original QtGui.NasaGroundIntegrationSkipBoundary gate passes unattended
and visibly. Its source, assertions and registration are intact. The archived
failed run selected publication three, a second copy of the same 10-second
state. The final runs delivered all 63 ordinary publications and collected 31
points at 0, 20, …, 600 seconds. Publication three stayed in the raw stream and
did not advance GroundTrack's collection counter. The original test types a
collection frequency of two and leaves IntegrationStepsToSkip at its default.

The compatibility decision is confined to decimated regular GroundTrack data.
Propagate identifies the command start and the runtime objects in a synchronous
Publisher context bound to the exact packet pointer and count. This context is
available with or without a numerical-step subscriber. GroundTrack regards a
command start as already delivered only when the preceding packet has identical
values, label order, direction, runtime objects and clocks. It includes runtime
formation members in that comparison. A new run, copy, sampling setting,
activation or solver boundary invalidates the comparison as appropriate.
Storage is bounded to one previous packet per decimating GroundTrack instance.

Frequency one continues to collect every packet. A changed spacecraft state
at the same epoch, reversed direction, a distinct runtime object with the same
name, and an ordinary repeated packet remain eligible. Solver trial packets
keep their publication cadence. The explicit integration-step setting uses its
existing committed endpoint path. Numerical stepping and stopping algorithms
are outside this correction. The five core and subscriber preimages and their
exact diffs are in ``core-change-audit.json``.

Verification
------------

The first corrected original gate passed unattended at source-v1. Source-v2
added defensive identity checks, invalidation on run-state changes, and new
controls. Its first control invocation failed because the new local-publisher
fixture registered its stream before subscribing an observer. Source-v3 fixes
the fixture setup and checks its stream ID. The earlier failed invocation and
the original failed boundary receipt remain recorded as failures.

At source-v3, the unchanged gate and eight unchanged regressions passed in both
modes. These cover the original OrbitView/GroundTrack publication-cadence test,
dense and sparse GroundTrack symbol cases, PlotSetup, and DC, Yukon, nested and
mixed solver publication. Each original cadence trace still produced 61 packets
and the required distinct OrbitView and GroundTrack selection phases at every
ordinal. The audit also checked eight previously sealed single-command streams
for their historical collection periods. Those comparisons reuse their original
source versions. Their withdrawn requirement admission stays withdrawn.

Three new cases passed in both modes. A changed state at 10 seconds yielded
GroundTrack points at 0, 10 and 30 seconds. Frequency one retained both 10-second
publications. Reversal at 10 seconds produced GroundTrack points at 0, 10 and
−10 seconds. The protocol checks also distinguish runtime objects sharing a
name, plain nested publications reusing a pointer, pointer/count mismatches,
solver state transitions, ordinary repeats and restoration of the outer scope.

The independent audit checks the literal 63-packet grid, every collection
decision, and all 378 raw Cartesian components per original invocation against
a circular solution. The maximum position error was 7.28e-12 km. Across the
three controls in both modes it checked 44 epochs, 60 position components,
12 geographic components and 16 protocol scenarios. The visible desktop
receipts recorded 239 native X11 exposure events. Six altered copies were
rejected for wrong selection, missing ordinary data, corrupted velocity,
omission of an unthinned duplicate, or incorrect object/repeat classification.

Evidence and replay
-------------------

``build/nasa-coverage/coverage95-20261102-ground-boundary`` contains the
449-artifact manifest, stage receipt, evidence audit, failure history, and
``four-row-evidence.json``. That map preserves the original workbook IDs,
text, fingerprints, inherited children, bounded source-versioned receipts and
outstanding clauses. Lossless compression saved 60,838,968 bytes. Approximately
52 MiB of regular stage and invocation artifacts are retained, with zero PNG
or runtime ELF copies. The inventory lists 594 mutable launcher links. Use
fresh output directories for replay. Omitting the visible options runs the
unattended route.

.. code-block:: bash

   python3 src/qtgui/tests/RunAcceptance.py --build-dir build/linux-gui --filter '^QtGui.NasaGroundIntegrationSkipBoundary$' --nasa-review build/nasa-coverage/coverage95-20261102-ground-boundary/runtime-review-v3.json --max-screenshots 0 --visible --display-route nested --display-scale auto --start-minimized --pause-ms 150 --output build/gui-acceptance/ground-boundary-local-replay
   python3 src/qtgui/tests/RunPlotIntegrationStepAcceptance.py --cases LegacyChangedState,LegacyEveryPacket,LegacyReverseBoundary --visible --pause-ms 150 --output build/gui-acceptance/ground-boundary-controls-local-replay
   python3 src/qtgui/tests/AuditGroundBoundaryCompatibility.py --manifest build/nasa-coverage/coverage95-20261102-ground-boundary/evidence-manifest.json --output /tmp/ground-boundary-audit.json

FRR!E634 has now passed the enabled original boundary gate. FRR!E633 inherits
that result. Both remain open, as do FRR!E330 and FRR!E329. Remaining clauses
include general sampled frames and coeval spacecraft references, acceleration
and attitude, function/provider lifetimes, other ephemeris and force cases,
and solver redraw/retention/rendering conjunctions. The full objective and 59
prerequisite holds continue. Coverage is **1,009/1,158 (87.1%)**, 149 open,
**92 more needed**. Historical operations have a separate count of 1,816/2,367.
Zero whole rows were admitted. Scientific/core and current-build qualification
remain open.
