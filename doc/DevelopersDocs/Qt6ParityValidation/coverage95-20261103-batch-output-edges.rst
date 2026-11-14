Batch estimation output edges and three core repairs
====================================================

Five new cases and three affected regressions pass unattended and visibly:
16 final GUI passes and 30 edge-case phases. Each new case has two successful
recoveries in the same window. All 28,700 recovery OEM rows / 172,200 Cartesian
components match an independent literal-mu 398600.4415 circular orbit and the
native final-pass stream. Audit v2 checks 58,182 raw publications, including
28,700 accepted final publications. Earlier passing versions are retained
without counting their numerical rows twice.

The five edge cases are:

* Stop: request a native pause after batch trial propagation, then click the
  actual debugger End mission button. No continuation or OEM data follows.
  Both recoveries use the same source and avoid Build.
* IterationLimit: MaximumIterations=1 preserves the original independent
  six-component scaled-SVD one-step reference and 358 observations. Status is
  explicitly nonconverged, with no native final pass and no OEM rows. The
  approximately 0.24 km remaining truth error is not presented as convergence.
* Singular: 24 observations from twelve colocated stations at two epochs,
  with six Cartesian solve-fors and native Cholesky inversion, fail the normal
  matrix check after trial propagation and write no output.
* InvalidSolveFors: Cd alone with Drag=None is rejected before force evaluation.
  Native estimation already requires a Cartesian/Keplerian state solve-for;
  this case is not a new Cd-only estimation capability.
* InternalRankDeficient: the unchanged rank-deficient rejection probe using
  Internal inversion now fails cleanly and recovers. It is an ordinary passing
  assertion of native error behavior, with no expected-failure property.

For the last four cases the first recovery restores the full normal network
with Build; the second recovery avoids Build. Every recovery has exactly 1,435
knots at 20 second spacing from 0 through 28,680 seconds. Every output state is
checked independently at 2e-5 km / 2e-8 km/s bounds, then matched to a native
IsFinalPass publication. No final publication epoch is missing. Failed/stopped/
limited trials have zero rows and zero final-pass publications. The actual
Export editor Write checkbox, draft isolation, exact Apply bytes, Unicode
save/reopen, source history, execution locks and continuation sentinel are checked.

Three demonstrated core defects were repaired:

* RunEstimator.cpp: a nonconverged batch restored fitted initial conditions but
  published them with its old 07:58 clock. FINISHED now skips this publication
  for every batch estimator; an accepted final trajectory already publishes
  during the final pass. Sequential estimator behavior is unchanged.
* RunEstimator.cpp: Cd-only reduced the spacecraft STM to 1x1. Point-mass force
  derivatives require six Cartesian rows and wrote outside that allocation.
  A source-identical diagnostic run observed propagation state size 7 and an
  empty force cache at the first corrupting call. An early size guard rejects
  this invalid input before force evaluation. Spacecraft, force and ESM code
  and the existing native state-solve-for prerequisite are unchanged.
* BatchEstimator.cpp: Internal inversion returned a covariance with negative
  diagonals, asymmetric entries and NaN correlations for a rank-deficient
  normal matrix. It now applies the existing Schur Cholesky precheck to a copy
  before performing the original Internal inverse. Observation editing,
  derivatives, fit updates and the requested inverse are unchanged. This
  intentionally changes acceptance of invalid covariance; broad numerical
  conditioning and scientific qualification remain open.

All eight failed attempts are preserved at their exact source/fixture versions:

* v1: the proposed CartesianState+Cd failure input compressed away the zero Cd
  column and completed; the new iteration-limit test incorrectly expected a
  full final pass. Both new-test assumptions were retained and corrected.
* v2: Cd-only aborted with heap corruption; the corrected iteration-limit
  assertion exposed the real one-row mistimed packet. Both are repaired.
* v3/v4: provisional short-arc singular fixtures produced no observations
  and failed the trial-propagation assertion. These remain failed receipts.
* v5/v7: a visible rank-deficient network reached Internal inversion and completed
  with invalid covariance. v7 added its ordinary named regression; unchanged
  fixture/driver assertions pass after the v8 covariance guard.

v6 established the native Cholesky error path. v8 changes only the additional
Internal covariance guard, requiring new runs of the affected batch paths.
The final targeted set includes the unchanged InitialGuess and IterationLimit
mode tests and nominal positive batch OEM test. The original one-step reference,
all numerical bounds and the RK4 oracle remain intact. One audit bootstrap
TypeError treated an old proof's row count as an array; its source/diagnostic
are retained, and the audit now reads the existing raw CSV at the same bounds.

The normal Release build was restored after source-identical diagnostic builds.
The final executed-source-v8 archive, before/after identities, eight mapped
visible processes, receipts/JUnit/stdout/proof agreement and exact two-file
preimages/core-v8.patch are bound by authoritative audit v2. The earlier v1 audit
is retained as a superseded checkpoint with its then-open Internal conflict.
All visible windows started minimized, resizable and nonfullscreen. Screenshots
and runtime binary copies are zero; every native/build process is terminal.

Build and replay from the repository root::

   cmake --build build/linux-gui --target GmatQtNasaBatchOutputEdgeTests -j 4
   python3 src/qtgui/tests/RunBatchOutputEdges.py
   python3 src/qtgui/tests/RunBatchOutputEdges.py --visible
   python3 src/qtgui/tests/AuditBatchOutputEdges.py \
     --stage build/nasa-coverage/coverage95-20261103-batch-output-edges \
     --output /tmp/batch-output-edges-recheck.json

All five QtGui.NasaBatchOutputEdge registrations have ordinary assertions and
acceptance build dependencies. Audit v2 also rejects corrupt trial output,
wrong state, missing knot, lost recovery and incorrect terminal status.

FRR!E406 remains open for finite writer/backend and nested-estimator evidence
conjunctions and its exact solution obligation. Reuse earlier supported batch
format/SPK, filter/smoother STK and unsupported-format controls at their recorded
versions; do not add unrelated arbitrary-orbit/covariance/attitude science as an
output-membership clause. The updated exact-row worklist is in this stage.
Coverage remains 1,019/1,158 (88.0%), 139 open and 82 needed. Historical operations
remain 1,816/2,367 (76.7%). Denominator, fingerprints, 59 holds and all separate
scientific/core/current-build/full-NASA gates remain intact. No commit or push.
