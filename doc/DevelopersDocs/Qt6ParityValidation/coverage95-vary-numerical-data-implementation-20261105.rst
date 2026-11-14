Vary numerical data and refresh contract, 2026-11-05
====================================================

Vary::RefreshData allocated five Reals, while Solver::RefreshSolverVariables
read six, including data[5] as the physical initial value. That stack-array
mismatch could corrupt unscaled reporting values. Registration and refresh also
used unchecked mutable parameters/scales, and Solver checked ranges only after
partially replacing its variable arrays.

Payload and mutation repair
----------------------------

Vary now constructs one six-value std::array payload for registration and
refresh: scaled initial value, perturbation, lower bound, upper bound, maximum
step and physical initial value. Refresh retains its native unclamped initial
value; initial Execute retains its existing lower/upper clamping and warning
text. Both methods supply the sixth field explicitly. The current in-tree search
finds Vary as the only caller of these Solver variable-data methods. Their
existing six-field contract is now documented at both declarations/definitions.
No public virtual signature or layout changes were made.

Solver snapshots the incoming six fields before writing, so an input array that
aliases a solver array keeps its original values. It validates finite scaled/
physical initial values and perturbation, non-NaN bounds/step, strict lower <
upper, positive maximum step and nonzero perturbation when the provider declares
AllowIndependentPerts. Bounds and maximum step may be infinite. Negative finite
perturbations remain supported; a provider such as fmincon that declares the
perturbation unsupported retains the unused zero value behavior.

Destination indices for all seven affected arrays are checked before writes.
Registration validates its counter/name before access and increments only after
committing. Refresh validates every matching-name destination before replacing
any of them, preserves all-matching-name updates, and returns false for an absent
name. The native hasFired/RETAIN early success remains before payload checks or
writes. Vary now handles failed refresh or negative registration IDs explicitly.
The original range and maximum-step diagnostic text is retained.

Value evaluation and scaling
-----------------------------

ElementWrapper::EvaluateRealForRange adds an explicit range role: native system
parameter success is checked as in the finite calculation path, NaN is rejected,
and infinities are permitted. Ordinary calculation paths retain strict finite
results. Legacy direct EvaluateReal/ToString implementations are untouched.
Initial Vary guesses use the range role until native clamping has run, so an
infinite guess can still clamp to a finite bound. An actual unclamped infinite
control is rejected when Solver needs numerical initial data. No value is
classified as failed merely because it equals a reserved sentinel.

Vary registration/refresh read each option through the appropriate checked
finite/range path. MultiplicativeScaleFactor must be finite and positive;
AdditiveScaleFactor remains finite. Authored-option provenance and unsupported-
setting warnings/capabilities remain. The native defaults +/-9.999999e300 and
maximum-step default are unchanged. Infinity conventions are preserved rather
than narrowing bounds to finite values.

The documented formulas remain xn = m*(xd+a) and xd = xn/m-a. Ordinary finite
scaling uses the same reciprocal/division operation order as before. When the
reciprocal overflows, direct multiplication implements the same forward formula.
When a finite x+a exceeds the intermediate range, separately scaled same-sign
terms avoid that intermediate overflow. Unscaling retains normal division and
subtraction, with an fma-based equivalent numerator when the intermediate quotient
overflows but the final physical value can fit. Non-finite applied physical
values fail before assignment/report updates. These fallback paths are source
implementation, not a numerical-accuracy/rounding qualification.

Execute, restored-nominal reporting refresh and Apply Corrections share checked
unscaling and evaluate current scale operands once. A failed control/initial-
guess SetReal is handled; CurrentValue is updated after successful assignment.
Native publisher notification rules and supported initial-guess correction types
are retained. Apply Corrections remains valid after RunComplete: it checks the
retained solver and registered ID, not solverDataFinalized (which RunComplete
clears). No Vary reset/state-machine ordering is changed. Existing physical
setter errors and broader function/private-reference lifetimes still need their
own complete context review.

Compilation and source notes
------------------------------

The private integrated Release build v1 passes 323 steps / 355.43 seconds,
session 2384 completed. After the input snapshot adjustment, v2 passes 66 steps /
6.94 seconds, session 6644 completed. Enabled core, Qt, console, plugin and
language-binding clients compile/link; final source hashes match v2. XML syntax
and essential source inspection accompanied compilation; no tests, GUI,
missions/functions/models/solvers, numerical comparisons, evidence readers,
seals or qualification campaigns were run. Configured test/probe clients linked
only. A source-edit assertion caught a malformed refresh declaration during the
editing pass; it was corrected before the first build. Neither build failed.

Stage: build/nasa-coverage/coverage95-20261105-implementation-vary-numerical-data.
Six preimages, before/at-build-v1/v2/final hashes, exact patches, build logs/
receipts, summary-v1 and next-scope-notes-v1 are retained. The latter records
read-only Mission Tree scope findings and the original FRR!E470 editorial
zero-output question. It adds no UI/requirements closure: SaveMission's special
label parser rule is not a hidden tree command.

No engineering row or acceptance credit is added. Tracking-v8 remains
1089/1158 (94.04%), 69 other original rows. Recorded verified baseline remains
1053/1158 (90.9%); all 105 acceptance rows remain open. Planning dispositions,
original row text/fingerprints, assertions, fixtures, tolerances, denominator,
ledger and qualification gates are unchanged. All broader function I/O/scoping,
command contexts, OEM, body/frame, provider and specification scopes remain in
the complete implementation worklist; no completion of that pass is claimed.
