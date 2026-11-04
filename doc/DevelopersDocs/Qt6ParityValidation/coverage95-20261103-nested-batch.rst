Nested batch output membership and simulator reentry
====================================================

Three new cases pass unattended and visibly, with two executions in each window:
six GUI passes, twelve phases and four directly affected regression passes.
Outer Target/DC, Optimize/Yukon and an active-constraint Optimize/VF13ad each
contain a complete simulation plus converged BatchEstimator. Their trial radii
include 7,000 and 7,100 km; the sole globally accepted pass is the independent
7,200 km, 45 degree circle. The second mission execution avoids Build.

Audit v1 checks 440,136 raw publications. All 17,292 emitted OEM rows / 103,752
components match independent literal-mu 398600.4415 circle equations and native
batch final-pass publications from the globally accepted outer pass. It also
checks every one of 189,996 local batch final publications in 132 outer trial
streams against the independent circle for that pass's radius. Those trial
streams are excluded from output. Native outer clone states determine global
acceptance; native estimator IsFinalPass determines local acceptance. The
publisher still reports SOLVEDPASS for local accepted streams, while the existing
EphemerisFile nested classifier suppresses outer trials.

The final orbit has 431 observations through 28,800 seconds, yielding exactly
1,441 knots at 20 second spacing. This differs from the inherited 7,000 km
network's last observation at 28,680 seconds / 1,435 knots. The new test's
provisional 1,435 assertion failed and is preserved. Its corrected grid follows
the actual new observation support; the original RK4 and closed-form expressions
and 2e-5 km / 2e-8 km/s bounds remain unchanged. Actual Write checkbox, draft
isolation, exact Apply bytes, Unicode save/reopen, history and execution locks
are checked. Both mission executions use the same source.

One core defect and its repair
------------------------------

The original RunSimulator remains FINISHED after its first invocation. Later
passes take zero simulation steps, leaving the initial observation file intact.
At the first 7,100 km pass the batch fit then has only one valid observation after
editing and fails. All three original failures are retained.

A reset-only candidate reaches measurement generation but crashes while assigning
MeasurementData in MeasurementManager::CalculateMeasurements. The old manager
retains adapters across initialization. The complete one-file repair rebuilds
the command-local Simulator and PropSetup clones through Initialize, then invokes
the existing Reset action before executing again. Native measurement equations,
force model, estimator, publisher and ephemeris writer code are unchanged in this
increment. RunSimulator.cpp.before and core-v5.patch preserve the exact delta.
The successful two-phase runs demonstrate regenerated data and no-Build replay.
The unchanged batch-mode Reentry and forward-filter STK regressions pass both
modes. Their earlier unattended source version is retained explicitly; no single
all-tests current-build qualification is claimed.

Failures and remaining numerical gate
------------------------------------

Thirteen native failed attempts and one compile failure are retained:

* v1 compile: a hidden Solver overload requires the integer parameter ID.
* v2, three cases: the new test retained a configured Export pointer across Apply,
  which rebuilds objects. It now obtains the fresh object after Apply.
* v3, three cases: actual stale-observation simulator reentry failures.
* v4, three cases: the partial reset-only repair crashes; gdb-v4.log localizes it.
* v5, one case: the new 1,435-knot assumption fails on the complete 1,441 grid.
* v6/v7: unconstrained VF13ad forward differences with h=1e-4 and h=1e-8 report
  convergence but miss the unchanged independent final-orbit bounds. The larger
  step produces U about 1.99995 rather than exactly 2. These are retained failures,
  not qualified unconstrained optimization accuracy.
* v9 names the original h=1e-4 unconstrained profile as ordinary registration
  QtGui.NasaNestedBatchOptimizeVF13adUnconstrained. It still fails without an
  expected-failure property or inverted assertion. The supported VF13ad output
  case separately adds the explicit active lower constraint U>=2 at the analytic
  quadratic optimum; its output-membership success does not erase that gate.

v6 supplies the DC/Yukon passes and v8 the constrained VF13ad pass. The visible
selection executes those three supported profiles plus the two affected
regressions. Five negative audit controls reject wrong state, missing knot,
incorrect trial acceptance, missing replay and incorrect pass radius. A binder
bootstrap wrongly excluded the file runtime-census.json as a runtime directory;
its diagnostic is retained and only directory components are now excluded.

Finite census and remaining requirement work
---------------------------------------------

The actual normal-startup solver factory lists DifferentialCorrector,
ExtendedKalmanFilter, Simulator, BatchEstimator, deprecated BatchEstimatorInv,
Yukon and VF13ad. Smoother uses a separate factory category and is explicitly
captured. BatchEstimatorInv creates the same BatchEstimator class. Fmincon is
commented out in the startup and remains a prerequisite hold. The native file
format list is CCSDS-OEM, SPK, CK, Code-500 and STK-TimePosVel. CK is identified as
an attitude writer in the source census; no attitude science is inferred.

The source census records all five writer constructors and the common trial
publication guard before data retrieval/output. It links the prior every-row
filter/smoother STK and batch edge proofs, plus older batch format/SPK and
unsupported filter controls. The older immutable conjunction must still be
reaudited at its recorded versions. Do not execute the old audits directly:
they contain current-worktree assertions and rewrite their historical summaries.
Use retained payloads, original snapshots, claims and independent decoders.
No repeat of completed command, flat solver, filter STK or batch-edge families is
needed. Unconstrained VF13ad numerical accuracy remains explicitly open.

FRR!E406 stays open and no exact key is assigned. The separate OEM B-1 check was
already completed in coverage95-20261101-oem-b1.rst; its retained ShortArc and
ICD conflicts are unchanged. Do not repeat or silently revise those assertions.
Coverage remains 1,019/1,158 (88.0%), 139 open and 82 needed; historical operations
remain 1,816/2,367 (76.7%). All 59 holds, fingerprints, denominator and separate
scientific/core/current-build/full-NASA gates remain. No commit or push.

Replay
------

From the repository root::

   cmake --build build/linux-gui --target GmatQtNasaNestedBatchTests -j 4
   python3 src/qtgui/tests/RunNestedBatch.py
   python3 src/qtgui/tests/RunNestedBatch.py --visible
   python3 src/qtgui/tests/RunNestedBatch.py --include-unconstrained-conflict
   python3 src/qtgui/tests/AuditNestedBatch.py \
     --stage build/nasa-coverage/coverage95-20261103-nested-batch \
     --output /tmp/nested-batch-recheck.json

The conflict flag includes the currently failing ordinary unconstrained probe.
The full executed-source-v9 archive, before/after hashes and five mapped visible
processes bind the final native execution. Windows start minimized, resizable
and nonfullscreen; screenshots and runtime binary copies are zero. Every build,
native test and debugger process is terminal.
