Script Event command support: first missing-context batch
========================================================

Nine additional command contexts now pass unattended and visibly, at their
recorded source/runtime versions. Original requirements coverage remains
**1,002/1,158 (86.5%)**, with **99** more whole requirements needed for 95%.
Historical operation coverage remains **1,816/2,367 (76.7%)**. No original
requirement, denominator, fingerprint, assertion or qualification gate changed.

FRC!E138 requires the Script Event to support all commands supported in the
main mission sequence. The existing six branch-family passes are reused.
The new contexts are Propagate, Maneuver, BeginFiniteBurn, EndFiniteBurn,
Save, Report, Global, Toggle and NonlinearConstraint. This fills actual missing
grouped execution contexts; it does not earn the universal row yet.

What was exercised
------------------

Each new context uses the existing main-sequence command fixture and its
independent result contract inside a real Script Event body dialog. Checks
cover the grouped Mission-tree icon, fixed boundaries, Cancel and pending
Apply, exact/idempotent source changes, Undo/Redo, Unicode Save/Save As/reopen,
native contained-command identities and source/history retention after Run.

The reference is the closed-form 20-second point-mass circle, literal impulse
increments, scalar/active-constraint results and command-specific effects.
Each mode checks 108 report cells plus runtime spacecraft/scalar values and
elapsed-time plot publications. Report separately writes the selected value;
Toggle disables the selected subscriber while leaving its control active.
Save exports all six independently expected propagated Cartesian values.
Finite-burn observations require false before BeginFiniteBurn, true before
EndFiniteBurn and false before Report. There is no propagation between those
burn commands; this qualifies activation support, not thrust dynamics.

Global observes the same Keep object move from the local object store to the
global store during execution; Count stays local and nonglobal. Checking only
IsGlobal was insufficient because initialization sets that flag before Execute.
The flag checks are retained alongside the stronger membership/identity checks.

There are 132 native exposure events across the nine selected visible passes.
The nested desktop uses automatic scaling, remains resizable, starts Iconic
(minimized), and has no fullscreen state. No passing screenshots are retained.
Native exposure is not evidence that a person watched or that physical host
GPU/compositor behavior was qualified.

Repairs and retained failures
----------------------------

No production/core code changed in this increment. An exact comparison finds
286 recorded production source files unchanged since the preceding increment.
The original Script Event driver and its six fixtures remain byte-identical.
Changes are a new test driver, replay/audit helpers and CMake registrations.

The first nine unattended cases crashed in the new driver because Apply replaces
the editor and the driver retained its deleted button pointer. The correction
reacquires the editor and controls. A batch-debugger backtrace and all nine failed
receipts remain. Review also replaced stateful command traversal with structural
GetNextWhileExecuting traversal, owner stops and event-marker containment checks.

Only failed/affected cases were rerun. Propagate passed in headless-v2; the other
eight passed in v3. Stronger burn/Save/Global observations ran in v4, and the
final Global migration witness ran in headless-v5 and visible-v2. Eight earlier
visible-v1 passes are reused at their recorded versions. Earlier passing Global
tests with insufficient effect witnesses are preserved and excluded from the
final selected Global evidence. No claim is made that every recorded pass ran
the final driver build. The prior critical Brouwer conversion and GroundTrack
skip-boundary failures remain open and enabled.

Evidence audit
--------------

Under ``build/nasa-coverage/coverage95-20261101/``, the final bounded receipt is
``event-command-evidence-audit-v4.json``. It binds 298 consumed evidence hashes,
immutable coverage/source snapshots, archived executed sources, pre-run source
identities, selected executable identities and unchanged runtime input tables.
The executable-plan inventory is not a loaded-library observation.

Saved proofs must exactly match the full logged NASA_CHECK output. Each selected
case also has a passing CTest JUnit entry whose actual 1024-byte truncated output
prefix matches its complete process log. The cap can precede NASA_CHECK in
visible JUnit; the complete process log remains mandatory. Audit-development
failures from assuming complete JUnit output are recorded separately.

Six negative controls reject an incorrect/nonfinite orbit, ignored burn commands,
a Global initialization flag without migration, changed Global object identity,
and a stale saved state. Independent review passes the bounded capsule with
zero whole-row credit. ``event-command-independent-review.json`` and
``event-command-failures.json`` preserve the review and failure history.

Thirty-five redundant generated files were losslessly compressed, saving
59.96 MiB; the stage occupied about 35.62 MiB before final documentation/sealing.
Consumed evidence is unchanged. No PNG, runtime binary or kernel copies were
created. Generated summaries can be read with ``gzip -dc``.

Remaining inventory and original scope
-------------------------------------

``event-command-supported-inventory-v2.json`` retains all 53 runtime factory
entries, including aliases and framework names. Twenty-five entries have bounded
contained-command witnesses available across this batch and the six old branch
passes; that is not a NASA coverage percentage or a full-current-build claim.

Nineteen loaded executable types still need grouped witnesses:
CallBuiltinGmatFunction, ClearPlot, Write, MarkPoint, PenUp, PenDown, SaveMission,
Stop, FindEvents, UpdateDynamicData, Set, RunSmoother, RunEstimator, RunSimulator,
CallGmatFunction, CallPythonFunction, CommandEcho, BeginFileThrust and EndFileThrust.
Nested BeginScript/EndScript also needs a distinct context. Five framework/main-
body applicability decisions and two aliases remain explicitly open. Stop needs
an expected-stopped outcome and proof that the following command was skipped;
the completed-plus-tail-report driver cannot represent that contract.

Marker execution transparency supports the equivalence argument; it does not
replace the missing command witnesses. Legal enclosing-solver restrictions must
match the main sequence. Broader backend/science qualification remains separate.
The remaining 156 original requirements are not reduced to this command inventory.
The earlier triage still records 59 prerequisite holds, leaving an optimistic
1,099/1,158 (94.9%) ceiling until at least two holds are substantively resolved.
No external/historical authority hold was resolved by this batch.

Replay and verify
-----------------

From the checkout root::

   cmake --build build/linux-gui --target GmatQtNasaScriptEventCommandTests -j 4
   python3 src/qtgui/tests/RunScriptEventCommandAcceptance.py --visible

Restore the initially minimized GMAT test-desktop window to watch it. Omit
``--visible`` for unattended execution; use ``--filter`` to select a new/affected
context. The public helper defaults to 350 ms viewing pauses and no screenshots.
The nine cases are also registered with CTest and the acceptance build targets.

Verify the retained capsule without rerunning GMAT::

   python3 src/qtgui/tests/AuditScriptEventCommandAcceptance.py \
     --verify-receipt build/nasa-coverage/coverage95-20261101/event-command-evidence-audit-v4.json

``next-after-script-event-commands.json`` records the concrete next missing
contexts. The 95% goal remains active; this is a bounded increment, not NASA
qualification or completion of the requested percentage.
