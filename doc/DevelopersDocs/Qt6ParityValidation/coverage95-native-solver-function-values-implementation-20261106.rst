Native solver values across function boundaries, 2026-11-06
===========================================================

The previous v49 batch made source progress; all thirteen final hashes and its
successful 825-step build receipt were current at turn start. This batch makes
concrete source progress across twelve production paths. The full goal remains
active and incomplete, without a blocker.

Value providers and execution ownership
---------------------------------------

Solver reference support now requires an explicit native provider. Exact native
DifferentialCorrector, Yukonad (Yukon) and FminconOptimizer types supply it; other
subclasses retain the closed default. Missing providers fail rather than silently
losing derived state. Their current source does not create Solver's legacy owned
plot member; a populated member fails explicitly until its ownership provider is
implemented. No other solver or owned-plot requirement is waived.

The base provider copies all numerical/configuration/history fields, keeps file
streams closed and leaves execution uninitialized. Optimizer copies objective,
constraints, gradients and sized/unsized Jacobian matrices. DC stages independent
one-dimensional and matrix buffers using actual allocated dimensions. Its LU
index/workspace size is recorded separately from mutable registration counts;
null rows and partial allocations retain safe ownership. Buffer representations
copy without evaluating cells. No formulas, units, tolerances or assertions change.

Yukon copies both derivative calculators, optimizer state, the independent native
problem and the completed passive Yukon runtime. Actual callback owner identities
are checked before mapping both runtime callback pointers to the new owner.
fmincon retains configuration/result/iteration metadata and detaches borrowed
MATLAB/GMAT services without closing, opening or modifying the source session.

Completed Clone/Copy validate native types and rebuild retained contexts after
derived copies finish. Native constructors/operators remain cold definitions and
retire inherited contexts; self-assignment preserves state and prior destination
contexts stay alive through copies that can borrow from them. Yukon cold copies
also preserve objective/declaration labels. Unknown derived Clone/Copy require
their own complete implementation.

FunctionManager admits the three providers, prepares their data explicitly,
skips generic SolarSystem binding and execution initialization, and refuses a
missing reference provider. ObjectInitializer preserves captured solver values
through function preparation. Target/Optimize retain their existing cold-copy
initialization path when starting a run. Copies no longer publish status using
the source resource name; normal construction/initialization/execution still
publish runtime status. Solver buffer cleanup now clears unscaled values too.

Full original scope remains
---------------------------

This is not whole-function or universal solver acceptance. Other solver,
estimator, simulator, plugin, plot/subscriber/interface/measurement and command
providers, interpreted body/prototype admission, arbitrary aliases and every
private/global/caller/interior/service/engine-reset/recursive/exceptional/later
and scientific requirement remain open. Runtime behavior and scientific accuracy
have not been verified. No narrower acceptance profile replaces the original.

Compilation and accounting
---------------------------

Stage: ``build/nasa-coverage/coverage95-20261106-implementation-native-solver-function-values``. Twelve source preimages, before/build/final
hashes, indexed patches, source/data maps, logs/terminal receipts and four
planning preimages are retained. Added delta whitespace: zero.

* v1: 186/186 steps, 245.72 seconds, return 0.
* v2: 66/66 steps, 3.03 seconds, return 0.
* v3: 68/68 steps, 3.24 seconds, return 0.
* v4: 68/68 steps, 3.09 seconds, return 0.

Final source matches the final successful build. Matching core/Qt/console,
enabled plugins and Python consumers rebuilt. Two Solver virtual routes and one
DC allocation-dimension member require matching consumers; external source/ABI/
platform and runtime behavior remain unqualified. Existing Optimizer oldCost has
a defined cold default. No new scientific algorithm or tolerance.

Only compilation and essential source debugging ran. No tests, GUI, missions,
functions, models, solvers, numerical comparisons, acceptance readers, seals,
packages, installed/shared runtime, commits, pushes or external messages ran.
Original assertions/fixtures/tolerances/texts/fingerprints/denominator/ledger/
qualification gates remain intact. Tracking-v50 stays 1093/1158 = 94.39%; the
recorded source-versioned qualified baseline stays 1053/1158 = 90.9%. All 105
original acceptance rows remain open; 65 rows remain outside the engineering
tally. No additional row or verification credit. Full implementation incomplete.

Next
----

Complete native providers for every remaining solver/estimator/simulator and other resource family, owned plot/subscriber/interface/measurement resources, all command/plugin ownership/reference/retirement paths and passive UserDefinedFunction/GmatFunction body/prototype admission. Finish snapshot cross-resource/private-formation/caller/global/recursive/exceptional bindings, arbitrary ownership/interior/scope-table/service/engine-reset/later lifetimes, GetEphemStates destination-owned-frame alias Copy, native VECTOR_WT builtin carriers/discovery/custom shape/arithmetic and every original body/kernel/NAIF/scientific/resource requirement. Keep complete original scope and verification deferral. Research POV independently without asking the user for a dialect.
