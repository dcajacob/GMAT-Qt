Implemented-feature verification: first batch
=============================================

The user authorized verification of implemented features while leaving unresolved
requirements and all acceptance gates open. This supersedes the earlier runtime
hold. The implementation and verification campaigns remain incomplete.

Results
---------

Twenty-eight distinct cases passed. Four focused GUI cases also passed on private
native X11: cost-command spelling, Stop inside a solver function, Pause/Resume
inside a solver function, and toolbar contrast. This was private Xvfb execution,
not user-visible acceptance or full platform qualification.

The passing cases comprise:

* The unchanged OEM text and record-support checks; the latter independently
  checks 1,007 serialized state/epoch records.
* All ten existing function I/O cases, three function-definition cases, three
  function run-control cases, two optimizer-rule cases and toolbar RunControl.
* Seven focused new cases: POV geometry, nonfinite/short OEM states, compatible
  cost-command syntax, solver-context Stop and Pause/Resume, function output
  bracket policy, and dark/light toolbar icon contrast.

The POV checks compare mesh volumes against analytic box/torus values or an
independent circular-section integral, including all four spindle modes,
transforms and intentional empty geometry. They do not qualify arbitrary POV
syntax, shading or a complete importer profile. The OEM numeric fixture covers
21 nonfinite epoch/state combinations plus a short vector, rejection before
buffer mutation and subsequent valid-record recovery. Cost syntax uses the real
Yukon optimizer and the independent constrained optimum X=2, cost=1; native
serialization and clones retain the exact spelling and label. Both native and
unattended function controls verify the actual solver publication context and
following-command sentinel. Rendered icon silhouettes meet 4.5 active and 3.0
disabled contrast ratios in both dark and light palettes after a palette change.

Failures remain open
---------------------

The unchanged cases QtGui.NasaAssignmentNamedCallGmatFunction and
QtGui.NasaAssignmentNamedCallBuiltinGmatFunction hit their original 30-second
mission watchdogs in both unattended and private native X11 execution. No timeout,
fixture or assertion was relaxed. They remain failures, not environment waivers.

A separate ten-second console diagnostic of the GMAT-function sequence, with
CommandEcho enabled in a private copy, stalls at its first Propagate before the
function call. That locates a diagnostic boundary; it does not identify the root
cause or establish the same internal cause for the built-in case. The console
first rejected the original Unicode comment spelling, so the second diagnostic
uses ASCII-only comments with unchanged equations and commands. Neither diagnostic
is acceptance evidence. Trace and original failure outputs are retained.

Repairs driven by failures
---------------------------

Three concrete defects were repaired in five owning source files:

* GmatFunction::SetGmatFunctionPath used a helper that checks the directory part
  of a filename. Directory probes now append a separator so an existing .gmf
  file is not treated as a folder and given a second filename.
* Minimize overrides the existing virtual label-insertion hook for the compatible
  NonlinearConstraint spelling. The inherited native-type-name search split the
  alias keyword. Canonical Minimize behavior is retained; no new virtual slot,
  parameter ID or data field was introduced by this repair.
* ScriptInterpreter retains authored brackets for whole native-function calls.
  Interpreter assembles allowed scalar shorthand before applying the existing
  RequireOutputBrackets policy. Strict/bracketed, strict/bare, optional/bracketed
  and optional/bare cases have explicit positive/negative checks.

The new test initially used the wrong Minimize serialization argument count and
omitted the leap-second reader needed for OEM epoch formatting. Those test defects
were corrected; the initial compiler failure and numeric crash are retained.
Native launches initially could not connect to X11 inside the sandbox. Replays
outside the sandbox used private displays and passed; the failed launches remain.
The latest focused-test build returned zero. All compiler and test handles for
this batch are terminal. No source was edited during a live build or test.

Evidence and limits
--------------------

Records are in build/nasa-coverage/coverage95-20261107-verification-restart.
results-summary-v1.json distinguishes 28 passing cases from the two unresolved
integration cases. Fifty invocations include retained failures, repairs and
native replays; they are not fifty distinct passing tests. Source/test snapshots,
compiler logs, per-case commands, receipts, native input hashes and diagnostic
logs are retained. The four focused native replays checked unchanged source and
runtime hashes before/after execution.

All 1,334 pre-existing test inputs and the original requirements/worklist/tracker
remain byte-identical. New verification is registered separately in CMake; no
original assertion, fixture, tolerance, row text, fingerprint, disposition or
denominator changed. No new row credit or percentage is assigned. The carried
verified baseline remains 1053/1158 (90.9%); 1093/1158 (94.39%) remains an
engineering estimate. This batch does not certify the 40 implementation-present
rows, all 105 open acceptance rows, current-build science or full NASA acceptance.

Next bounded work is the two named-call integration timeouts, starting from the
observed first-Propagate boundary. Keep the original watchdogs and numerical
assertions. Do not reopen speculative solver/include/renderer feature expansion.
After that, continue the original implemented-feature worklist with tests that
cover each clause's actual scope. External and undefined requirements remain open.
