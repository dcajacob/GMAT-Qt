Command sequence finalization after an error
============================================

Implementation first; new tests remain deferred. GmatCommand.cpp,
BranchCommand.cpp and GmatFunction.cpp previously stopped walking their command
sequences at the first RunComplete exception. Later commands retained running
state and missed their terminal cleanup. This is relevant to function scope and
iterative-output recovery now that ephemeris terminal errors reach callers.

Head-sequence, branch and function finalization now attempt the remaining
commands in their original order, capture the first exception with exception_ptr
and rethrow it after the walk. Branch idle flags and base-command initialization
state reset before rethrow. Function reference restoration is attempted after
the walk, including after a command failure. The existing fcsFinalized recursion
guard stays set during finalization; it records an attempted walk, not successful
cleanup of every resource. Secondary errors do not replace the first exception.
Single-command/non-head behavior and branch-end traversal rules are unchanged.

This does not guarantee successful cleanup inside a failing derived command,
atomic file rollback, full invocation unwinding or preservation of an earlier
execution exception by every outer caller. Headers, layouts, virtual slots,
numerical formulas and original assertions/tolerances are unchanged. Save output
completion was inspected and already checks flush and close; no Save edit made.

Build and bounded existing checks
---------------------------------

Stage: build/nasa-coverage/coverage95-20261105-implementation-command-finalization.
Three exact preimages and incremental.patch retain these production edits only.
Patch SHA256:
84938e68029c87dc2c04f8dedf508656b150b7e7f6322c217201ea280ef70f59.
Reverse patch check passes. The private integrated-current build passes in6.75s
with no diagnostics. summary.json records current image and source hashes;
seven monitored installed images are unchanged. Public packages are not updated.

Four unchanged existing GUI cases pass against the new private core/function
plugin: Recursion, Stop, NestedDCRuntimeError and NestedYukonStopCommand.
These cover recursive outputs, stop/edit/recovery and nested differential-
correction/optimization interruption with original ephemeris publication checks.
The first invocation of the fourth check used the nonexistent abbreviated
NestedYukonStop fixture and failed before testing runtime behavior. That failed
invocation remains; only the corrected fourth invocation repeats in v2.

Replay uses run-existing.py, saved command.json files and the existing isolated
startup/display helpers with fresh output directories. v1 retains three passes
and the invocation failure; v2 retains the corrected fourth pass. Native xcb,
resizable initially minimized desktop, zero screenshots, private core mapped,
older installed core absent; all handles terminal. No new tests, fixtures,
registrations, qualification readers, matrices or seals. Direct injected terminal
errors and multiple-failure ordering checks are deferred; existing passes are
regression checks, not qualification of those new exception paths.

No original requirement newly admitted. Verified coverage stays1053/1158,
105open,48neededfor1101. Original59holds,31resolved/28unresolved including6MATLAB,
37MATLAB identities and separate historical1816/2367 stay unchanged. Goal remains
active/incomplete; continue demonstrated supported runtime/editor gaps.
