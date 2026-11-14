Expression node graphs — implementation, 2026-11-06
===================================================

MathTree now owns an independent node graph after native copy/assignment. Copies
inventory every structural identity before rebinding, preserve shared subexpressions,
check native clone types and source/peer/reused pointers, and reject ownership cycles.
Staged unique ownership survives allocation/binding failures. Replacing the top node
retires only old nodes outside the replacement graph; cleanup deletes each node once.

Two appended MathNode virtual contracts expose structural children and reconnect
copies. FunctionRunner includes its separate argument-node array, previously missed
by tree walkers. Thirteen setup, cleanup, rename and reference-name walkers now visit
unique structural nodes, including those optional argument subtrees. Null local or
global maps produce the native unresolved-variable error rather than dereferencing
null. Function binding rejects null/wrong-type resources.

FunctionRunner copies its native call definition through FunctionManager's existing
definition-copy policy, detaches invocation maps/caller and retains names/argument
metadata. BuiltinFunctionNode and MathElement retain definition arrays; wrapper maps
are detached. This is not an active-invocation checkpoint or completed function-value
expression binding. RHSEquation/EquationWrapper ownership, actual object references,
scope maps, callee/runtime contexts and unknown providers remain to implement.
All supported numeric, matrix, vector, string, builtin and function expression forms
remain in scope. No expression is replaced by a cached numeric constant.

Private integrated builds pass: v1 195 steps/148.82s, v2 66 steps/2.8s, v3 67 steps/2.98s, v4 66 steps/2.94s. Final eight source hashes match v4;
all four build sessions closed. MathTree and FunctionRunner also syntax-compile with
DEBUG_MEMORY/DEBUG_FUNCTION/math-init/wrapper/rename and both optional argument macros
(0.50s and0.44s). V4 is whitespace only. No new whitespace errors. Matching core and
provider rebuilds are required for the appended virtual contracts; external ABI and
other platform behavior are unqualified.

Exact preimages, before/build/final hashes, patches, source map, notes, logs, receipts
and planning preimages are under
``build/nasa-coverage/coverage95-20261106-implementation-math-tree-node-graphs``.
No tests, GUI, mission/function/model/solver execution, numerical comparisons,
acceptance readers or seals ran. No installed/shared runtime, packages, commits or
pushes changed. Tracking-v37 remains1093/1158 (94.39%); recorded source-versioned
verified baseline1053/1158 (90.9%), all105 original acceptance rows open. No broad
parent credit. Original assertions, fixtures, requirement text, tolerances,
fingerprints, denominator, ledger and verification gates remain unchanged.
