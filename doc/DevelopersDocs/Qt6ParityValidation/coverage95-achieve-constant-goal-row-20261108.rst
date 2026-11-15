FRC!E97 numeric Achieve Goal admitted
===================================

The source-versioned acceptance union is now **1,078/1,158 (93.1%)**.
There are **80 original requirements open** and **23 more needed** for the
unchanged 1,101-requirement / 95% target. Historical operation acceptance
remains 1,816/2,367.

Only FRC!E97 is newly admitted. Its original Real number leaf appears under
the Achieve control-variable list. The full FRC!E90-E97 context is retained,
including the distinct Parameter, Variable and array-element siblings. Actual
numeric measured Goal support is verified on the left of the equals sign;
existing numeric desired-value support alone is not substituted for it.

Whole-row evidence
------------------

AllowConstantGoal=true accepts a generic finite NumberWrapper as the measured
Goal. Default and explicit false preserve reference-goal behavior and numeric
Goal rejection. The command validates finite measurements, desired values and
positive finite tolerance before updating solver results. Constants retain
their literal value; this does not invent mutable literal Vary controls.

Eight native cases verify default/explicit false, policy cloning/assignment
and serialization, satisfied fractional/scientific literals, an inconsistent
constant target and overflow rejection. The inconsistent case has measured
1.25 and desired 2.25, fails without executing its successor and retains the
constant measurement while its control changes. It therefore establishes the
actual measured position independently of the desired RHS.

The new actual GUI workflow and unchanged original Achieve Parameter tests
pass both profiles. They cover the option checkbox and dynamic Goal selector,
nonfinite inputs, invalid Apply/OK, draft Cancel, exact Undo/Redo, Unicode
Save/reopen, default reference and final explicit-false compatibility. There
are 25 native scalar checks, 60 new GUI report/measurement checks, 16 original
report scalar checks, 16 actual selector checks and 16 invalid Apply/OK checks.
Visible execution records 58 native exposures and zero screenshots.

The component also repaired a missing real-value getter dispatch for the
existing read-only REAL AchievedValue field. Both accessor paths now return
the same stored value. The two pre-repair runs, including five accessor
failures each, remain preserved; solver results already obtained through
GetNumber are identical after the repair. No formula or numerical bound changed.

See ``coverage95-achieve-constant-goal-verification-20261108.rst`` and
``build/nasa-coverage/coverage95-20261108-achieve-constant-goal-validation``
for the complete component and exact source/runtime versions.

Admission and limits
--------------------

Admission stage:
``build/nasa-coverage/coverage95-20261108-achieve-constant-goal-row-v1``.
Its 72 frozen readers reconstruct the preceding union and admit exactly
FRC!E97. Twelve incomplete or altered admission controls reject, including
substitution of RHS evidence for the measured operand. The independent frozen
replay produces a byte-identical ledger without executing native missions.

The final source capsule contains 2,735 paths, including all 1,334 protected
original inputs and assertions. Only this row changes in the full audit; the
other 1,157 full-row and 138 closure records are unchanged. All original
fingerprints, tolerances, denominators and the 1,101 target remain. All 59
separate formal hold identities/statuses are preserved: 42 resolved and 17
unresolved, including the same six unresolved MATLAB holds.

The sealed prefix, receipt, manifest, exact delta and remaining-row list are
in the admission stage. Its ``verify-stage-v1.py`` replays the sealed union
with archived readers and source versions.

FRC!E93's whole Achieve type parent and the literal Vary control requirements
remain open. No full solver science, physical frame/ephemeris, function scope,
external ABI, platform/native OS input, current-build or full NASA approval
is inferred. The campaign goal remains active.
