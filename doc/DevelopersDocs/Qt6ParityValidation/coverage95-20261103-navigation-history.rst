Completed-filter history repair and navigation operation closure
================================================================

Coverage remains **1,019/1,158 (88.0%)**, 139 open and **82 more needed**.
Historical operations remain **1,816/2,367 (76.7%)**. No original requirement is
admitted by this bounded repair. All Others!E196 retains its full 53-entry command
and six-operation conjunction, and all 59 holds and scientific/core/current-build/
full-NASA gates remain open.

Demonstrated production defects
------------------------------

The prior ordinary RunEstimatorBranch operation test failed unattended and
visibly after a nested filter completed. RunSmoother followed previous siblings
only, so it missed the filter inside the completed If branch. New NestedIf and
Else guards reproduce this failure. A third guard runs a distinct named filter
with only six observation records before the smoother: the old lookup ignores
the requested filter name, selects the wrong instance and the mission fails.
The wrong-filter run's log retains its five-minute epoch and Sandbox failure.
SkippedMatching and FutureSkipped guards pass before the repair.

Exact repair and limits
-----------------------

Three production files change: RunEstimator.cpp, RunEstimator.hpp and
RunSmoother.cpp. RunEstimator records a monotonic completion sequence only after
its normal FINISHED path advances and finalizes successfully. The exception
cleanup path does not record completion. Copies, assignment, a new deferred
execution, Reset and mission cleanup clear the sequence. An atomic counter
provides runtime order across command instances without retaining pointers.

RunSmoother walks the mission graph using unconditional next links, child links
and a visited set. It selects the greatest nonzero completion sequence whose
solver is a SeqEstimator with the requested name. This avoids treating source
order as execution order, excludes unexecuted/skipped commands, and terminates
across branch-end cycles. It transfers both the selected instance's update
statistics and its object buffer. No propagation, filter, covariance, residual,
measurement or smoothing equation changes.

RunEstimator's class layout changes, so both libGmatEstimation and libEKF were
rebuilt; before/after hashes and actual visible process maps prove the new images
were loaded. The exact preimages and core-v2.patch are retained. These tests
establish the listed bounded behavior; they do not grant scientific approval or
claim a complete solver-loop execution matrix.

Validation and reused outcomes
------------------------------

Five new native GUI guards are individually registered: NestedIf, Else,
SkippedMatching, FutureSkipped and WrongNamed. Their original fixtures and the
shared Script Event driver's assertions remain unchanged from the pre-fix run.
The WrongNamed input deliberately uses the first six of the retained eleven GPS
records for Other, while FPS still requests the full EKF. The short input's
source and hash are preserved. The expected smoother still must process all
eleven full-filter records and reach six hundred seconds.

All five guards, the unchanged failing RunEstimatorBranch operation case and the
existing NasaScriptEventBackendNavigation regression now pass unattended and
visibly: **14 current native GUI passes**. The earlier five passing operation
contexts are independently reaudited and reused as **ten source-versioned
passes**. All six navigation operation contexts therefore have passing evidence
in both modes, at their recorded versions. This does not attribute old outcomes
to a newly qualified complete binary.

The current selected matrix supplies **16 twelve-field state rows**, **48
additional navigation state rows**, **72 summary components** and **1,056 GPS
report component rows**. Closed-form Earth point-mass circular motion uses
mu=398600.4415 and radius=7000 km. Sat is checked at twenty seconds and navigation
spacecraft at six hundred seconds. Original bounds remain 1e-4 s, 2e-5 km,
2e-8 km/s, 2e-10 days and 1e-12 scalars; summary display bounds remain 2e-5.
Both filter and smoother reports retain eleven records with exact sixty-second
UTC identities, three components each, residuals below 2e-5 and independent
7000-km observed/computed norms. These norm checks do not independently qualify
all Earth-fixed orientation effects.

A direct graph test exercises actual RunSmoother::Initialize with controlled
completed/unexecuted estimator probes. It proves latest completion wins when
source order disagrees, nested discovery, cycle termination, requested-name
matching, skipped/future exclusion, copy/assignment reset and mission cleanup.
The GUI runs provide actual solver completion evidence; the direct test supplies
otherwise hard-to-distinguish graph and lifetime guards. It is registered as
QtAcceptance.NavigationHistorySelection. Its final run binds binary/library
hashes before and after execution. An initial direct-test compilation error
(missing abstract-method stubs) is retained at source-v3; source-v4 fixes only
those stubs. The GUI/core assertions stay unchanged.

All visible windows start minimized, resizable and non-fullscreen. Seven mapped
processes bind the actual native images. Zero screenshots, runtime binary copies
or large-kernel copies are retained.

Audit and continuation
----------------------

AuditNavigationHistoryRepair.py independently checks original failures, exact
core delta, unchanged assertion sources, source-v2 unattended/source-v4 visible
versions, raw results, per-spacecraft summaries, filter/smoother reports,
stdout/JUnit, visible exposure and process maps. Four corrupt controls reject
wrong final state, missing filter state, wrong smoother state and false mission
completion. It invokes the previous navigation auditor to retain and reuse its
five original passing contexts without erasing its historical failures.

The stage build/nasa-coverage/coverage95-20261103-navigation-history contains
source archives, regular-file evidence-inputs-v1.json, evidence-audit-v1.json,
precise core preimages/delta and stage-receipt-v1.json. The audit status is
passed-bounded-navigation-history-repair with an empty qualified-key list.
The authoritative coverage ledger remains the default-orientation
coverage-snapshot-v1.json. All 59 prerequisite holds are preserved.

Continue the full Mission Tree parent using the existing 53-entry worklist.
Eleven function/dynamic/backend/navigation families now have explicit six-operation
root/branch evidence at retained versions. Remaining work includes SaveMission,
structural/unique-starter applicability, and missing tree Close/context Summary
routes in older lifecycle families. Do not replay completed families just to
rediscover them. The 95-percent goal is active and incomplete.

Replay only the new and affected selection
-----------------------------------------

.. code-block:: sh

   cmake --build build/linux-gui --target GmatQtNasaScriptEventCommandTests GmatQtNasaCommandNavigationOperationTests GmatNavigationHistorySelectionTests -j4
   ctest --test-dir build/linux-gui -R '^QtAcceptance.NavigationHistorySelection$' --output-on-failure
   python3 src/qtgui/tests/RunNavigationHistoryAcceptance.py
   python3 src/qtgui/tests/RunNavigationHistoryAcceptance.py --visible
   python3 src/qtgui/tests/AuditNavigationHistoryRepair.py \
     --stage build/nasa-coverage/coverage95-20261103-navigation-history \
     --output /tmp/navigation-history-audit.json
