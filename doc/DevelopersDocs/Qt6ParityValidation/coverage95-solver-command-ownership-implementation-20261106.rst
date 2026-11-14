Solver command ownership and exact snapshot targets, 2026-11-06
===============================================================

The prior v46 turn made source progress. Its nine final hashes and two successful
build receipts were current at turn start. This turn makes concrete source
progress; the full goal remains active and incomplete, without a blocker.

Local solver ownership
-----------------------

Sandbox clone updates previously replaced the command-owned solver pointer with
the configured/master pointer. Later command deletion could delete that master,
while child controls still referenced the previous local instance. Updates now
copy into the actual local solver and require normal owner reinitialization;
single-parameter updates validate the native type and handle an absent instance.
Target and Optimize stage a native cold definition and keep one owned instance
per actual solver class/type. Ordinary reinitialization preserves that instance's
identity, and changing supported classes does not delete an instance still
referenced by another command or Parameter. Nested active-call dispatch remains
on its existing native path.

The solver resource bag owns solver instances and iteration snapshots. A generic
GmatCommand lifetime holder keeps it alive through owned branch and following
command destruction. Source native ownership is disclosed by GetCommandOwnedObjects
and included in UserDefinedFunction ownership inventory. Cold command definitions
do not share another command's runtime bag. Target assignment now copies its full
SolverBranchCommand configuration; assignment clears prior snapshot and listener
views without losing their unique owners.

Iteration snapshots
---------------------

StoreLoopData stages fresh native clones for spacecraft/formations from actual
local/global maps. Aliases of one actual object share one snapshot; different
objects with identical names remain separate. Null scope entries are preserved as
lookup absences. Source/peer/native-type clone guards run before ownership adopts
any clone. All captures complete before the previous snapshot set is replaced.
Ordinary engine frame associations retain their native policy, while retained
values keep their completed native copy/context binding.

ResetLoopData validates actual lifetime tokens, native type and current scope
membership for the complete restore set before applying full virtual native Copy.
It does not search for a replacement object with the same name. Propagators then
receive the existing native ResetLoopData action. This also removes the former
GmatBase-only assignment used to refresh an existing spacecraft snapshot, which
could leave its derived state stale. Normal cleanup and destruction keep unique
snapshot owners; raw localStore entries are only views.

Copy, allocation and error paths
--------------------------------

Solver assignment now closes its previous stream, retires its owned plot and
clears cold variable storage like the native copy constructor. The separate nested
state, exit-mode text and saved unperturbed scalar copy correctly; the scalar has
a defined default. Optimizer assignment carries its objective-defined flag.
ExternalOptimizer copies rebuild source readiness, and fmincon copy construction/
assignment detaches its borrowed MATLAB interface until normal initialization.
The shared engine interface is not closed by this change.

DifferentialCorrector frees old arrays before base assignment changes counts.
It additionally records allocated variable/goal row dimensions so re-registration
or partial initialization cannot make cleanup use the wrong row counts. Row pointer
arrays initialize to null for partial-allocation cleanup. Numeric tolerances,
assertions, fixture expectations and scientific algorithm formulas are unchanged.

Target/Optimize finalization attempts child cleanup and frees loop snapshots even
if solver finalization fails, retaining the first error. SolverBranchCommand clears
listener views even if a child RunComplete reports an error. Retiring a function
scope also detaches the base command's cached summary-frame reference.

Full remaining scope
----------------------

This is native command ownership and snapshot-target implementation. It does not
implement all solver resource value/reference providers or admit interpreted
function values. Cross-resource/private formation/caller/global/recursive snapshot
bindings, arbitrary interior and mutation aliases, service/engine reset, all
remaining command/plugin providers and scientific/exceptional behavior remain
required and unqualified. Native cold definitions are not active execution
checkpoints. No narrower whole-function/all-command profile is declared complete,
and no original requirement is waived.

Compilation and accounting
---------------------------

Stage: ``build/nasa-coverage/coverage95-20261106-implementation-solver-command-ownership``. Thirteen production preimages, before/
build/final source hashes, indexed patches, source map, logs/terminal receipts
and four planning preimages are retained. New source delta whitespace: zero.

* v1: 228/228 steps, 209.09 seconds, return 0.
* v2: 69/69 steps, 5.36 seconds, return 0.

Final source matches v2. Matching core, Qt, console, enabled plugins and
Python bindings were rebuilt. One appended GmatCommand ownership virtual and four instance fields (command
lifetime owner list, solver resource bag, two allocation dimensions) require
matching core/plugin consumers. Enabled targets were rebuilt; external source/ABI
compatibility remains unqualified. External ABI/platforms,
optional debug configurations and runtime behavior remain unqualified.

No tests, GUI/mission/function/model/solver runs, numerical comparisons, acceptance
readers, seals or qualification campaigns ran. No installed/shared runtime,
package, commit, push or external messages changed. Original assertions, fixtures,
texts, tolerances, fingerprints, denominator, ledger and gates remain intact.
Tracking-v47 remains 1093/1158 = 94.39%; the recorded source-versioned qualified
baseline remains 1053/1158 = 90.9%. All 105 original acceptance rows remain open;
65 rows remain outside the implementation-present tally. No additional row or
verification credit. Full implementation is not complete.

Next
----

Complete full native solver value/reference providers, remaining command/plugin ownership/reference/retirement providers and passive UserDefinedFunction/GmatFunction body/prototype admission. Finish snapshot cross-resource/private-formation/caller/global/recursive/exceptional bindings, all interior/scope-table/service/engine-reset/later lifetimes, GetEphemStates destination-owned-frame alias Copy, direct RVECTOR results and every original body/kernel/NAIF/scientific/resource requirement. Preserve original scope and keep verification deferred until implementation is finished. Research POV independently without asking the user for a dialect.
