MATLAB service and Resource ownership repair — 2026-11-08
========================================================

Generic named-object creation returned the Moderator-owned MATLAB process service.
A GUI draft adopted that borrowed pointer into deleting preview storage. The
New-resource dialog also offered MatlabInterface as an independent Resource.
The guide explicitly describes a process service controlled through its Resource
Tree entry rather than a script-created object.

The owning MatlabInterfaceFactory now rejects generic CreateObject requests for
that service before returning the pointer. Its dedicated typed CreateInterface
accessor is byte-identical and still supplies the same process service. The
factory marks the service unviewable in the generic Resource creator across its
constructors, copy and assignment paths. Full factory registration remains intact;
the actual Interfaces > Matlab node and its Open/Close/mode controls remain.
Moderator, ResourceDraft, engine/session/callback code, class headers, instance
layouts and virtual ordering are unchanged. Scripts that previously attempted
named MatlabInterface creation now receive a clear error directing them to the
interface controls or MATLAB function calls. Real Resource types remain available.

Evidence
--------

Stage: build/nasa-coverage/coverage95-20261108-matlab-service-resource-ownership-current-review.
Plan-v1 retains all five original FRC12 rows and the relevant Resource Tree and
MATLAB interface/mode rows.

Six isolated native cases fail before and pass after the repair: factory exposure,
script creation, direct generic creation, both Moderator creator paths, and GUI
draft ownership. The draft witness releases the erroneously borrowed baseline
pointer instead of deliberately destroying the process service. Actual service
identity/name/mode stay intact after corrected rejection, and an ordinary Variable
draft remains independently owned. No engine invocation is needed by these probes.

The first newly authored factory witness used generic CreateObject as the accessor
that should remain available. It was corrected, before the valid six-case baseline,
to use the actual typed service accessor and separately test generic rejection.
Both versions and the original five-case failure are retained. Protected original
tests were not changed.

Seven selected GUI checks pass in EACH final profile, fourteen executions:
current Resource catalogue, full native factory census, ordinary creator/default
transactions, provider-unavailable behavior, supplementary current service menu,
and Save/reload for FminconOptimizer and MatlabFunction. The latter additionally
executes the saved function through actual MATLAB after standalone reload and
returns the independently specified exact value 43. Fmincon checks its seven
writable configuration observations and rebuild, not optimization science.

The actual service workflow checks Cancel, Close while inactive, Open, repeated
Open preserving a workspace, dirty-source isolation, Close/reopen yielding a new
MATLAB process, a GUI-opened workspace serving the mission call (result 130),
ReportViewer contents and busy reentry rejection. Both final runs observe two
actual MATLAB process IDs, close the Engine, and retain source/history/Unicode
caller bytes. The runs recorded 433 and 405 busy reentry observations. These are
Linux/installed-MATLAB observations, not Windows SingleUse or full platform proof.

One original available-provider GUI check remains failed in its retained receipt:
it requires exactly two popup actions, while the preexisting current UI includes
Open, Close, separator and Engine mode. MainWindow.cpp is byte-identical to the
pre-stage plan. The supplementary generated driver requires those four actions
in order, including enabled mode selection, and preserves every other original
process/workspace/mission/source assertion. Reversing the one predicate change
recovers the protected source exactly. No product menu or original assertion was
altered to make that old oracle pass.

The GUI catalogue changes from 86 to 85 independently creatable types. The native
factory census retains all 570 types in 36 categories, including the typed service.
Two previously unexercised real Resource types now have finite Save cases. The
source-versioned family type-name coverage is not a universal construction,
all-properties, dependency or scientific proof.

Limits and continuation
-----------------------

All 1,334 protected test inputs match their baseline. Forty-two current source,
fixture and generation owners are archived and hash-verified. Actual loaded ELF
identities and all failed/corrected witness phases are retained. Linker warnings
about two transitive MATLAB Boost libraries were recorded; actual Engine workflows
nevertheless load and pass.

No new complete requirement credit. Canonical acceptance remains 1,078/1,158
(93.1%); 80 open, 23 additional passes needed for 95%. The keyed audit remains
32 of 44 groups reviewed, not qualified: this is a concrete follow-up rather than
an admission of the whole Resource Tree or MATLAB parent. Six formal MATLAB holds,
read-only persistence roles, all-resource/property/domain/science/current-build
and platform conjunctions remain open. No denominator, original fingerprint,
assertion, tolerance, package, push or message change occurred.

Continue automatically with Function-call cross references. Preserve the exact
numbering inconsistency and both named and literal context: FRC160 names MATLAB
but points to FRR37 (GMAT Functions); FRC161 names GMAT but points to FRR38
(Parameters and Calculations); All Others E241 points to absent FRR36. The next
plan retains all 51 FRR37 and 88 FRR38 rows. Do not invent replacement requirements.
The overall campaign remains active.
