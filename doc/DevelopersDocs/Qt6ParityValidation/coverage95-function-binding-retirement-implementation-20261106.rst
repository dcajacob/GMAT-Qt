Function definition binding retirement, 2026-11-06
==================================================

This source implementation follows the v43 native callable work. All thirty-nine
v43 final source hashes and four successful build receipts were current at the
start of the turn. The goal remains active and incomplete; no blocker is claimed.

Scope and lifecycle
-------------------

RunComplete deliberately remains an ordinary execution-finalization operation.
It does not delete the Sandbox's reusable assignment wrappers. A separate
RetireFunctionBindings command contract is called by the actual owning manager
after native function finalization and before invocation objects are deleted.
The command's actual local-map address must match the retiring store. Structural
commands in branches and owned recursive stores are included. A first callback
error does not skip the later retirement walk or replace the original error.

UserDefinedFunction guards recursive retirement by actual definition identity,
marks matching compiled definitions cold and clears its matching validator table.
The next ordinary native initialization validates and reconstructs wrappers.
Parsed syntax, expression DAGs, lexical names, literals, command topology and
completed summary text survive retirement. Shared global tables are detached as
views; their entries are not cleared by command retirement.

Assignment first detaches LHS/RHS aliases to RHS-owned wrappers, including the
native x = x + 1 alias case. It then retires the RHS and deletes unique remaining
operand owners. RHSEquation drops the parsing/resource map, retires its tree and
deletes its wrappers while owned copied-scope storage remains alive. Nested
EquationWrappers destroy their owned trees/managers while the caller FOS lives.
MathTree finalizes every unique FunctionRunner before passively detaching node
references, wrapper maps, scope maps, cached function pointers and copied-scope
storage. Independent runner errors are attempted and the first error is retained.
MathTree, RHSEquation and Assignment have reentrancy guards.

MathNode adds an explicit retirement contract; its default rejects providers that
do not implement one. Native arithmetic nodes keep structural/value data, elements
clear actual Parameter and wrapper-map views, builtin nodes clear both argument
wrapper caches, and FunctionRunner clears its current environment without changing
callee names. FunctionManager's passive context retirement preserves weak actual
callee identity and invocation ownership until reentrant Cleanup completes.
Cleanup holds copied-scope storage through callee, FOS and wrapper retirement,
including when a reentrant passive detachment releases the manager's own bag.
CallFunction retires its manager and cached argument/callee views too.

Limits and remaining scope
--------------------------

This is the base/assignment/call/expression retirement path. Other command and
plugin families can have additional runtime fields and still require their own
complete providers and retirement. Interpreted function value providers remain
disabled. No whole-function or all-command requirement is closed by this batch.
Nested destructor errors keep the native manager logging behavior; arbitrary
allocator failures, concurrent destruction, engine/service/global-table reset,
interior/caller aliases and every later/recursive/exceptional/scientific lifetime
case remain unqualified. The full remaining original scope is retained.

Compilation and accounting
--------------------------

Stage: ``build/nasa-coverage/coverage95-20261106-implementation-function-binding-retirement``. Twenty-four production preimages, before/
build/final source hashes, indexed patches, source map, logs/terminal receipts
and four planning preimages are retained. New source delta whitespace: zero.

* v1: 284/284 steps, 222.51 seconds, return 0.
* v2: 66/66 steps, 6.94 seconds, return 0.

Final source matches v2. Matching core, Qt, console, enabled plugins and
Python bindings were rebuilt. One MathNode virtual slot, one GmatCommand virtual
slot and three guard fields require matching consumers. External ABI/platforms,
optional debug configurations and runtime behavior remain unqualified.

No tests, GUI/mission/function/model/solver runs, numerical comparisons, acceptance
readers, seals or qualification campaigns ran. No installed/shared runtime,
package, commit, push or external messages changed. Original assertions, fixtures,
texts, tolerances, fingerprints, denominator, ledger and gates remain intact.
Tracking-v44 remains 1093/1158 = 94.39%; the recorded source-versioned qualified
baseline remains 1053/1158 = 90.9%. All 105 original acceptance rows remain open;
65 rows remain outside the implementation-present tally. No additional row or
verification credit. Full implementation is not complete.

Next
----

Implement the remaining command-specific ownership/reference and retirement providers, then complete passive UserDefinedFunction/GmatFunction compiled-body/prototype providers before enabling interpreted function value admission. Finish retained/interior/global/caller/scope-table/service/engine-reset/later/exceptional lifetimes, GetEphemStates borrow-of-destination-owned-frame Copy, direct RVECTOR math results, all original body/kernel/NAIF/scientific and resource requirements. Research POV independently; the user has no dialect or representative models and must not be asked again. Finish implementation before verification without narrowing original requirements.
