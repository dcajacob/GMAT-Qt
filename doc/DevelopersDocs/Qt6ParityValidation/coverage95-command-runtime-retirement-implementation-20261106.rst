Core command runtime retirement, 2026-11-06
================================================

The preceding v44 turn made concrete source progress; its twenty-four final source
hashes and two successful build receipts were current at turn start. This turn
continues implementation. The goal is active and incomplete, with no blocker.

Runtime views and ownership order
---------------------------------

BranchCommand, ConditionalBranch, While, For, SolverBranchCommand, Target,
Optimize, Vary, Achieve, NonlinearConstraint, Minimize, Report, Write and Toggle
now override the existing actual-store retirement contract for their own fields.
Parsed declarations and command topology survive. Runtime wrapper and object
views retire while the owning function's local objects still exist. Retiring
one store does not detach commands bound to another actual store.

UserDefinedFunction builds a postorder from the native owned command links.
Borrowed BranchEnd navigation backlinks are excluded; cyclic ownership rejects.
Children retire before parent-owned loop snapshots. Branch cursors/function-command
caches, conditional and For operands, solver-control wrapper/solver views,
reporter/writable/parameter/subscriber views and callback registrations retire.
Solver initialization flags and registration IDs return to cold state. While
retires condition wrappers before its owned elapsed-time Parameters on validation,
assignment, destruction and function retirement. Ordinary RunComplete retains its
reusable Sandbox wrapper policy.

Native copy and wrapper corrections
-----------------------------------

ConditionalBranch copies all lexical/operator arrays and sized empty operand
slots; assignment no longer appends duplicated operator text. Report and Write
also copy complete declaration slot counts and base state, rebuild empty native
wrapper slots, and reset reporter/run caches. Write assignment no longer shares
its source's owned wrappers and cached writable objects. For assignments retire
old wrappers and copy a cold loop cursor. Achieve/NonlinearConstraint/Minimize
assignments retire wrappers before overwriting pointers. Solver control IDs and
Vary readiness reset; Vary retains the structural last-variable marker. Solver
mode/exit option arrays now copy, and cold Report/Write rename paths skip empty
wrapper slots until native validation reconstructs them.

Report/Write ClearWrappers now actually deletes unique owned wrappers while
preserving declaration slots. Replacement handles setting an already installed
wrapper and keeps any old wrapper still used by another declaration slot. The
common old-wrapper collector clears every alias role even if its owner was
already collected; NonlinearConstraint cleanup uses that corrected owner path.
No function, solver or expression was executed to obtain replacement values.

Remaining full scope
---------------------

This batch covers the listed native lifecycle fields. The interpreted provider
is still disabled. Full retained wrapper values, command/plugin-specific resource
providers, solver instance ownership and arbitrary cross-owner/destination-owned
aliases remain required; native command copies here are cold definitions, not
active execution checkpoints. Solver definitions remain with their native command
owner until ordinary replacement/destruction, and require their full retained
ownership/reference implementation. No all-command or whole-function row closes.

Owning-source review also located a next obligation: AddAutomaticObject can tag a
managed current-FOS object as a borrowed sandbox object and put its pointer into
an empty automatic prototype slot. That slot can survive FOS retirement. Its
actual identity/lifetime and declaration-reconstruction path must be fixed before
claiming complete safe compiled-definition reuse/copy. No same-name substitution
or silent narrowing of the requirement is accepted.

Compilation and accounting
---------------------------

Stage: ``build/nasa-coverage/coverage95-20261106-implementation-command-runtime-retirement``. Thirty production preimages, before/
build/final source hashes, indexed patches, source map, logs/terminal receipts
and four planning preimages are retained. New source delta whitespace: zero.

* v1: 103/103 steps, 148.05 seconds, return 0.
* v2: 71/71 steps, 3.5 seconds, return 0.

Final source matches v2. Matching core, Qt, console, enabled plugins and
Python bindings were rebuilt. Existing virtual retirement/wrapper slots are overridden; no new fields or base
virtual slots are introduced by this batch. Matching enabled consumers rebuilt. External ABI/platforms,
optional debug configurations and runtime behavior remain unqualified.

No tests, GUI/mission/function/model/solver runs, numerical comparisons, acceptance
readers, seals or qualification campaigns ran. No installed/shared runtime,
package, commit, push or external messages changed. Original assertions, fixtures,
texts, tolerances, fingerprints, denominator, ledger and gates remain intact.
Tracking-v45 remains 1093/1158 = 94.39%; the recorded source-versioned qualified
baseline remains 1053/1158 = 90.9%. All 105 original acceptance rows remain open;
65 rows remain outside the implementation-present tally. No additional row or
verification credit. Full implementation is not complete.

Next
----

Resolve managed automatic prototype entries that can outlive their invocation FOS, with actual identity and persistent declaration ownership preserved. Complete solver ownership/retained references and all remaining command/plugin ownership/reference/retirement providers, then passive UserDefinedFunction/GmatFunction body/prototype admission. Finish retained/interior/global/caller/scope-table/service/engine-reset/later/exceptional lifetimes, GetEphemStates destination-owned-frame alias Copy, direct RVECTOR results and every original body/kernel/NAIF/scientific/resource requirement. Keep full original scope and verification deferred until implementation is finished; research POV independently without asking the user for a dialect.
