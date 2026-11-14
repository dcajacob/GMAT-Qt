Solver redraw during committed-step plotting
============================================

The native Qt cases exercise DifferentialCorrector, Yukon, nested Target and
Target inside Optimize, each in All, Current and None modes. OrbitView and
GroundTrack use a 100-second integrator step, skip one step, retain six points
and redraw two recent segments. The test applies the settings through the
primary editors and saves, reopens and rebuilds the script. The real solver
clone states identify the trial and accepted passes.

At the Report breakpoint in the first trial, All contains the regular points at
200 and 400 seconds and the trial arc at 600, 800, 1000 and 1200 seconds.
The regular and trial arcs are disconnected. None retains the regular arc at
0, 200 and 400 seconds. In Current the ground display already contains the
trial. OrbitView still contains the regular arc until the one-iteration For
block in the mission issues a native publisher flush. Both viewers then
contain the trial arc. At mission completion all modes retain the accepted
arc at 1000, 1200, 1400, 1600, 1800 and 2000 seconds. The completed view
draws all five segments.

Each check captures the production framebuffer and compares it with a
same-size PlotCanvas that uses the literal retained epochs, break flags and
recent segment count. It keeps every vertex for camera fitting. Each positive
comparison is pixel-identical. Each tested incorrect segment count changes at
least five pixels. The audit cross-checks the hashes, difference counts,
solver states, chronological coordinates, native return codes, JUnit results
and visible-window policy. Only framebuffer hashes and numerical differences
are retained.

The comparisons exercise the software X11 renderer at the sampled debugger,
flush and end-of-mission boundaries. They do not instrument the timing of
every repaint, distinguish all later overlapping trial geometries, or establish
independent renderer or host-GPU accuracy. Earlier per-publication membership,
bounded eviction, regular display cadence, legacy boundary, function lifetime
and spacecraft-origin receipts remain versioned evidence.

Reproduction
------------

Run from the checkout root and use a fresh output directory for each invocation.

.. code-block:: sh

   cmake --build build/linux-gui --target GmatQtPlotIntegrationStepTests -j4
   cases=PaintSolverAll,PaintSolverCurrent,PaintSolverNone,PaintOptimizerAll,PaintOptimizerCurrent,PaintOptimizerNone
   cases=$cases,PaintNestedAll,PaintNestedCurrent,PaintNestedNone,PaintMixedAll,PaintMixedCurrent,PaintMixedNone
   python3 src/qtgui/tests/RunPlotIntegrationStepAcceptance.py --cases "$cases" --pause-ms 0
   python3 src/qtgui/tests/RunPlotIntegrationStepAcceptance.py --cases "$cases" --visible --pause-ms 150

The visible runner opens an initially minimized, resizable, non-fullscreen
nested desktop. For an offline verification from the checkout root:

.. code-block:: sh

   stage=build/nasa-coverage/coverage95-20261102-plot-step-solver-paint
   python3 "$stage"/AuditPlotStepSolverPaintEvidence.py --manifest "$stage"/evidence-manifest.json --output /tmp/plot-step-solver-paint-audit.json

Evidence and coverage
---------------------

The versioned stage contains the source preimages and exact changes, the
three failed invocations and their explanations, and the source-v4 native
receipts. Across both runs, 112 production frames matched the literal predictions,
304 incorrect alternatives were distinguished, and 600 epoch, trial and
connection triples were checked. The visible trace recorded 1,037 native
exposure events with zero saved screenshots. All six regular mode/case
comparisons matched the earlier trajectories. No production file changed in this increment. Retain prior source
versions for regression evidence. The three affected regular redraw cases
must match their sealed trajectory outputs and pass the existing segment
comparison in both modes.

FRR!E329, FRR!E330, FRR!E633 and FRR!E634 each retain their original
fingerprint, scope and inherited clauses. The latest delta links the preceding
four-row map. Remaining work includes coherent sampled rotating frames,
acceleration and attitude, additional providers and their data lifetimes,
and solver behavior beyond the recorded render boundaries. The cumulative
ledger remains **1,009/1,158 (87.1%)**, 149 open and 92 more needed. The
historical-operation ledger remains **1,816/2,367 (76.7%)**. Technical approval,
scientific review, current-build qualification and the 59 prerequisite holds
remain open.
