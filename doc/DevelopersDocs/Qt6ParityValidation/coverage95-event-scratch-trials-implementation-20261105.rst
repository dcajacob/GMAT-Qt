Internal event recordings during solver trials, 2026-11-05
============================================================

The earlier EphemerisFile trial-output guards also reached EphemManager's
internal SPK/CK recorders. Those temporary kernels supply event-search geometry,
so suppressing their data could leave FindEvents inside solver sequences without
the trajectory it needs. They must be distinguished from user-requested
EphemerisFile output when applying the output-only trial rule.

Owning source and lifecycle
---------------------------

EventLocator::Initialize asks the spacecraft to RecordEphemerisData when enabled.
Spacecraft owns an EphemManager that creates internal orbit/attitude EphemerisFile
recorders and subscribes them to Publisher. LocateEvents calls the spacecraft's
ProvideEphemerisData, which closes the current recording, loads written SPK/CK
kernels, stores their paths and starts a new recording filename. The manager
retains its native kernel list/coverage handling and later unload/delete policy.
Spacecraft assignment does not replace the manager with a cloned recorder.

Non-test background-generation callers in the owning source are EphemManager's
two recorders and the EphemerisFile-to-writer setter. The role is not a scripted
output parameter. It already controls the native SPK writer's internal short-
segment treatment. This batch does not invent a second type of user output or
change any short-segment interpolation rule.

Implementation
---------------

ShouldSuppressTrialOutput combines the existing effective enclosing-solver state
with the destination's background role. Only non-background/user output is
suppressed. SetRunState creates/restores trial output configuration snapshots
using that same distinction; Subscriber still receives the effective SOLVING
state. No publisher or solver state is replaced with a fabricated accepted state.

All existing trial guards use the same predicate: full-resource assignment,
stale-file initialization, Toggle, filename rollover, reference bindings, numeric
step updates, event callbacks and packet distribution. Internal recorders retain
their normal data, notification/segment, file and writer lifecycle during trials;
they do not take output-entry snapshots. User-output resources retain the same
trial suppression and accepted-pass restoration rules as before this batch.
Terminal processing still closes a writer before the ordinary packet guard.

EphemManager now sets background generation immediately after constructing each
recorder, before filename/reference setup, Initialize and ToggleOn. This also
covers creation while already inside a solver. EphemerisFile retains the role
and passes it to a lazily created writer. The obsolete instruction that the
role had to be assigned after initialization was removed. FindEvents help
explains temporary geometry recordings and per-invocation report Append behavior.

Limits and accounting
----------------------

This is not full isolation of each solver trial's kernel pool. Native retained
kernel lists, overlapping/prior-trial coverage and selection, event-report
contexts, nested functions/solvers, last-Vary markers and control-registration
sweeps remain source-review/implementation work. In particular, the newly fixed
structural binding list does not replace the direct-list registration sweeps
inside Target/Optimize Execute. A complete all-command contract is not claimed.
The user-output final-pass/convergence policy and recursive returned dependency
ownership also remain open; this patch changes neither policy nor ownership.

The private integrated Release build passes 77 steps in 207.19 seconds; session
11279 completed. Enabled core, Qt, console, plugins and language bindings compile
and link. Final source hashes match the at-build snapshot. Help XML syntax and
essential owning-source review accompanied compilation. No test, GUI, mission,
function, model, solver, numerical, proof-reader, seal or qualification execution
was performed. Configured test/probe clients linked only.

Stage: build/nasa-coverage/coverage95-20261105-implementation-event-scratch-trials.
Preimages, before/at-build/final hashes, exact patches, build log/receipt and
summary-v1.json are retained. FRC!E7/E55 and FRR!E406 stay partial; no engineering
row or acceptance credit is added. Tracking-v7 stays 1088/1158 (93.96%), with 70
other original rows partial/unclassified/dependent. Recorded verification stays
1053/1158 (90.9%); all 105 original acceptance rows remain open. Original row
texts/fingerprints, assertions, fixtures, tolerances, denominator, ledger and
gates remain unchanged.
