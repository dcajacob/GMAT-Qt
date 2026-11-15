Expression references and function-scope containers — implementation, 2026-11-06
===============================================================================

MathNode gains a passive-reference provider contract. MathElement maps its actual
Parameter pointer and wrapper map without native setters that read/cache values.
Builtin nodes clear/reconnect input/output wrapper caches. FunctionRunner and its
FunctionManager bind actual callees, solar system, internal frame and environments
without mutating callee definitions through SetFunction/SetSolarSystem. Native call
definitions remain fresh; active invocation stores/callers are not checkpointed.
Native arithmetic handles its structural/value base fields; additional provider
references require an override. No opaque-provider completeness is claimed.

Local/configuration/value-owned scope tables copy by actual source table identity.
One retained scene shares storage across separate root/provider binds. Lexical keys,
null declarations, resource identities and table aliases survive. Enclosing canonical
resource copies take precedence over temporary inner clones. Native constructor phases
use isolated cold maps, preventing interim mappings from contaminating the retained
pass. Failed map/force mappings cannot later be reused as successful partial results.

Native global commands insert into their shared global store (Global::Execute and
ManageObject::InsertIntoGOS). Runtime FunctionManager forwards the force service to
its function. These externally supplied engine services retain native shared/borrowed
containers. Already value-owned tables copy instead; actual local/global table aliases
remain aliases. No artificial lifetime lease is created for external service storage.
Sandbox/global retirement and full global/formal/caller lifetime combinations remain
unqualified. Ordinary copy constructors stay cold; explicit retained binding preserves
source preparation without expression evaluation or native setter initialization.

Equation ownership and native initialization
---------------------------------------------

Active and retained equation definitions expose their owned MathTrees/nodes and value
objects. All identities register before provider binding; structural types, children
and aliases are checked. Node rebinding is deferred to the owning equation. Immediate
scope/environment lookup tickets allow already registered backlinks, while descendant
computational references keep their normal cycle rules. This does not bar legal
function recursion merely because a scope table refers to its containing resource.

Definition changes retain older owners that can still back configured-map or object
bindings. Copy/assignment includes them. Destruction clears active dependents first,
then retires older definitions newest-first. Ownership-only registration keeps real
source owners canonical before native value clones of borrowers. Nested collection
avoids repeated traversal. Pruning and complex later replacement remain open.

MathTree initialization now resets absent local/global maps and runner stores rather
than keeping earlier caller pointers. FunctionManager lookup handles absent maps,
preserves local null shadowing and array-index policy, and retains the native solar-
system fallback. Copied tree node references and functions bind by actual identity;
no name-based replacement or frozen evaluated constant is substituted.

Remaining scope and compilation
---------------------------------

Function/GmatFunction and other local resource family gates remain to implement.
Mapping a callee pointer does not prove a complete private function definition/body
closure. Direct RVECTOR math-node result production, root math-object admission,
unknown providers/private children, cross-owner public/interior roots, global/formal/
caller role aliases, service retirement, live reporting/force combinations, recursive/
exceptional/scientific behavior and all original remaining resource/command/body/frame/
kernel/NAIF obligations remain. GetMathTree handoff, internal tree mutation tracking,
parser-internal failures and exceptional allocation/Swap rollback are not qualified.
No original expression form, resource family or broad requirement is waived.

Private integrated builds pass: v1 208 steps/155.0s, v2 73 steps/6.75s, v3 89 steps/152.25s, v4 73 steps/6.99s, v5 93 steps/369.17s. Final twenty source hashes match v5;
74550/24381/17875/62183/99408 are closed. Ten optional debug/argument syntax-only
source compilations pass; the eight helper-header consumers were repeated after its
final change. Syntax sessions92513/35464/71419 closed. The new ExpressionBinding.hpp
has an absent-file preimage marker and a /dev/null patch. One MathNode virtual slot,
four native members (MathTree1,FunctionManager1,RHS1,Equation1) and one internal scene
member require consistent rebuilds; external ABI/source/platforms remain unqualified.
No new stage whitespace. Existing dirty FunctionManager whitespace remains untouched.

Exact preimages/before/build/final hashes/indexed patches/source map/notes/logs/
receipts/summary/planning preimages are under
``build/nasa-coverage/coverage95-20261106-implementation-expression-scope-binding``.
No tests/GUI/mission/function/model/solver/numerical/acceptance reader/seal execution.
No installed/shared runtime/package/commit/push changes. Tracking-v40 stays1093/1158
(94.39%); recorded source-versioned qualified baseline1053/1158(90.9%),all105 original
acceptance rows open. Assertions/fixtures/texts/tolerances/fingerprints/denominator/
ledger/gates remain unchanged. No broad row credit; active goal continues.
