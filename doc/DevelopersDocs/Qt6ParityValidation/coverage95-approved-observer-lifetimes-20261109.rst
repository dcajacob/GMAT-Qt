Approved acceptance-observer lifetime corrections
================================================

The user explicitly approved the prepared two-file lifetime patch following
the interrupted-thread recovery. The exact reviewed patch is now applied to
SharedPlaybackTests.cpp and PlotIntegrationStepTests.cpp. Original source
copies, before/after hashes and the approval scope are archived in
build/nasa-coverage/coverage95-20261109-approved-observer-lifetimes-v1.

SharedPlaybackTests retains the model owner and uses a const-map iterator for
the final XY point. It no longer binds a point reference into a temporary
PlotCurve copy. The expected (39, 1521) assertion and retained-history-length
assertions remain; an additional guard requires the curve and sample to exist.

PlotIntegrationStepTests reacquires the current ResourceEditor from its
subwindow after Apply and deferred event processing, then checks that the
current panel is clean. It no longer calls hasChanges on the retired editor.
The typed-value, source isolation, Undo/Redo and native-value assertions remain.
No unrelated original test, numerical tolerance or registered timeout changed.

Targeted build and first verification
------------------------------------

Both edited test executables were compiled from their actual owning source
files and relinked into a new private stage against the existing complete
ordinary and ASan runtime builds. All other engine source hashes match those
runtimes' recorded source baseline. This was a targeted test rebuild, not a
new aggregate application build. The earlier CMake flag-preservation correction
retains its separate four-configuration validation.

SharedPlayback and SharedPlaybackButtons pass in both profiles: four passing
runs. The ordinary PlotIntegrationSteps run emits 95 passing case proofs and
then exits 1 at the unchanged ``Origin message history changed during mission``
assertion. That separate observer premise conflicts with the Message Window's
10,000-block limit, as established during the recovery diagnosis.

The first ASan plot run, concurrent with the ordinary profile, emits 91 passing
case proofs without an ASan report but reaches its unchanged 300-second limit.
It is recorded as a timeout, not a pass. The repeat selects only ASan
PlotIntegrationSteps and retains the same executable, inputs, assertions and
timeout. This standalone repeat emits 95 passing case proofs without an ASan
report, then exits 1 at the same unchanged history assertion as the ordinary
run. Its independent receipt is kept alongside the first timeout. Both
lifetime corrections are verified through those cases; neither parent result
is promoted to a full pass.

Evidence and acceptance boundary
--------------------------------

The historical 1,334-input manifest is not rewritten. An explicit approved
amendment identifies exactly the two changed inputs and preserves their old
versions. The new execution manifest contains the corresponding authorized
hashes. All original full-run and per-test failure receipts remain unchanged,
as do all 520 original runtime hashes and the canonical accepted-prefix and
coverage records.

The earlier full regression remains its original 3,744-pass, 304-failure,
12-skip baseline. Focused results do not overwrite it. NASA acceptance remains
1,078/1,158 (93.1%); no requirement credit is assigned by these test corrections.
Full parent qualification and the broader campaign remain incomplete.

The stage contains approved-amendment-v1.json, patch-scope-validation-v1.json,
build-result-v1.json, verification-result-v1.json and
historical-evidence-preservation-v1.json. Per-case logs and terminal receipts
are under runs-v1 and runs-v2; verification-result-v2.json preserves the
standalone repeat and verification-summary-v1.json summarizes the final result.
Leak checking is disabled in the ASan runs;
external libraries, native desktop input and host GPU/compositor behavior are
not qualified by these private headless executions.
