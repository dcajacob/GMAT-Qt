Solver iteration restoration and formation identities
====================================================

Target and Optimize called ordinary Copy when restoring iteration snapshots. That bypassed the existing scoped restoration implemented for spacecraft, owned hardware, state and covariance. Ordinary Formation::Copy also replaced current member bindings with the frozen value's bindings, which are empty for a dormant native clone. These were concrete rollback call-site omissions. This batch connects iteration rollback to the existing in-place path and preserves actual formation membership.

Scope established by source tracing
-----------------------------------

The previous next-source-review-v97 note identified cold Solver/Estimator/SeqEstimator/SmootherBase copy policies as a candidate for further investigation. Those ordinary definition-value providers intentionally start a fresh run and do not establish a complete active checkpoint contract. Current scoped restoration call sites were in EstimationStateManager; SolverBranchCommand still copied spacecraft and formations without that scope. This packet fixes that demonstrated call path. No active estimator/smoother checkpoint provider was added or claimed complete, and none of the broad original state/result/function requirements was discharged.

Snapshot graph and correspondence
---------------------------------

StoreLoopData starts from the actual spacecraft/formation entries in the local/global script maps, then follows formation SPACEOBJECT references. Members with no direct script-map entry are captured too. Identity deduplication handles aliases/shared membership and bounds traversal through repeated references. Every formation snapshot records ordered actual member pointers with weak lifetime tokens. Independent captures finish before replacing the previous snapshot collection.

A native function-value copy guard retains current frame/owner contexts during freezing instead of rebinding each snapshot into another private scene. Native spacecraft constructors already capture their selected frame references. A captured context retains its dependency lifetimes; it is not a new globally resolved same-name scene.

ResetLoopData checks every actual target's lifetime and concrete class before dereferencing its roles. Formation member arrays must still match the captured ordered identities and weak tokens. Reachability is recomputed from actual script-map roots through those unchanged memberships; an unrelated same-name object cannot inherit an earlier numerical snapshot.

Prepared values retain current root names/full names. Clone results are checked for exact native class and forbidden borrowed identities, including map resources, command-owned resources, saved snapshots, prepared values and probes. All detached probes remain owned through preflight so allocator reuse cannot turn a stale forbidden address into a false borrowed-object result. Recursive owned-role correspondence is checked before application. Concrete guarded copies run on detached probes before any live target receives values. This detects supported deterministic provider/layout failures; arbitrary later allocation or foreign-provider failures are not a proved atomic rollback transaction.

In-place application and formation behavior
------------------------------------------

Live application uses NativeRestorationGuard and the common native Copy helper, retaining previous dependency contexts and actual resource names. Compatible spacecraft hardware roots, state/covariance storage and selected environment references use their existing scoped providers. Branch ResetLoopData notifications run after leaving the restoration scope.

Formation scoped assignment validates component names, packed dimension, count and sharing mode; resolved members must still match the declared order and packed size. It uses the existing SpaceObject state restoration, retains readiness on success and leaves failed application unready. Formation::Copy keeps destination member bindings in this scope. Ordinary whole-object Copy still transfers source bindings, and ordinary assignment keeps its existing propagation-buffer policy. The caller's captured membership witnesses provide the iteration path's actual-identity check.

Changed active declarations, hardware/STM layouts, formation memberships or firing roles still require native preparation. Restoring those edits coherently with command/propagator/measurement borrowers remains implementation scope. This packet does not silently fall back to retiring active roots.

Compilation and compatibility
-----------------------------

Configured C++ compiler v1: return 0, 4.05 seconds, last logged step 67/67.
Configured C++ compiler v2: return 0, 4.11 seconds, last logged step 66/66.

Five source/help and four planning preimages preceded edits. Before/build/final hashes, indexed patches, source map, compiler logs and terminal receipts are retained. Both sessions are terminal; final source hashes match v2. Added-line whitespace is clean and three help XML files parse. The two compiler logs contain no warning/error diagnostics. No public instance layout or virtual slot changed; the extended snapshot record is local behind the existing command resource pointer. Matching configured native/plugin/GUI consumers linked. External binary/plugin and other platform qualification remain undone.

Complete coherent changed-component/membership/topology restoration and native preparation at iteration boundaries, including firing/burn transitions and dependent propagation/measurement registrations; fixed-layout in-place copies are not the complete all-command contract. Finish original function I/O/scoping and full supported command/resource/clone/ownership behavior, state/result/posterior and estimator/smoother requirements, public restoration-buffer contracts, shared external values/lifetimes and source/stream/participant/error/noise/hardware providers. Preserve ordinary cold estimator definition-copy semantics unless a concrete restoration/value contract requires a separate path; inspect actual call sites before changing that policy. Continue private renderer, any-Resource Save/persistence, body/kernel/NAIF/CSPICE-pool, ephemeris B-1/format/final-iterative/scientific, specification/external/historical dependencies and independent POV/rotation research. Defer runtime verification until the full implementation pass is complete.

No runtime/GUI/mission/numerical tests, new tests, acceptance auditors/readers/seals, evidence admission, fixture/assertion/tolerance/denominator/gate changes, packages, installed/shared runtime changes, commits, pushes or external messages were performed in this batch.

Tracking-v98 remains 1093/1158 (94.39%), with 65 unresolved rows. The recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full implementation audit incomplete; goal active.
