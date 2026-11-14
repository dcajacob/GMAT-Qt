Passive Yukon state copies and callback owner identity, 2026-11-06
=================================================================

The preceding v48 vector-boundary batch made source progress. Its seventeen
final hashes and successful final build receipt were current at this turn's
start. This batch changes thirteen production paths and makes source progress;
the full goal remains active, incomplete and without a blocker.

Native state copies
-------------------

Yukon copy construction and assignment previously queried the user problem,
rebuilt the initial guess, omitted portions of the current numerical history,
and assigned meritFunction from hessUpdateMethod. Copies now transfer every
state and option member directly, with independent function-generator ownership.
They do not initialize, evaluate, optimize or reconstruct a problem.

The retained source inventory covers 73 Yukon members, 28 function-generator
members, 31 native problem-data members and all 13 option fields. This inventory
describes source copy coverage; it is not a scientific or acceptance audit.
Numerical vectors/matrices preserve sized and unsized storage. New native Swap
methods exchange storage and dimensions without allocation; fixed-size derived
types are rejected before mutation. Matrix formatting cache ownership also
exchanges. No numerical formula, tolerance, unit or original assertion changes.

Yukon and function-generator assignment stage complete independent copies before
swapping. Native problem assignment stages every allocating value before data
replacement. Primitive counters/flags/pointers now have defined defaults before
normal native construction initializes them, so passive copies do not read
uninitialized optional state. Ordinary constructors retain their initialization
and problem-evaluation behavior, with an explicit null callback guard.

Callback and owner binding
--------------------------

Yukon and its function generator expose passive callback rebinding. Both verify
the exact original pointer before changing it; Yukon verifies the pair before
publishing either binding. No name-based replacement or callback execution.
Standalone copies retain the native borrowed problem policy until explicitly
rebound by an owner; this does not manufacture ownership of external callbacks.

GmatProblemInterface records a weak lifetime identity for its actual GMAT
optimizer. Its native accesses require that owner to remain live, rather than
dereferencing an unchecked or address-reused pointer. Rebinding records the new
actual owner; null explicitly detaches it. Native problem copy transfers its
independent data and callback metadata without evaluation.

Full original obligations
-------------------------

This completes these backend copy paths, not whole Solver resource values.
Native Solver/Optimizer/Yukonad/DC/fmincon providers, derivative caches,
plot/subscriber resources, callback ownership at the wrapper, generic services,
concurrent/external lifetimes, all command/plugin and interpreted function
providers, and the complete original scientific/private/global/caller/recursive/
exceptional requirements remain open. No narrower solver or function acceptance
profile is substituted, and no original clause is waived. Solver and interpreted
provider admission were not broadened in this batch.

Compilation and accounting
---------------------------

Stage: ``build/nasa-coverage/coverage95-20261106-implementation-yukon-passive-value-state``. Thirteen production paths (one new helper
header), preimages/existence/before/build/final hashes, indexed patches, native
field inventory, source map, build logs/terminal receipts and four planning
preimages are retained. Added delta whitespace: zero.

* v1: 825/825 steps, 263.15 seconds, return 0.

Final production source matches the successful build. Utility/core/Qt/console,
enabled plugins and Python consumers rebuilt. No virtual slots were added;
GmatProblemInterface has one new weak identity member and requires matching
consumers. New nonvirtual utility/plugin copy APIs and C++17 value construction
were compiled in the configured build. External source/binary/platform and
runtime behavior remain unqualified.

No tests, GUI, missions, functions, models, solvers, numerical comparisons,
acceptance readers, seals, packages, installed/shared runtime, commits, pushes
or external messages ran. All original fixtures/assertions/tolerances/texts/
fingerprints/denominator/ledger/gates remain intact. Tracking-v49 stays
1093/1158 = 94.39%; the recorded source-versioned qualified baseline stays
1053/1158 = 90.9%. All 105 original acceptance rows remain open; 65 rows remain
outside the engineering implementation tally. No additional row or verification
credit. The full implementation pass remains incomplete.

Next
----

Join the completed passive Yukon backend copies to full native Solver/Optimizer/Yukonad/DifferentialCorrector/fmincon value and reference providers, including owned plot/subscriber resources, native derivative caches, owner/service lifetimes and explicit callback rebinding. Complete all remaining command/plugin ownership/reference/retirement providers and passive UserDefinedFunction/GmatFunction body/prototype admission. Finish snapshot cross-resource/private-formation/caller/global/recursive/exceptional bindings, all interior/scope-table/service/engine-reset/later lifetimes, GetEphemStates destination-owned-frame alias Copy, native VECTOR_WT builtin carriers/discovery/custom shape/arithmetic cases and every original body/kernel/NAIF/scientific/resource requirement. Keep the complete original scope and verification deferral. Research POV independently without asking the user for a dialect.
