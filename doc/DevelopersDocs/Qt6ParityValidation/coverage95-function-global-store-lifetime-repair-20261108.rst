Function global-store lifetimes and copied display initialization, 2026-11-08
============================================================================

The seven retained original post-call global-store failures now pass unchanged
on the current build. Both recorded resource catalogs, totaling 73 cases, pass
headlessly and visibly. Broader verification also exposed and repaired a real
DynamicDataDisplay initialization recursion. The original visible VF13ad Central
progress test had one editor-activation failure; its unchanged diagnostic rerun
passes, but the intermittence remains open. No whole scoping or NASA row credit
is claimed. Canonical acceptance stays 1078/1158 (93.1%), 80 open, 23 needed for 95%.

Actual global views without live invocation retention
----------------------------------------------------

Previously RetireFunctionObjectStore correctly retired the invocation but also
removed the only inspectable global-store view. All seven current original cases
reproduced their global-store-absent assertion. Native field values, flags and
caller identity checks before that assertion already passed.

ObjectStoreLifetime registers an actual owner's map generation. Sandbox binds
its local/global maps during Initialize and expires those identities before
Clear deletes referents, and at destruction. Tokens are weak in observations;
they do not retain the map, its objects, a Sandbox or a Function. Rebinding the
same address creates a different identity. Duplicate live ownership is rejected.
The existing FunctionManager weak-owner protocol still handles manager stores.

ObjectManagedFunction records the actual registered global binding separately
from its invocation state. Its getter provides the active binding, or a guarded
last-bound global view after invocation retirement. Unbound nested activations
cannot borrow an enclosing observation. Nested restoration restores the proper
observed scope and rejects retired enclosing stores. Definition assignment and
destruction clear observations; cold clones inherit no caller observation.
Existing active unregistered maps retain their caller-lifetime contract; they
are not retained as post-call views. Invocation stores, wrappers, services and
initialization flags still retire normally. No arbitrary current Sandbox is
substituted and no shared global is cloned to satisfy an identity assertion.

The returned map pointer remains borrowed. Clients must requery after owner
changes; this API does not make a previously held raw pointer safe after Clear.
ObjectManagedFunction, Function, GmatBase, Solver and Optimizer instance layouts
and virtual ordering are unchanged. Sandbox has two new owner-guard members;
matching private consumers were rebuilt. New helper methods are nonvirtual.

Copied DynamicDataDisplay recursion
----------------------------------

The full headless batch reached 101 passes and one crash in the unchanged
DynamicDataDisplay remaining-resource case. Two isolated debugger receipts
reproduce it. The deeper trace repeats Initialize -> UpdateData ->
Subscriber::InitializeForRun -> Initialize until stack exhaustion. A copied
subscriber's public update guard demanded initialization while Initialize was
already populating the table and before InitializeForRun could mark it ready.

DynamicDataDisplay now uses a private nonvirtual UpdateDataValues helper for
its internal initialization update. Public updates retain the output/binding
guard; initialization checks copied bindings before creating output. The original
cell selection, numeric/string values and color calculation body is byte-identical.
No readiness flag is prematurely set and no physical equation changes.
The original crashing case and original populated-table editing, value, color,
selective-update and persistence checks pass headlessly and visibly.

Executed evidence and limits
----------------------------

The first repaired run passes the seven unchanged original cases plus the new
lifetime witness: 8/8. The new witness has seven native unit observation records
covering cold copies, nested state, duplicate owners, map/function address reuse,
owner destruction, unregistered borrowing and refused restoration. It also uses
a real GMF/caller/Sandbox: correct shared identity and independent results 12/22,
external observation invalidation at Clear, and a distinct rebuilt generation.
The actual Sandbox map address is reused in both profiles without reviving its
old view.

The expanded headless run has 101/102 passes; the sole DynamicDataDisplay crash
then passes after repair together with the populated-table control. All 73
resource cases therefore have passing headless receipts. The final visible run
has 102/103 passes and all 73 resource cases pass, including the repaired display.
Function I/O, recursion, frames, pause/stop, subscribers and the other two VF13ad
compatibility cases pass. The unchanged native resource-copy support test passes.
Three current factory/menu/parameter inventories also pass in each profile.
No claim is made that the entire earlier headless batch was rerun after the
local display fix; the complete visible batch uses the final current source.

The visible VF13ad Central failure occurs when reopening its actual resource
editor after the enabled-report phase. That phase has passed its source, nominal,
iteration, convergence and independent optimum checks. An isolated unchanged
rerun with 30 diagnostic snapshots passes both phases without a code change.
The first failure is retained as an open intermittent editor-activation gate;
the diagnostic pass does not establish a root fix or full GUI qualification.

Finite original scoping and continuation
---------------------------------------

All original E491-E500 texts and fingerprints are retained. E493 any-resource
and E497 any-eligible-global clauses remain open. Current inventory has 570
unique native factory names in 36 categories, 324 parameter roles (321 system,
three user) and 86 GUI offered types. Thirty-three GUI types are outside the two
73-case catalogs; twenty catalog types are outside the GUI menu. These are
routing differences, not unsupported classes or scope exclusions. Other frozen
function families cover some of them. Listing does not prove creation/use or
eligibility; default configuration failure is not an exclusion either.

Continue with the exact current inventory in current-scope-universe-gaps-v1.json
and the plan in build/nasa-coverage/coverage95-20261108-function-scope-universe-current-review.
Map every supported resource, owned component, function, user/system parameter
and always-global category to actual main/function evidence and required
operations. Rebuild and execute remaining original families, repair real defects
and preserve all original universal wording. Keep visible activation intermittence,
formal MATLAB/Mars holds and all broader scientific/platform/current-build gates
open. No user decision is pending.

Stage: build/nasa-coverage/coverage95-20261108-function-scope-lifetime-current-review.
Exact preimages, bounded patch, debugger failures, original/new raw receipts,
terminal build logs, current owner hashes and a verified source archive are
retained. All 1334 protected original inputs are unchanged; delta whitespace is
clean. Only private build outputs changed; installed runtimes, releases and
remotes are untouched. All process handles are terminal. The goal remains active.
