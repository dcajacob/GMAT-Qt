85 percent campaign: live solver-iteration drawing
===================================================

Requirements coverage is **958/1,158 (82.7%)**: twelve new whole source rows,
34 since the sealed 924-row baseline. **27 more are needed** to reach at least
985. The goal remains active. Historical coverage is unchanged at
**1,816/2,367 (76.7%)**. All **200 remaining requirements and 551 historical
cases** remain exported with their original text, fingerprints and closure gates.

New live result checks
-----------------------

Four native GUI cases check differential correction, Yukon optimization,
nested differential correction and Yukon with an inner differential corrector.
Each uses one actual Publisher stream to drive nine simultaneous viewers:
OrbitView, GroundTrack and XYPlot, each in All, Current and None mode. This
checks 36 viewer/mode/context combinations using four missions.

The four retained proofs contain **195 raw publications**: DC18, Yukon45,
nested42 and mixed90. Every streaming publication is checked for exact ordered
membership; every native completed-pass flush checks all nine viewers. Current
must replace earlier trials while preserving accepted data. None must exclude
trials. All must retain the entire unfiltered stream.

Accepted-pass classification comes from the **actual numerical solver clones**:
every enclosing solver must be FINISHED. The test does not use the production
plot-filter classification as its oracle. Nested cases explicitly exercise
inner-FINISHED passes that are still outer-loop trials: four in nested targeting
and twelve in the mixed hierarchy. There is exactly one globally accepted pass
per mission. Known scalar goals, the quadratic optimum and every raw trajectory
segment are checked, including independent Newton/RK4 recalculation by the
evidence auditor.

Actual embedded resource controls are operated through keyboard input, with
pending/native isolation, owning-panel Cancel, Apply, exact Undo/Redo and
Unicode save/reopen assertions. All source/history/file bytes must survive Run.
The original solver report/render/replay/lifecycle proofs are reused at their
recorded source versions. The old mixed-solver source also received its missing
unattended execution, including unchanged stop/recovery assertions.

Exact new rows are ``FRR!E334/E335/E336``, ``E349/E350/E351`` and
``E638/E639/E640``, together with parents ``E333/E348/E637``. Both original
execution cohorts and every eleven-clause parent obligation remain mandatory.
The composed auditor rechecks the original legacy evidence, original assertion
fields, four live contexts and the newly supplied unattended clause before
admitting any row. No partial live case independently awards coverage.

Two Qt receiver repairs
-------------------------

The new test demonstrated that Current OrbitView lost its completed trial at
flush: three ordinary points remained where six points were required. The
receiver refreshed immediately after clearing the just-drawn trial. Native
Current subscribers without arc context could also replay accepted samples as
trials. ``QtPlotReceiver.cpp/.hpp`` now request the existing arc metadata for
Current, retain completed buffered trial replay until its successor arrives,
and then remove the old trial before adding new data.

The delay is confined to an actual buffered native trial replay. Explicit
clearing, camera reset, availability handling and LastN retain their prior
immediate clearing behavior. A targeted CameraReset regression caught an
initially overbroad delay; its original assertion stayed intact and now passes.
SharedPlayback and OrbitDataAvailability also pass. One affected DC live case
was repeated after the final callback guard changed and passes in10.004seconds.
The earlier Yukon/nested/mixed passes remain source-versioned evidence, with no
claim that they ran against the later complete binary.

**No engine source changed in this increment.** All101 previously changed core
files remain byte-identical and pending independent technical review. The two
Qt production files have separate before/after hashes and patches in
``build/nasa-coverage/coverage85-20261101/publication-GUI-change-review.json``.
They also remain subject to independent review.

Failures, replay and storage
-----------------------------

Four failed execution attempts remain recorded: two fixture attempts using a
hidden legacy setup button, the actual Current-display defect, and the explicit
camera-clear regression. The production failure's raw trace is retained even
though a later paused-callback exposure error initially masked its exception;
the test now preserves its first exception. A missing QDialog include failed
one build, and a composed-review/runtime-review mixup failed one preflight
before any execution. All have resolved dispositions; none earned credit.

No unrelated passing GUI case was rerun. Replays were confined to the changed
clear/reset method and the failed or affected live case. New visible windows
started minimized, remained resizable and were not fullscreen. All runs retain
zero screenshots and zero runtime binary copies.

Source text is preserved through verified deltas against an existing source
archive. Bulky run-local planning/coverage summaries are losslessly compressed;
actual receipts, logs, JUnit, raw publications, native solver states, reports,
qualifications and cumulative coverage remain unchanged. The retention manifest
records original and compressed hashes. Ten coherently rehashed corrupt
compositions are rejected without running the GUI or engine.

Reusable execution and audit
-----------------------------

From the checkout root, watch just these cases with::

   cmake --build build/linux-gui --target GmatQtNasaSolverPublicationTests -j4
   python src/qtgui/tests/RunAcceptance.py --build-dir build/linux-gui --visible --start-minimized --pause-ms 300 --max-screenshots 0 --filter '^QtGui.NativeNasaSolverPublication' --nasa-review src/qtgui/tests/NasaSolverPublicationTraceability.json

Restore the nested test desktop to watch it; resize it as desired. Ordinary
CTest execution uses the same assertions through the native/Xvfb runner::

   ctest --test-dir build/linux-gui -R '^QtGui.NativeNasaSolverPublication' --output-on-failure

``NasaSolverPublicationTraceability.json`` deliberately grants no standalone
rows. ``NasaSolverPublicationComposition.json`` is the separate post-execution
review. ``AuditSolverPublicationAcceptance.py`` and
``BuildNasaCoverageExtension.py`` admit its complete source-versioned
qualification; auditing existing receipts does not execute tests again.

Limits retained
-----------------

The full remaining backlog is in
``build/nasa-coverage/coverage85-20261101/coverage.json``. Further work includes
finite-burn polynomial integration, body/frame/state families, remaining missing
unattended conjunctions and API/console contracts. Legacy, absent, conflicting
and unfinished requirements remain in the denominator.

Ground membership uses a shared native frame rotation with a separate ellipsoid
calculation; this does not independently qualify FK5, Earth orientation or
ephemerides. Arbitrary solver objectives, scales, nesting, estimators and third
party solvers remain outside these selected contexts. Full current-build,
physical input/host-GPU/cross-platform, scientific/core and full NASA
qualification remain unachieved.
