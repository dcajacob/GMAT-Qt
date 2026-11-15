Function output declaration notifications, 2026-11-06
====================================================

This batch changes 17 production paths. The prior v52 batch made source
progress; its planning state is retained in the stage preimages. The full
implementation goal remains active and incomplete.

Declaration and preparation contract
-----------------------------------

Function::SetOutputTypes validates equal metadata lengths, stages copies of all
three arrays and a stable callable before publishing, then synchronously calls
the active output initializer. Native initialization can obtain a correctly
typed output immediately after the setter returns. Fourteen native leaf
overrides delegate to this protocol. Identical declarations during notification
return without recursion; changed recursive declarations fail. Preparation
exceptions restore the previous three arrays. The existing manager refresh
stages output objects and its dictionary before publication.

The callback does not atomically include output names, arbitrary native caches,
or custom callback side effects. Full custom declaration/discovery and nested
invocation semantics remain required; this is not a universal closure claim.

Invocation lifetime
-------------------

An invocation guard saves and suspends the enclosing handler before argument
preparation, then binds this manager before native initialization. Its handler
checks the actual callee binding and managed-store lifetime before refreshing.
The guard remains active through failure finalization and cleanup, then restores
the enclosing handler only if the original function lifetime is still live.
Null or retired stores defer to ordinary preparation. Existing before/after
initialization refresh remains. Only this handler stack is restored by the new
guard: all FOS, native cache, input/output, global and service state still needs
the complete nested invocation protocol.

Function copy construction starts with an empty callback and notification flag.
Assignment does not copy those transient services from the source. Two instance
fields and a nonvirtual API were added; no new virtual slots. Matching configured
consumers rebuilt. External ABI/platform/runtime behavior remains unqualified.

Compilation and accounting
--------------------------

Stage: ``build/nasa-coverage/coverage95-20261106-implementation-output-declaration-notifications``.

* v1: 181/181 build steps, 227.59 seconds, return 0.
* v2: 69/69 build steps, 10.47 seconds, return 0.

Final source hashes match build v2. Production preimages, before/build/final
hashes, indexed patches, logs, terminal receipts and four planning preimages
are retained. Both build processes terminated; newly added delta whitespace is
zero. Core, Qt, console, enabled plugins and Python consumers compiled.

No tests, GUI, missions, functions, models, solvers, numerical comparisons,
acceptance readers or seals ran. No packages, installed/shared runtime,
commits, pushes or external messages. Original assertions, fixtures,
tolerances, texts, fingerprints, denominator, ledger and gates are preserved.

Tracking-v53 retains 1093/1158 = 94.39% implementation present, with 65 other
rows. The recorded source-versioned qualified baseline remains 1053/1158 =
90.9%; all 105 original acceptance rows remain open. No new row or verification
credit is claimed. Full implementation remains incomplete.

Next
----

Complete the whole nested invocation state and native output-name/cache contracts, legacy discovery, inout aliases, external-map lifetimes and custom vector arithmetic/type inference. Complete all remaining solver/estimator/simulator and resource providers, owned plot/subscriber/interface/measurement resources, command/plugin ownership/reference/retirement paths and passive UserDefinedFunction/GmatFunction body/prototype admission. Finish snapshot cross-resource/private-formation/caller/global/recursive/exceptional bindings, arbitrary ownership/interior/scope-table/service/engine-reset/later lifetimes, GetEphemStates destination-owned-frame alias Copy and all original body/kernel/NAIF/scientific/resource requirements. Retain full original scope and verification deferral; research POV and rotation drift independently without inventing specifications or asking again for a POV dialect.
