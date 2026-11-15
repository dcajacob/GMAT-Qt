FunctionManager call frames and shared validation context, 2026-11-06
====================================================================

The prior v56 turn made concrete source progress in seven production paths;
its final hashes and successful v2 receipt were current at this turn's start.
This batch changes eight paths and makes source progress. The full original
implementation goal remains active and incomplete.

Manager call frames
-------------------

An executing manager routes new call setup to a separate cold manager instead
of changing its current fields. Each nested activation has independent call
configuration, input/output wrappers, pending values, FOS/owner/generation,
initializer, literal/evaluated caches, typed scalar/matrix/vector/object results
and caller/value/local stacks. Prepared-frame getters follow the pending call;
ordinary current-call getters keep the enclosing view. Execute and all four
typed evaluation routes use the prepared frame, retaining its object results
through the same manager lifetime and next-call contract.

Frames refresh call-site configuration between calls while retaining native
cache lifetimes. New arguments are consumed by their own frame. Parent results,
pending arguments and stack resources are not copied into the active child.
Cold manager copies omit activation ownership and retain the explicit naming policy. Eight fields were
added; no new FunctionManager virtual slots. Public preparation methods use
internal cores so invocation-internal work does not dispatch to its own child.
External/custom manager initialization overrides remain unqualified and required
where relevant to the original scope.

Before a completed reentrant frame is reused, its old dictionaries, wrappers,
initializers and descendants retire while its completed scalar/matrix/vector/
object result storage remains retained. The next argument-capture step can use
that preceding result. Repeated recursive calls do not accumulate one cached
store per call; completed store histories are bounded by active nesting and retained
return values. This is source implementation and compilation, not a measured
runtime memory or aliasing qualification. Unbound/retired queue access fails
explicitly before dereferencing its callee.

Function services and cleanup
-----------------------------

A shared active callee is not changed by setup setters before scoped entry.
Explicit legacy function naming defers until the saved function scope exists.
Nested ObjectManagedFunction suspension also detaches its FOS/GOS views, forcing
a private activation store. Native shared globals and runtime forces retain
their existing actual service identity.

Completed children retire before their parent's caller storage. Active
finalization requests wait for a later safe finalization/setup boundary; active
context retirement waits for execution to exit. Copy assignment rejects an
active target. Cleanup finalizes the callee only when this manager actually has
its installed invocation, so an old child cannot finalize a different active
call. Nested failure preserves caller publication data. Staged wrapper queues
retain ownership until native initialization and input binding complete, then
transfer to the actual function queue; cancellation deduplicates aliases.
Wrapper retirement precedes pending input/result storage retirement. Destructor
child retirement guards the parent's cleanup and retains its table storage.

Running, pending and completed frames have separate owners. Dispatch moves
the running manager onto the C++ stack; setup performed by parent command
reinitialization can prepare a different next frame. Completed results retain
their manager until reuse/replacement. This prevents prepared next-call context
from being hidden behind a completed descendant or growing an idle chain.

A caller-frame guard records both stack depths and the exact saved epoch. On
failure it unwinds only that matching frame after child cleanup and function
state restoration. Retired managed maps are discarded without dereference;
ordinary saved values restore through the native caller path, with secondary
errors reported without replacing the first error. Publishing the map/value
stack pair is atomic against allocation failure. Normal return already consumes
the matching frame, so the guard does not restore it twice.

The complete structural command graph, including branch children, receives the
actual caller manager, publisher and internal frame before initialization and
execution. Stack return also restores function services and input interfaces
before reinitializing its caller, preventing command bindings from retaining a
completed child manager. Full command execution cursors/caches and prototypes
remain required; these service bindings do not constitute that full frame.

Shared validation services
---------------------------

ValidatorContext snapshots all current validator service pointers, parsing
scratch, wrapper policy and diagnostic fields. Moderator lookup navigation
(object/previous maps, current function, internal solar system and management
mode) restores directly, including null, without setter side effects. Actual
created resources, registrations and global data changes are not rolled back.
Nested diagnostic lists restore to the enclosing operation; root diagnostics
remain available to the caller. Context restoration uses stored values/swaps
and does not run models. Three previously uninitialized validator fields now
have explicit defaults before they can be captured.

Invocation, direct preparation, rebinding and finalization/cleanup use scoped
validation contexts. Nested standalone argument initialization also scopes the
active function's fields. Snapshots own scratch storage, not referenced model
objects or singleton services. Full interpreter-object state, direct/finalizer
callbacks, custom lifecycle overrides, external/concurrent/engine-reset lifetimes
and every original resource/command/scope/scientific case remain required.

Compilation and accounting
--------------------------

Stage: ``build/nasa-coverage/coverage95-20261106-implementation-manager-call-frames``.

* v1: 181/181 build steps, 339.03 seconds, return 0.
* v2: 172/172 build steps, 180.75 seconds, return 0.
* v3: 66/66 build steps, 8.11 seconds, return 0.
* v4: 89/89 build steps, 174.23 seconds, return 0.
* v5: 66/66 build steps, 8.1 seconds, return 0.
* v6: 89/89 build steps, 163.44 seconds, return 0.
* v7: 89/89 build steps, 168.82 seconds, return 0.
* v8: 66/66 build steps, 9.5 seconds, return 0.

Final production hashes match build v8. Production/planning preimages, before/build/final hashes, indexed patches, source map, logs/terminal receipts and summary are retained. All build processes terminated; added delta whitespace zero. Four additional service preimages were captured after v1 and before their first edits; the final build covers all eight paths. Configured core/Qt/console, enabled plugins and Python consumers rebuilt. Existing Validator/Moderator instance layout and virtual slots are unchanged; Function has a nonvirtual active-state query. Matching manager consumers rebuilt; external source/ABI/platform/runtime and optional debug behavior remain unqualified.

No tests, GUI, missions, functions, models, solvers, numerical comparisons, acceptance readers or seals ran. No packages, installed/shared runtime, commits, pushes or external messages. Original assertions/fixtures/tolerances/texts/fingerprints/denominator/ledger/gates remain intact.

Tracking-v57 retains 1093/1158 = 94.39% implementation present, 65 other rows; recorded source-versioned qualified baseline 1053/1158 = 90.9%, all 105 original acceptance rows open. No additional row or verification credit; full implementation incomplete.

Next
----

Complete interpreted FCS/prototype/command invocation runtime frames and passive UserDefinedFunction/GmatFunction resource providers, then every remaining solver/estimator/simulator/resource provider and owned plot/subscriber/interface/measurement resource. Complete reentry during direct/finalization/custom-manager lifecycle and all shared interpreter/service ownership, native output-name/cache transactions, legacy discovery, inout aliases, external-map lifetimes and custom vector type/arithmetic. Finish snapshot cross-resource/private-formation/caller/global/recursive/exceptional binding, arbitrary ownership/interior/scope-table/service/engine-reset/later lifetimes and all original body/kernel/NAIF/scientific/resource requirements. Preserve the full worklist and verification deferral; continue independent POV and unresolved rotation research without invented specifications.
