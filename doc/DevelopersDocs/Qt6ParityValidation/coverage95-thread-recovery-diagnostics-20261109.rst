Interrupted-thread recovery, campaign 2026-11-09
================================================

This records the recovery checkpoint before the subsequent user approval of
two original-test lifetime corrections. Those corrections and their separate
verification are recorded in coverage95-approved-observer-lifetimes-20261109.rst.

The previous thread ended with a platform error stating that its content was
flagged for possible cybersecurity risk. The thread exposed no additional
reason or specific trigger. Its last visible action configured a private local
crash-diagnostic build. That configure also failed independently because its
private application directory lacked required startup/Python template files.
The evidence supports continuing ordinary local debugging; it does not identify
the classifier's internal decision.

Completed regression baseline
-----------------------------

The unfiltered 4,060-case run completed at 2026-11-09T10:17:33 UTC with 3,744
passes, 304 failures and 12 skips. The guarded v2 comparison verified all 4,344
source hashes, all 1,334 protected original inputs and the canonical acceptance
records before any continuation change. Nineteen previous failures now pass;
one former pass fails, and one retained failure changed its diagnostic.

The new failure is SharedPlaybackButtons. PlotIntegrationSteps changed from
its previous epoch assertion to a segmentation fault after 74 emitted case
proofs. Both original failed receipts remain. The 304 failures are grouped into
53 diagnostic groups for investigation; grouping does not classify them as
product defects or obsolete expectations.

Build configuration repair
--------------------------

The legacy UNIX configuration block overwrote CMAKE_CXX_FLAGS_RELEASE with
``-O3`` after cache initialization. Consequently, the first private diagnostic
build had zero instrumented C++ compile actions despite sanitizer flags in its
cache and link commands. It is explicitly invalidated as a C++ ASan diagnosis.

CMakeLists.txt now retains configuration flags initialized by CMake, the
toolchain or the user. This also preserves Debug sanitizer flags added by
GMAT_RUN_ADDRESS_SANITIZER. Standard CMake/toolchain defaults now apply instead
of the former forced configuration values. The exact applied candidate was
configured and generated in Release, Debug, RelWithDebInfo and MinSizeRel;
all 882 C++ actions in each configuration preserved the requested flags.
Release and Debug retained their requested sanitizer instrumentation.
This validation generated build files; it did not rerun the full regression
after the CMake correction.

A separate diagnostic build, using global instrumentation flags with the
original CMake source, completed all 946 build actions. Its generated commands
verify instrumentation on all 882 C++ and both C compile actions, and both
test binaries expose ASan instrumentation symbols. External libraries remain
uninstrumented and leak checking is disabled; these runs diagnose invalid
memory accesses and do not qualify leaks or external runtimes.

Observer findings
-----------------

The original instrumented SharedPlaybackButtons aborts with heap-use-after-free
at SharedPlaybackTests.cpp:89. Const QMap::operator[] copies PlotCurve; the
test retains a reference to a point in that temporary copy after destruction.
A separate diagnostic with a const-map iterator compares all retained point
fields across 14 observations. It passes both normally and with ASan, preserves
all samples, and observes the expected final XY point (39, 1521). The protected
original test remains unchanged and its failed result is not promoted to pass.

The original instrumented PlotIntegrationSteps aborts before its first case
proof because editInteger calls hasChanges on a ResourceEditor after Apply has
replaced it and dispatched its deferred deletion. The stack identifies the
invalid observation at PlotIntegrationStepTests.cpp:85, reaching
ResourceEditor::hasChanges at ResourceEditor.cpp:1881. A separate diagnostic
copy reacquires the current panel from the guarded subwindow after Apply and
checks its clean state and resource identity. Its patch is an artifact only;
the protected helper and its original assertions remain unchanged.

The instrumented safe-observer plot diagnostic passes 95 original cases,
including the previous crash location, with no ASan report. All 276 guarded
editor observations confirm a retired old editor and a clean replacement.
It then exits 1 at the unchanged history assertion. The exact earlier crash
instruction remains unproved; these results do not convert the original
failed parent receipt into acceptance.

A separate observation-only, ordinary build reached 95 original plot cases
and the existing history assertion. The Message Window contained 10,000
blocks before and after the mission, matching its configured limit. Exactly
36 leading lines were displaced; the remaining 483,138 characters match the
new history prefix byte for byte, followed by 2,369 new characters. The
unbounded-prefix assumption in the observer conflicts with the bounded
Message Window. This explains that assertion, not the precise instruction
responsible for the earlier uninstrumented segmentation fault.

Further bounded reconciliation
-------------------------------

Twelve SolverNestingRejection failures expect the blanket nested-control-flow
restriction removed by the earlier FRC!E7/FRC!E55 nearest-solver-scope repair.
The current Interpreter retains enclosing solver ownership through ordinary
branches. Ten related original positive control cases pass in the completed
run. The twelve specifically paired archived positive cases remain skipped;
they are not reported as passing. This reconciles the stale negative premise
without reverting supported nesting or editing its failure receipts. Exact
post-Apply subconditions of the twelve negative tests have not been separately
instrumented, and no whole solver-parent acceptance credit is assigned.

Qualification and evidence
--------------------------

NASA acceptance remains 1,078/1,158 (93.1%), with 80 open scopes. No accepted
prefix, ledger, admission reader, original assertion, numerical tolerance,
fixture or registered timeout was changed. The overall campaign is incomplete.

Evidence directories under build/nasa-coverage:

* coverage95-20261109-post-repair-regression-v1: completed run, guarded
  broad-comparison-v2.json, failure-groups-v1.json, platform error investigation,
  stable-playback-v1 and history-observer-v1 diagnostics.
* coverage95-20261109-plot-crash-asan-current-v1:
  instrumentation-invalidated-v1.json and original diagnostic logs.
* coverage95-20261109-plot-crash-asan-current-v2: generated instrumentation
  audit, build receipt, original ASan findings, and separate observer diagnostics.
* coverage95-20261109-build-flags-repair: exact candidate, four-configuration
  validation and applied-result-v1.json.

The minimal two-file observer-lifetime patch is prepared under
coverage95-20261109-post-repair-regression-v1 as
proposed-protected-observer-lifetimes.patch. Applying it requires release of
the campaign's explicit protected-original-input boundary. It does not change
numeric assertions or address the separate bounded-history observer premise.
