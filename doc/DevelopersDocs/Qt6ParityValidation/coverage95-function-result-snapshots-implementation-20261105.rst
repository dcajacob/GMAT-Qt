Function-expression result snapshots, 2026-11-05
================================================

This batch repairs concrete function I/O/lifetime paths found while reviewing
FRR!E406, E450 and E491. It does not complete those broad rows or add requirement
credit. Tracking stays 1087/1158 (93.87%); recorded verification stays 1053/1158
(90.9%) and all 105 original acceptance rows remain open. The full implementation
goal remains active and runtime verification remains deferred.

Failed expression calls
------------------------

FunctionRunner delegates real, matrix and object expression results to
FunctionManager. Evaluate, MatrixEvaluate and EvaluateObject previously ignored
Execute's Boolean result. A failed invocation could therefore fall through to
a previously cached value/type; an old object pointer could have been invalidated
by cleanup. All three evaluators now require successful execution before reading
the result. Execute invalidates the old typed result/view at entry, including
before an absent-function error. Native exception propagation is preserved.

The preceding owned storage is not destroyed at entry: a C++ caller may supply
the preceding result as an input to the next invocation. It remains available
during argument materialization, and is replaced by a new object snapshot or
retired during cleanup/destruction. An invalidated view is never returned as a
successful new result. The user-facing function help now states that a failed
call does not substitute an earlier expression result.

Local object result values
----------------------------

SaveLastResult previously stored the local output object's raw address before
HandleCallStack/PopFromStack restored the caller's local frame. Restoring an
aliased local object could erase the returned value before the expression used
it. Function-owned object results now receive an owned Clone snapshot before
that restoration. Null results, null Clone and an invalid self-returning Clone
fail explicitly without deleting the source. Ordinary true globals and borrowed
GroundStations retain their native borrowed identity under the existing function
ownership policy.

EvaluateObject returns a borrowed view of the completed result, valid until the
next invocation or finalization. The manager owns a local snapshot; a caller
does not acquire deletion ownership. Manager copies start without invocation
results, assignment clears old snapshots, and cleanup/destruction retire result
storage before its borrowed local dependencies are deleted. Cleanup establishes
its existing re-entry guard before destroying function/command-valued snapshots.

The snapshot owns the returned root value, not a recursively transferred
dependency graph. Spacecraft/frame/model references inside a returned resource
still need caller-name translation and ownership/lifetime handling. Named output
Copy routes and nested multi-output restoration also remain in the broader
review. This batch must not be presented as complete arbitrary-object return
support or an FRR!E406 final-pass-policy solution.

Detached function-definition copy ownership
--------------------------------------------

Generic result snapshots also reach GmatFunction::Clone. UserDefinedFunction's
copy constructor already detached its command graph, but shallow-copied the
source's functionObjectMap and its initialized/finalized flags. Its destructor
calls ClearFunctionObjects, which deletes locally owned templates in that map.
A detached copy could therefore delete source-owned templates on destruction.

A detached definition copy now starts with an empty parser-owned template map,
fresh command-graph/runtime flags and no borrowed ownership of the source map.
Its path, source-format, signature and other configuration are still copied by
the existing base constructors. Templates belong to the graph rebuilt from that
definition, rather than a detached copy of another graph's ownership map. This
does not claim a complete audit of definition assignment, parser rebuilding or
all resource Clone implementations.

Build and retained artifacts
------------------------------

Stage: ``build/nasa-coverage/coverage95-20261105-implementation-function-result-snapshots``.
FunctionManager gains a private unique object-result owner; public signatures,
script grammar, original assertions and numerical tolerances are unchanged.
Its layout change requires rebuilding all users together. Exact source preimages,
at-build source hashes/patches, build logs/receipts and final patch/hash/summary
artifacts are retained. Build v1 passes 89 steps in 228.64 seconds (session 42961 completed). Build v2
passes 67 steps in 5.20 seconds (session 5981 completed). The final executable
sources match build v2. No build sessions remain live.

Only compilation and essential owning-source debugging were performed. No test,
GUI, script/function/model/solver execution, numerical comparison, acceptance
reader/seal or qualification run. Test/probe clients are linked only. No original
fixture/assertion/tolerance/text/fingerprint/denominator/ledger/gate changed. No
installed/shared runtime, package, commit or push update. Windows/macOS behavior
is unqualified.

Later verification must cover success followed by false/exceptional calls for
all three expression types, local/global/borrowed object results, recursively
aliased caller maps, reuse of a result as the next input, missing/failed Clone,
definition-copy destruction, cleanup re-entry and dependency lifetimes. Finish
the remaining implementation scopes before returning to those checks.

Definition-assignment source follow-up
---------------------------------------

UserDefinedFunction assignment does return *this. It copies readiness flags and
detaches fcs without a complete graph/template lifecycle transition; GmatFunction
assignment currently bypasses that layer. Sandbox::HandleGmatFunction builds
missing command graphs through Moderator::InterpretGmatFunction and binds nested
functions and event managers during sandbox setup. A runtime replacement also
needs function-manager argument/store invalidation and an appropriate rebuilding
route. Do not treat clearing fcs alone as implementation, or delete an executing
graph. This remains unfinished and adds no credit.
