Whole force-model restoration within a prepared graph
=====================================================

ODEModel assignment replaced its force list, cleared internal frames, retired acceleration-reporting registration and reset state/parameter associations. Propagation and measurement borrowers could therefore retain retired force/frame identities after native Copy. This batch implements a scoped whole-model provider for an unchanged prepared graph, building on the spacecraft, station and concrete force restoration providers. It does not complete every active-session or changed-configuration contract.

Current force roots and propagation bindings
--------------------------------------------

The restoration path validates the complete owned force/child correspondence, concrete types and within-owner roles, then checks native physical/previous-state layouts, force/body declarations, satellite IDs, dynamic object/index/parameter roles, state-object identities, solve-for slots/holders and derivative storage shapes. Selected actual bindings are checked, not substituted by matching names across independent scopes. Changed collections or layout require native preparation before application.

PhysicalModel restores its numerical values within current arrays. Existing concrete force roots receive Copy in place; their current owners, borrowed transient subscriptions and contexts remain selected. ODEModel retains its internal frames, current PSM/GmatState, dynamic/state-object associations, preparation flags and acceleration registry. Previous-state values and nonanalytic/nominal derivative values restore into existing storage. Captured constraints, error-estimation settings, coverage state, mu map and solve-for values apply. An explicit scope guard leaves failed whole-model restoration unready; successful application retains prior readiness. Universal provider/allocation rollback is separate remaining work.

Native snapshot values and solve-for holders
--------------------------------------------

Copy construction now captures previousState, satellite/dynamic/state roles and mu data instead of dropping them. Native force cloning checks exact RTTI/type-name correspondence. Solve-for declarations, parameter count and values carry through native copies; holders and covariance owners map by actual source/target owned-force and nested-child correspondences, including owned drag atmospheres. Ordinary assignment keeps its cold force/frame replacement lifecycle while rebuilding these links into its new forces. Function-value reference binding remaps source solve-for holders only after force correspondence registration. Existing external holders continue through the native resolver/context policy.

GetFunctionValueOwnedObjects discloses direct force roots through an existing inherited slot. FunctionManager applies its existing reporting-context transient exclusion to this disclosure, preserving the detached acceleration-reporting policy while ordinary function values retain their full native force collection. No additional force is admitted merely because a component was disclosed.

External and polyhedron providers
---------------------------------

ExternalModel restoration validates unchanged module path, entry points, force-exclusion/satellite roles and exposed Python-interface identity. It retains the selected interface and readiness while the shared physical-model provider applies numerical values. External interpreter/module global state is not represented as native object fields and is not qualified by compilation.

PolyhedronGravityModel restores captured numeric body/time/orientation/cache values and mesh values into its existing body/mesh objects for unchanged shape/body roles, retaining current preparation flags. Its capture initializes time fields and carries numeric values; Copy validates concrete type. Ordinary mesh replacement stages its clone and rejects source/current aliases or incompatible native mesh types. Raw public mesh/table view lifetime remains open. The disabled legacy EventModel source was examined and its exploratory edits withdrawn to captured preimages before build; src/base/CMakeLists.txt excludes it, and modern event-locator behavior was not changed.

Compilation and retained scope
-------------------------------

Configured compiler v1: return 0, 89.28 seconds, last logged step 227/227.

Eight participating source/help preimages and four planning snapshots preceded the first edits/build; two additional inactive-event preimages remain unchanged with an explanatory note. Before/build/final hashes, indexed patches, source map, compiler logs and terminal receipts are retained. All compiler sessions are terminal; final sources match the last successful configured build. Added-line whitespace is clean and three help XML files parse. No instance fields or virtual slots were added; new methods are nonvirtual or override existing slots. Matching configured native/plugin/navigation/GUI consumers rebuilt, including external/polyhedron providers. External binary/platform compatibility remains unqualified. The unchanged EphemManager non-SPICE declaration limitation remains open without repeating its known failing conditional run.

Finish shared external atmosphere/burn/profile snapshot/value/lifetime contracts and raw/mesh/table/interpolator view lifetime. Complete covariance matrix/subview lifetime and posterior semantics; changed spacecraft/body/location/topology at native preparation boundaries; complete active estimator/smoother state/result and snapshot role remapping; public restoration-buffer contracts and full original function value behavior. Continue remaining native parent/clone owners and whole-spacecraft/plate/hardware edits, source/stream/participant/error/noise/hardware and top-level filter/physical-model updates, changed-list configuration remapping, external borrower/context lifetime, arbitrary provider/file failure cleanup and typed resize/reference providers. Finish all original function I/O/scoping/command/resource and nested Target/Optimize clauses, shared globals and reporting/wrappers; retain unresolved FRR-36/specification dependencies. Continue private renderer, any-Resource Save, body/kernel/NAIF/CSPICE-pool and ephemeris format/B-1/final-iterative/scientific scopes, including the existing EphemManager non-SPICE boundary. Preserve external/historical dependencies and POV/rotation research; defer runtime verification until implementation is complete.

No runtime/GUI/mission/numerical tests, acceptance auditors/readers/seals, evidence admission, fixture/assertion/tolerance/denominator/gate changes, packages, installed/shared runtime updates, commits, pushes or external messages were performed in this batch.

Tracking-v94 remains 1093/1158 (94.39%), with 65 other rows. The recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full goal active and incomplete.
