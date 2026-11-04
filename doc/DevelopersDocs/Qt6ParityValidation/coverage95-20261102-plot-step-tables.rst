Tabulated ephemeris providers in committed-step plots
====================================================

Nine native GUI cases pass unattended and visibly. They cover STK TimePosVel,
CCSDS-OEM and Code500. Each provider runs forward and backward in split
commands and also supplies a spacecraft alongside a numerical RK89 spacecraft
in synchronized propagation. Code500 forward uses little endian, backward
uses big endian and the synchronized case uses little endian. The previous
SPK Type 9 forward and Type 13 synchronized outputs serve as controls.

The input states follow a specified cubic in position and its first derivative
in velocity. STK and OEM use 13 nonuniform UTC knots around the starting
epoch. Code500 uses 31 knots at 20-second spacing. The offline auditor reads
the text files and independently decodes the Code500 byte order, units,
civil/DUT timestamps and end padding. It checks the six components of every
knot against the polynomial. The repeated command start does not consume a sampling step. A source setting of one skipped step selects 0, 20
and 37 seconds in the forward split commands and their negatives backward.

In the synchronized mission the ephemeris spacecraft publishes at 0, 10,
20 and 23 seconds. The numerical peer publishes at 0, 3, 6, 9, 10, 13, 16,
19, 20 and 23 seconds. The sampled plot points are 0 and 20 seconds for the
first spacecraft and 0, 6, 10, 16 and 20 for the peer. The audit checks
runtime provider type, object identity by fixture name, command-local clocks,
initial flags, every published Cartesian state and the OrbitView position
and velocity. It checks GroundTrack sample epochs and finite coordinates.
Geographic accuracy for these Earth plots, other ephemeris conventions,
rotating frames, general provider behavior and scientific qualification are
outside this receipt.

The audit verified 2,052 fixture components, 936 published Cartesian
components across 156 provider records, and 468 OrbitView components. Both
plots together supplied 156 sampled epochs. The three visible runs recorded
39 native exposures with no saved screenshots. All native invocations passed.
The offline rejection controls detect a changed numerical peer state and a
corrupted Code500 knot. Source-v1 covers the STK and OEM split commands,
source-v2 the Code500 split commands, and source-v3 the synchronized cases.
The plugin binary digest is recorded separately from the test executable.

Replay from the checkout root
-----------------------------

.. code-block:: sh

   cmake --build build/linux-gui --target GmatQtPlotIntegrationStepTests -j4
   cases=EphemStkForward,EphemStkBackward,EphemOemForward,EphemOemBackward,EphemCode500LittleForward,EphemCode500BigBackward,EphemStkMixed,EphemOemMixed,EphemCode500Mixed
   python3 src/qtgui/tests/RunPlotIntegrationStepAcceptance.py --cases "$cases" --pause-ms 0
   python3 src/qtgui/tests/RunPlotIntegrationStepAcceptance.py --cases "$cases" --visible --pause-ms 150

Use a fresh output directory for each invocation. Visible replay opens the
resizable, non-fullscreen nested desktop initially minimized. To verify the
recorded versions offline:

.. code-block:: sh

   stage=build/nasa-coverage/coverage95-20261102-plot-step-tables
   python3 "$stage"/AuditPlotStepTableEvidence.py --manifest "$stage"/evidence-manifest.json --output /tmp/plot-step-table-audit.json

Coverage remains **1,009/1,158 (87.1%)**, 149 open and **92 more needed**.
FRR!E329, FRR!E330, FRR!E633 and FRR!E634 receive zero new whole-row
credit. Reuse the original legacy boundary results, solver pixel evidence
and all inherited clauses. Historical operations remain **1,816/2,367
(76.7%)**. The 59 prerequisite holds, scientific review, technical approval
and current-build qualification remain open. This stage changes only
acceptance code and provenance capture.
