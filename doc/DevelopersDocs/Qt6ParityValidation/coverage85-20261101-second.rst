85 percent requirements campaign: intermediate redraw cadence
==============================================================

Requirements coverage is **930/1,158 (80.3%)**: six new rows in this campaign,
including the newly closed **FRR!E331 and FRR!E635**. Historical coverage remains
**1,816/2,367 (76.7%)**. The 85-percent goal remains active: at least 985 verified
requirement keys are needed, so **55 more remain**. All original denominators,
assertions, unqualified source rows and inherited qualification gates are retained.

Actual intermediate updates
-----------------------------

``QtGui.NativeNasaPlotCadence`` edits the actual resource forms through keyboard
input: collection frequency2, OrbitView update frequency3 and GroundTrack update
frequency4. It asserts pending source/native isolation, Apply, exact Undo/Redo,
Unicode save/reopen and the resulting native settings. It then executes one
600-second, fixed ten-second-step, point-mass circular-orbit mission.

A test-only subscriber is appended to the real native Publisher. It does not
replace the production plot receiver or subscribers, and unsubscribes on exit.
It independently counts all 61 publications, verifies their literal ten-second
time grid, and checks every raw Cartesian state against the circular analytic
solution. The final raw report separately checks time, all six components and
sentinel41; source/history/file bytes must remain unchanged.

An opt-in observation at the end of the actual ``PlotWidget::refresh`` method
records completed renderer-update requests. Every regular publication must have
exactly the expected refresh history, with no early draw:

* OrbitView collected frames 3,6,9,12,15,18,21,24,27,30: ten updates.
* GroundTrack collected frames 4,8,12,16,20,24,28: seven updates.

The complete trace covers all interval residues and more than five cycles for
each viewer. Eight initial/block-flush/completion refresh observations are
classified separately. Actual native FlushData/SetEndOfRun callbacks identify
forced boundary draws; these cannot supply intermediate-cadence credit.

The new case passed visibly in **11.411 seconds**, with zero screenshots. Its
outer test desktop starts minimized, remains resizable and is not fullscreen.
Restore it to watch; this remains Qt event/software-rendering evidence, with
physical input/GPU/platform qualification separate. No old passing GUI case was
replayed for this increment.

The native backends differ
---------------------------

The factory creates ``OrbitView`` for the orbit plot and the newer
``GroundTrack`` class for GroundTrackPlot, rather than the older GroundTrackPlot
implementation. The actual runtime classes are explicitly checked. With factor2,
OrbitView retains the initial publication and even ordinals; GroundTrack retains
the initial publication and every second subsequent publication. Independent
publisher counters apply these separately reviewed collection contracts.

The first two development attempts exposed erroneous shared-initial-phase test
oracles. The second attempt's raw diagnostics showed OrbitView frame2 versus
GroundTrack frame1 at publication2, leading to the factory/source review. Both
failed versions, diagnostics, log/JUnit and exact source deltas are retained.
The strict every-N-collected-point redraw comparison was not relaxed; the fixture
now uses the actual distinct backends. End-of-block flushing was also made
explicit rather than assuming that the end-of-run flag identifies every forced
draw. No collection algorithm or renderer scheduling change was made to fit the
test. Disputed historical steps-to-skip versus collection-period rows remain
unqualified; these two cadence rows do not settle that separate source conflict.

One GUI-only instrumentation change
------------------------------------

``PlotWidget.cpp`` adds a seven-line opt-in read-only observation block after its
existing render update. An observer explicitly sets the dynamic property
``gmatObserveRefresh`` on a widget. Only then are its refresh serial and source
frame recorded. The test checks that the default path adds no observation
properties and changes no collected data before enabling the observer.

No header, object-layout, virtual interface, collection decision, render-update
schedule or engine source changes. Removing this single block restores the
entire previous source byte-for-byte. The exact patch and before/after identities
are in ``cadence-GUI-observation.patch`` and ``cadence-GUI-change-review.json``.
All 100 existing core changes remain separately unapproved. This observation is
not a new user-facing control or a bypass of the actual rendering path.

Evidence reuse and limits
--------------------------

The new two-row qualification is checked by the existing extension admission
verifier, which reconstructs the exact source version from compressed text and
rechecks the actual native result/log/JUnit, executable and before/after input
identities, visible window policy and full assertion conjunction. The previous
four rows are reused from their own frozen passing versions. The first coverage
snapshot is retained as ``first-coverage.json`` when the canonical union advances.
No old receipt is rewritten to today's source or binary.

The failed-development source records reuse matching earlier archived text and
store only changed source members. Runtime/observer binaries are identified by
hash and are not copied. Lossless compression of the two failed summary sets
saves **15,264,524 bytes**; their raw failure receipts, logs, JUnit, traces and
sample diagnostics remain. No passing screenshots are stored.

The exact union and all **228 requirement / 551 historical gaps** are in
``build/nasa-coverage/coverage85-20261101/coverage.json``. Every inherited source
obligation/qualification hold remains visible. Full-current-build, full-NASA,
independent scientific and technical-core approval are false. This increment
closes the originally named six plotting gaps; it does not complete the85percent
goal or qualify arbitrary solver iteration membership, scientific domains,
optional interfaces or current private NASA authority.

Reproduction
--------------

Reuse all six new rows without replaying GUI cases::

   python3 src/qtgui/tests/BuildNasaCoverageExtension.py \
     --baseline build/nasa-coverage/coverage85-20261101/sealed-baseline.json \
     --qualification build/nasa-coverage/coverage85-20261101/axis-qualification.json \
     --qualification build/nasa-coverage/coverage85-20261101/color-qualification.json \
     --qualification build/nasa-coverage/coverage85-20261101/cadence-qualification.json \
     --output /tmp/gmat-coverage85-check.json

To deliberately watch this new case on a matching source/build version, use a
fresh output directory::

   python3 src/qtgui/tests/RunAcceptance.py --build-dir build/linux-gui \
     --filter '^QtGui[.]NativeNasaPlotCadence$' \
     --nasa-review src/qtgui/tests/NasaPlotCadenceTraceability.json \
     --visible --start-minimized --max-screenshots 0 --output /tmp/cadence-watch

The same registration is available unattended through CTest. Native arbitrary
host/GPU behavior remains separate. ``second-increment-audit.json`` records the
conjunctive proof, source/assertion preservation and all explicit limits.
