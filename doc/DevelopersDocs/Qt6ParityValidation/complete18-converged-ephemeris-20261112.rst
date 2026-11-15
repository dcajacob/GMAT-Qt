Converged ephemeris output and initial-guess epoch
=================================================

The original requirement says iterative output must contain a solution. Target
and Optimize previously replayed their last attempt even when their solver
reported an iteration-limit failure. Ephemeris output now also checks an internal
scoped convergence condition. The scope is shared across core/plugin boundaries,
preserves nested solver context, and does not alter the numerical publisher,
reports, plotting, or background event-search recorders. Explicit RunInitialGuess
is exempt because it requests an initial trajectory rather than an iterative solve.

A supplementary control exposed another defect: nested Target initialization
could move the spacecraft before RunInitialGuess saved its starting state. The
snapshot now precedes that initialization, matching normal Solve mode.

Validation
----------

* The four failed/recovered Target and Yukon contexts preserve byte-identical
  diagnostic reports. Failed solves produce zero ephemeris rows; recovered solves
  produce the expected 25 independently checked circular-orbit rows.
* Both explicit initial-guess contexts produce 25 rows. The Target epoch failure
  is retained; its single repaired control passes. Five already passing contexts
  were reused without re-execution after that narrow fix.
* The existing batch iteration-limit test passes: no output for the unconverged
  fit, then two recoveries with 1,435 independently checked rows each. This was
  one selected dependent test; 4,573 registered tests were not run.
* Original protected assertions and bounds are unchanged. The previously
  authorized ASCII title helper remains the only changed protected input.

Admission remains open
----------------------

The old seven-provider inventory omitted the now-enabled MATLAB Fmincon
optimizer. Its ephemeris route still needs an executed witness. The native
IN_TOLERANCE success state is accepted alongside CONVERGED because it represents
satisfied scripted physical tolerances; a dedicated output witness is pending.
The current stage does not claim whole-row, whole-current-build, or scientific
qualification. The source-versioned requirement total remains 1,112/1,158,
with 11/18 agreed rows complete.

Evidence: ``build/nasa-coverage/complete18-20261112-converged-ephemeris-v1``.
