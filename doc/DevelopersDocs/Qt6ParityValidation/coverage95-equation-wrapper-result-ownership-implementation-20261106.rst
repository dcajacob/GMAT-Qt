Equation definitions and result ownership — implementation, 2026-11-06
======================================================================

EquationWrapper native copies retain their base metadata, expression tree, type,
reference declarations and map bindings. Copies remain cold and drop previous results.
Definition, reference, rename and clear operations invalidate readiness/cache. Successful
native initialization associates dependencies; each typed getter then evaluates the live
expression. Scalar, matrix, row/column vector views, string and object getters return the
current result or throw an error; no stale cached zero is substituted. Explicit
EvaluateEquation computes a result; a subsequent getter performs another fresh evaluation.
Object-result pointers remain valid until the next evaluation, mutation or destruction.

Generated array/object values now have explicit owners in ArrayWrapper/ObjectWrapper.
Ordinary resource bindings still borrow. Native owned copies are staged/type/source
checked; assigning from a wrapper borrowing the destination-owned identity keeps that
owner. RHS output creation stages all allocations. Assignment holds returned wrappers
with RAII on all paths, removes manual underlying-object deletion and repeated cleanup,
and rethrows errors previously swallowed by its currentFunction reporting branch.

RHS mutation/finalization/release/copy invalidates readiness. Successful native Initialize
restores it; missing roots and undefined type metadata fail safely. Declared matrix
shape checks remain strict. Scalar/one-cell matrix conversion is preserved, and the
existing ElementWrapper matrix-to-vector row/column policy is reachable. RHS rename
also updates the expression string. Native nested equation initialization is connected.
Validator remains responsible for supplying wrappers; no singleton/map-changing
implicit Validator calls were added.

Located ArrayElement/Equation child graphs participate in ownership inventories and
cycle preflight. Actual source owners register before native value clones of borrowed
aliases, so canonical ownership no longer depends on wrapper pointer order. Cycle
checks examine structural wrapper ownership; function reentry is not rejected merely
because the same compiled wrapper appears in another invocation. Full recursive scope
implementation/qualification remains open.

Remaining original scope
--------------------------

EquationWrapper still needs its passive retained-reference provider. A native RHS copy
containing a nested EquationWrapper can still stop at that explicit unsupported provider
boundary. MathElement actual refs and FunctionRunner/callee/scope/runtime environments
remain borrowed pending native initialization or future passive binding. Direct RVECTOR
math-node result production remains missing; vector views of row/column matrix results
use the existing conversion policy. No expression form or broad original clause is waived.

Unknown providers/private children, cross-nested/interior/container aliases, direct setter
retirement, GetMathTree ownership transfer, internal MathTree mutation tracking, parser-
internal exceptions, exceptional allocation/Swap rollback and all other original resource/
command/private/global/body/kernel/NAIF/later/recursive/scientific obligations remain.
Source inventory is not a runtime universe or whole-function-value completeness proof.

Compilation and status
------------------------

Private integrated builds pass: v1 79 steps/6.03s, v2 323 steps/173.11s, v3 323 steps/175.74s, v4 66 steps/2.73s.
Final eleven source hashes match v4. Sessions95665/6304/97094/35572 and syntax
session19986 are closed. Six final-source syntax-only optional debug branches pass:
ArrayWrapper, ObjectWrapper, EquationWrapper, ElementWrapper, RHSEquation and Assignment.
No whitespace errors. Four new native fields (Array1,Object1,Equation1,RHS1), exported
EquationWrapper and nonvirtual helpers require matching rebuilds; no new virtual slots.
The original three-argument inventory helper remains. External ABI/source/platform
behavior is unqualified.

Exact preimages/build/final hashes/indexed patches/source map/notes/logs/receipts/summary/
planning preimages are under
``build/nasa-coverage/coverage95-20261106-implementation-equation-wrapper-result-ownership``.
No tests/GUI/mission/function/model/solver/numerical/acceptance reader/seal execution.
No installed/shared runtime/package/commit/push changes. Tracking-v39 stays1093/1158
(94.39%); recorded source-versioned verified1053/1158(90.9%),all105 original acceptance
rows open. No broad row credit. Assertions/fixtures/texts/tolerances/fingerprints/
denominator/ledger/gates remain. Goal active; finish implementation before verification.
