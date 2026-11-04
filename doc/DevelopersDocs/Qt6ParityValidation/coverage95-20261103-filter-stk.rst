Independent filter and smoother STK output membership
====================================================

Two new cases pass unattended and visibly, with two executions per case across
Unicode save/reopen: four GUI passes and eight native phases. All 176 STK rows /
1056 Cartesian components match an independent 7,000 km, literal-mu = 398600.4415
point-mass circle. Every output row also matches a selected SOLVEDPASS native
publication, and every selected publication epoch appears in the file.

The inherited navigation fixture has eleven noiseless GPS observations at
0:60:600 seconds and exact initial state. The STK profile has two event-boundary
rows per measurement epoch, 22 rows per run, rather than a row for every internal
10 second integration step. The accepted forward stream has 23 publications; the
accepted smoother stream has 24. Each complete mission also exposes 22 smoother
SOLVING publications. Across both modes the audit checks 552 raw publications,
188 selected accepted publications and 176 smoother trial publications, preserving
the explicit accepted/trial distinction. Unsupported filter/smoother formats
retain their older native rejection evidence and were not replayed here.

The resource tree opens the actual Export editor; its Write checkbox is enabled
and applied through the form. The draft must leave source unchanged; Apply may
change only the WriteEphemeris statement. The mission starts with Export Off,
then toggles it around only the selected RunEstimator or RunSmoother command.
All other mission bytes derive from the retained navigation fixture. Output
coordinates, units, scenario epoch, exact cadence and every written vector are
checked. Bounds remain 2e-5 km and 2e-8 km/s with 1e-4 second epoch matching. The observed
publication frame uses native subscriber initialization and data-frame callbacks;
independent circle checks prevent a shared conversion from replacing truth.

Five new-test development failures are retained:

* v1's observer lacked native subscriber frame initialization and threw through
  the engine's generic exception handler; both cases failed at RunEstimator.
* v2 captures the observer diagnostic directly: missing coordinate context.
* v3 initializes the observer using Subscriber's internal-frame fallback; both
  cases then reach and fail a provisional greater-than 60 row assertion that
  incorrectly equated integration cadence with measurement publication cadence.
* v4 replaces that new-test assumption with exact 22-row/eleven-epoch assertions
  and requires selected SOLVEDPASS publications. The original numerical oracle
  and every position/velocity bound are byte-identical. The fixture also gains
  its required initial Export Off to isolate the selected phase after GUI Apply.

No production source or previously qualified test changed. The existing nested
subscriber repair remains byte-identical. The failed source/fixture versions,
receipts, JUnit and diagnostics are preserved; no failures are converted into
passing receipts. All v4 cases pass in both modes.

Stage ``build/nasa-coverage/coverage95-20261103-filter-stk`` has 229 regular artifact
bindings. Source-v1..v4 and the full executed-source-v4 archive, matching before/
after identities, and both visible process maps bind execution. Windows were
initially minimized, resizable and nonfullscreen; screenshots/runtime copies=0.
Audit v1 checks all source/runtime/fixture identities, exact Apply bytes,
receipt/JUnit/stdout/raw proof agreement, numerical state/clock references,
metadata, accepted provider/phase membership and absence of missing epochs.
Five corrupt-proof controls reject wrong state, missing row, wrong selected
phase, wrong accepted count and missing reopen. Every session is terminal.

Build and replay from the repository root::

   cmake --build build/linux-gui --target GmatQtNasaFilterStkTests -j 4
   python3 src/qtgui/tests/RunFilterStk.py
   python3 src/qtgui/tests/RunFilterStk.py --visible
   python3 src/qtgui/tests/AuditFilterStk.py \
     --stage build/nasa-coverage/coverage95-20261103-filter-stk \
     --output /tmp/filter-stk-recheck.json

The registrations QtGui.NasaFilterStkFilter and QtGui.NasaFilterStkSmoother are
ordinary acceptance tests with build dependencies. FRR!E406 remains open for the
remaining finite output obligations, particularly batch-estimator interruption/
iteration-limit/recovery. Existing batch format/SPK and unsupported filter-format
proofs should be reused at their recorded versions. This stage adds every-row
supported STK evidence without claiming covariance/attitude, arbitrary noisy
estimation science or full requirement qualification.

Coverage remains 1,019/1,158 (88.0%), 139 open and 82 needed. Historical coverage remains
1,816/2,367 (76.7%). No requirement key is assigned. The denominator, fingerprints,
59 holds and separate scientific/core/current-build/full-NASA gates remain intact.
