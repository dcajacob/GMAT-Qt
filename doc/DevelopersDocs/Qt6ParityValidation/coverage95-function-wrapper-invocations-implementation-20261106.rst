Function wrapper bindings and invocation ownership, 2026-11-06
=============================================================

The previous v55 batch made concrete source progress in 20 production paths;
its final source hashes and successful v4 receipt were current at turn start.
This batch changes seven production paths. Full implementation remains active
and incomplete. No universal function/resource clause is declared closed.

Actual wrapper leases
---------------------

ElementWrapper provides on-demand invocation leases for the actual wrapper,
without copying, evaluating or substituting its value. The registry has weak
entries only. A borrowed lease alone does not take ownership. When a native
owner releases a leased wrapper, deletion waits for its last lease. Ordinary
unleased release deletes immediately. Lease allocation/registration completes
before publication; a failed allocation leaves the original owner intact.

The existing virtual wrapper destructor invalidates outstanding identities
before its base fields retire. A captured lease cannot become live again when
an address is reused. Wrapper assignment/copies do not copy a lease identity.
Native release paths do not initialize the registry or allocate during cleanup.
Central actual deletion retains optional debug-memory tracking. Concurrent
external destruction and arbitrary third-party ownership remain unqualified.

Function interface frames
-------------------------

ObjectManagedFunction stages cold formal maps with the original keys, saved
input/output bindings, aliases, output deletion queue and lifetime leases. All
state layers allocate before detachment. Restoration remains unarmed until the
nonthrowing transfer commits, so failed staging cannot clear the outer call.
Borrowed inputs retain their caller's ownership; dedicated owned output wrappers
transfer into the saved invocation. Saved outputs that alias inputs retain that
same actual wrapper and its input ownership policy.

Restoration retires inner interfaces, validates each captured identity, then
restores original maps/queue and retains their leases. Retired bindings stay null
and fail restoration after the base readiness flag has restored, leaving the
function uninitialized. Owned output transfer also checks its captured lease.
Input binding establishes leases before publishing a new input; duplicate
aliases share a lease. Copies and assignment remain cold and do not copy active
leases. Native owner replacement and cleanup release through the shared lifetime
protocol, retain aliases still owned by another argument role and detach views
before deleting a wrapper once. Lease pruning retains every live map/queue role.

Retirement ordering
-------------------

Function entry retires inner interfaces before saved native cache/frame storage
and common environments restore. The ObjectManagedFunction destructor, old
store retirement and definition assignment retire interfaces while services
still live. Failure cleanup retires outputs before native finalization and
preserves input bindings through finalization. No model, function, numerical
comparison or GUI was executed to qualify these paths during implementation.

Remaining original scope
------------------------

Full FunctionManager configuration, pending input, result, output-target and
call-stack resources still require their own frames for same-manager/shared-
function/recursive/exceptional calls. Interpreted FCS/prototype/command state,
all resource providers and complete referenced-object/scope/service ownership
also remain required. Wrapper lifetime does not alone preserve the entire graph
of objects referenced by a wrapper. External/unregistered maps, engine reset,
concurrent destruction, legacy discovery, custom native caches and every
original scientific and resource requirement remain unqualified. No smaller
managed-wrapper profile replaces the original entire-object and any-command
requirements.

Compilation and accounting
--------------------------

Stage: ``build/nasa-coverage/coverage95-20261106-implementation-function-wrapper-invocations``.

* v1: 324/324 build steps, 183.6 seconds, return 0.
* v2: 66/66 build steps, 3.36 seconds, return 0.

Final production hashes match build v2. Production/planning preimages, before/build/final hashes, indexed patches, source map, logs/terminal receipts and summary are retained. Build processes terminated. Added delta whitespace: zero. Matching configured core/Qt/console, enabled plugins and Python consumers rebuilt. One ObjectManagedFunction lease-vector field and one Function virtual retirement route require matching consumers. No ElementWrapper instance fields or virtual slots were added. External ABI/platform/runtime and optional debug behavior remain unqualified.

No tests, GUI, missions, functions, models, solvers, numerical comparisons, acceptance readers or seals ran. No packages, installed/shared runtime, commits, pushes or external messages. Original assertions/fixtures/tolerances/texts/fingerprints/denominator/ledger/gates remain intact.

Tracking-v56 retains 1093/1158 = 94.39% implementation present, 65 other rows; recorded source-versioned qualified baseline 1053/1158 = 90.9%, all 105 original acceptance rows open. No additional row or verification credit; full implementation incomplete.

Next
----

Finish each FunctionManager configuration/input/output/result/call-stack frame across same-manager and shared-function reentry, including pending arguments and exceptional restoration. Complete interpreted FCS/prototype/command runtime frames and passive UserDefinedFunction/GmatFunction providers; complete all remaining solver/estimator/simulator/resource providers and owned plot/subscriber/interface/measurement resources. Complete native output-name/cache transactions, legacy discovery, inout aliases, external-map lifetimes, custom vector type/arithmetic, snapshot cross-resource/private-formation/caller/global/recursive/exceptional bindings, arbitrary ownership/interior/scope-table/service/engine-reset/later lifetimes and every original body/kernel/NAIF/scientific/resource requirement. Preserve the full worklist and verification deferral; continue independent POV and unresolved rotation research without invented specifications.
