Checked numerical parameter values, 2026-11-05
===============================================

The scalar family inventory found native system-parameter Evaluate methods that
return false and leave a reserved finite undefined value, while their legacy
EvaluateReal methods return the cache without checking success. Finite-value
checks alone could therefore admit a failed B-plane or other calculation to a
solver. The source also contained actual evaluation paths that submitted dummy
objective/constraint values when a wrapper was missing.

Calculation interface and compatibility
----------------------------------------

ElementWrapper now has two public nonvirtual methods:
EvaluateRealForCalculation and EvaluateParameterRealForCalculation(Parameter*).
They add no fields, virtual table entries or new parameter whitelists. A native
system Parameter returning REAL_TYPE and derived from RealVar is evaluated once
through its virtual Evaluate success contract. A false result raises
ParameterException; success reads the evaluated cache through GetReal. The
checked result must be finite. This follows the concrete source-family protocol
already mapped for the 97 numeric names / 92 real classes.

The implementation does not guess failure by comparing a number with a sentinel.
User Variables, Number/Array-element/property wrappers and non-RealVar plugin
Parameters retain their own value evaluators, followed by a finite-result check.
The native scalar conversion of a 1x1 Array remains available where the caller
already supports it. Null/unbound Parameter operands fail explicitly. Custom
plugin evaluator error semantics remain part of their provider contracts.

Legacy EvaluateReal and direct ToString implementations themselves are unchanged.
Direct report formatting can retain its existing undefined-value presentation.
Expression calculations that consume an invalid operand now follow the checked
numeric path. Initial solver metadata registration still uses its native value
lookup: it can precede propagation or a maneuver that makes a later goal defined.
No new all-branch execution, physical penalty value, convergence rule or dynamic
solver-problem interpretation is introduced.

Consumer changes
------------------

* MathElement's scalar operand path uses the checked method and raises an error
  if its numerical wrapper is absent. It does not claim validation of every
  matrix operation or every arithmetic operator's output.
* Achieve actual nominal/perturbation updates evaluate desired value, measured
  goal and tolerance through the checked path before updating targeter data.
  Its native metadata lookup remains. Missing targeter/field wrappers now fail
  before debug or evaluation dereferences. Existing finite/positive-tolerance
  checks and update order remain.
* Minimize requires an optimizer and objective in Execute, and requires its
  objective wrapper before Initialize dereferences it. Actual objective values
  use checked evaluation. The missing-objective branch no longer writes
  -999.999 as a successful objective. Native initial registration lookup remains.
* NonlinearConstraint requires its optimizer and both operands; Initialize checks
  operands before dereference. Its actual pass evaluates desired, achieved and
  optional tolerance exactly once into locals, forms the native residual, and
  rejects non-finite results before solver/report updates. Missing operands no
  longer submit -999.99 as a successful residual. Invalid comparison enums fail.
  The optional finite nonpositive tolerance still warns once and uses the
  optimizer feasibility tolerance. Missing tolerance still passes -1. The
  existing residual signs and delta-tolerance formulas remain. The native 54321
  registration placeholder is retained as registration data, not an actual
  missing-input calculation result. Debug formatting is corrected to use %p for
  the optimizer pointer. Derived display-only metrics are not claimed to be a
  complete numerical-range audit.
* StopCondition uses the same checked route for measured scalar wrappers and
  direct C++ Parameter operands. Active goal/epoch reads, buffers, stop goals,
  differences and base-epoch setup are checked. Non-finite epochs/goal sums and
  stop differences fail before range/interpolation use. BaseEpoch rejects
  non-finite input before assignment. GetStopDifference evaluates its measured
  value once even with debug output enabled. Native cyclic, sampling, root,
  first-step, tolerance and time formulas are retained.

Propagate sets BaseEpoch from PrepareToPropagate, after ordinary preceding
mission commands. StopCondition's initial metadata-only RHS lookup remains on
the legacy route; runtime calculations check success later. Apoapsis/Periapsis
Evaluate use a success flag for valid radial geometry rather than the event's
truth value, so the checked interface does not confuse a non-event with failure.
The four command reference pages describe successful finite calculation inputs
and the existing constraint feasibility-tolerance fallback.

Compilation and remaining work
--------------------------------

The private integrated Release build v1 passes 323 steps / 217.91 seconds,
session 22129 completed. After the two-file stopping extension, v2 passes 66
steps / 2.94 seconds, session 21266 completed. Enabled core, Qt, console, plugins
and language-binding dependents compile/link. Final source hashes match v2.
The source-at-build-v1 snapshot and separate stopping preimages preserve that
intermediate version. XML syntax and essential source inspection accompanied
compilation. Configured probe/test clients linked only.

No tests, GUI, mission/function/model/solver execution, numerical comparisons,
proof readers, seals or qualification campaigns were run. Runtime/model accuracy,
provider recovery for invalid probes and all error paths remain unverified.
Vary's registration/refresh, scaling/unscaling, report refresh and Apply
Corrections still use legacy value reads and need the next owning-source pass.
Other numerical consumers, custom plugin contracts, returned private dependency
ownership, solver/kernel contexts and the rest of the worklist remain open.

Stage: build/nasa-coverage/coverage95-20261105-implementation-checked-numerical-values.
Eleven source/help preimages, initial/stopping-extension before hashes,
at-build-v1/v2 and final hashes, exact patches, logs/receipts and summary-v1.json
are retained. No new engineering-row or acceptance credit is added. Tracking-v8
remains 1089/1158 (94.04%), with 69 other original rows. Recorded verified baseline
remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Planning
counts and original row texts/fingerprints, assertions, fixtures, tolerances,
denominator, ledger and qualification gates remain unchanged.
