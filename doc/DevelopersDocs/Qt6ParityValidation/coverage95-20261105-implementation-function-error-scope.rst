Function invocation cleanup covers setup and results
====================================================

Implementation first; new tests deferred. FunctionManager.cpp previously caught
only BaseException from the function body. Argument setup, initialization,
result assignment and call-stack restoration could escape without invocation
cleanup. Its cleanup could also replace the original execution exception.

A scoped invocation guard now covers setup through result/stack handling and
false returns. It attempts function finalization, publication cleanup and owned
invocation cleanup independently, preserving an active exception or false result.
Cleanup diagnostics are contained. The manager is temporarily retired during
FCS finalization so recursive RunComplete cannot delete its object store while
that finalization walk still uses it. Existing finalization/publication/release
order remains. A nested invocation already cleaned up is not cleaned again.
The guard runs before caller solver statuses restore, keeping private solver
finalization from overwriting those caller statuses.

New calls activate cleanup before preparing arguments, including calls following
a finalized invocation. A deleted ObjectInitializer pointer is cleared before
constructing its replacement, so allocation failure leaves no dangling pointer.
No public API, header, layout, virtual slot or numerical algorithm change.
Cleanup failures still need component-specific recovery; this is not atomic
caller/output rollback or a claim that every resource release succeeds.

Stage: build/nasa-coverage/coverage95-20261105-implementation-function-error-scope.
Exact preimage and incremental-v1/v2.patch retained. V1 compiled only; v2 preserves
finalization/publication order with an explicit retirement guard. Final patch
SHA256:909b4143a5d15dc5b194a0e37cc3e30ccbf2f1de7f2fef62c0260ea042096c47.
Reverse patch check passes. Private builds pass5.22/5.51s without diagnostics;
summary.json records final sources/images. Seven installed images unchanged.

Seven unchanged existing GUI cases pass against v2: I/O Properties and Nested;
builtin StringNumberInvalid and QuaternionProductZero; control Recursion,
Mutual and Stop. Original numerical/error/correction/recovery assertions remain.
Native xcb, resizable initially minimized desktop, zero screenshots, private core
mapped and installed core absent; all handles terminal. First tool launch timed
out in automatic approval review without starting a process; the permitted retry
ran all seven. gui-launch-v1.json retains that launch outcome. Replay uses
run-existing.py and saved case commands with fresh output directories.

No new tests, fixtures, registrations, matrices, readers or seals. Direct setup,
result-assignment, simultaneous cleanup and allocation-failure backfill remains
deferred; regression passes do not qualify those new failure paths. No original
row admitted. Verified1053/1158,105open,48neededfor1101;59holds,31resolved/
28unresolved including6MATLAB,37MATLAB identities,historical1816/2367 unchanged.
Goal active/incomplete. Public preview remains unchanged.
