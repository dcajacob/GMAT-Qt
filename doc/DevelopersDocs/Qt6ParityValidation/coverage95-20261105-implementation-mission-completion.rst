Mission completion after cleanup errors
=======================================

Implementation-first repair in Sandbox.cpp and Moderator.cpp. The previous
increment made command terminal walks continue after errors. The owning mission
handlers still allowed cleanup errors to replace the execution error, skip
stopped state, bypass UI/configuration restoration or escape without updating
the native return/exit status.

Sandbox now sets STOPPED and the previous-FCS error flag before error cleanup,
attempts command completion for all exception types and rethrows the original
execution exception. Secondary cleanup diagnostics cannot replace it. An empty
sequence no longer leaves the sandbox marked RUNNING. Normal terminal command,
subscriber and automatic-event failures set the FCS error flag before escaping;
the sandbox is already STOPPED.

Moderator attempts publisher idle transition, subscriber notification, UI
completion, diagnostics, configured object-map/solar-system restoration and
startup EOP restoration as independent phases. A phase failure changes success
or user-stop status to -5 for BaseException or -6 for other exceptions. A prior
execution failure status is retained, with additional phase diagnostics. Outcome
logging runs after restoration and exitCode receives the final status. Unknown
execution-error command reset also uses this contained completion path.

The existing repeated end-of-run notifications and interruption/event order are
preserved. This does not guarantee recovery inside each failed component,
rollback of files, completion of remaining automatic event locators after one
throws, or every early-initialization failure path. No new public API, members,
virtual slots, algorithms or numerical tolerances.

Build and bounded checks
------------------------

Stage: build/nasa-coverage/coverage95-20261105-implementation-mission-completion.
Two exact preimages and incremental-v1/v2.patch remain. V1 was edited before
building; final v2 patch SHA256:
033bdeaec73f99997cd21a8146280f240601d891c6a627e1a18aff40d52cb0c6.
Reverse patch check passes. The private integrated-current build passes8.84s
with no diagnostics. summary.json records current source/native image hashes;
seven monitored installed images are unchanged. No public package update.

Four existing GUI cases pass unchanged: Recursion, Stop, NestedDCRuntimeError
and NestedYukonStopCommand. They include successful runs, runtime math failure,
user stop, function/nested-solver output isolation and same-session recoveries.
Saved command.json/run.log/receipt.json files and run-existing.py provide replay
with fresh output directories. Native xcb, resizable initially minimized desktop,
zero screenshots, private core mapped and old installed core absent. All handles
terminal. No new tests, fixtures, registrations, matrices, qualification readers
or seals. Direct simultaneous execution/cleanup faults, final subscriber/EOP/UI
fault injection and unknown-exception backfill are deferred. Regression passes
do not qualify those new exception paths or whole original requirements.

No original row newly admitted. Verified1053/1158,105open,48neededfor1101;
59original holds,31resolved/28unresolved including6MATLAB,37MATLAB identities
and historical1816/2367 remain unchanged. Goal active/incomplete.
