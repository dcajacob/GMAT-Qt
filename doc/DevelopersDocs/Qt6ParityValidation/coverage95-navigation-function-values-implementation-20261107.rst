Simulator and tracking function-value graphs
=============================================

Simulator had no supported native function-value provider, and copied tracking sets retained participant and propagation references from their source scope. This batch adds passive native definition binding for Simulator and TrackingFileSet, using the existing private/caller resource graph for their actual dependencies.

Native value components
------------------------

GmatBase now declares the existing GetFunctionValueOwnedObjects contract virtually, with an empty default. Existing Function, Subscriber, GroundstationInterface and StopCondition methods retain their current meaning. Simulator discloses owned propagation setups followed by tracking definitions; TrackingFileSet discloses owned filters. Generated adapters, runtime streams and active events are excluded because native preparation regenerates them. This disclosure is independent of GetOwnedObjectCount/GetOwnedObject script serialization, so native definition ownership does not add nested simulator/tracking resources to generated scripts.

Function value graph traversal and caller component mapping consult disclosed children. Registration checks corresponding native shapes and uses the existing owned-object identity registry before rebinding references. Owned setups enter their complete PropSetup provider, including native components and parameter-safe initialization. Generic registration remains separate from callable/Solver capability checks; unsupported estimator/smoother and foreign Solver types do not gain a provider merely by having children. Existing inherited native binders remain responsible for spacecraft/station hardware, error/noise, frames and component dependencies.

Tracking references and aliases
-------------------------------

TrackingFileSet validates that its filters were registered as corresponding owned values before binding. Participant references resolve by actual pointer through the retained private/caller graph. The provider snapshots and maps the actual propagation list and assignment map, reconnects them to destination member containers and binds the supplied SolarSystem. Signal-path participant labels follow resolved object names. Aliases are derived only from actual resolved objects; ambiguous same-name source identities and colliding destination propagation roles raise explicit errors. Unresolved declared names retain native validation policy instead of manufacturing references by name. Filters reuse their existing participant-aware native providers and preserve their native copied progress policy.

The provider clears generated models, leaves readiness false and performs no measurement calculation or stream opening. Copy/assignment retarget self-references to the destination API propagation containers when the source uses those containers. A parent simulator reconnects its owned tracking definitions to the destination manager's propagation list/map after child binding.

Simulator definition values
---------------------------

Simulator explicitly opts in only for its exact native concrete class. Preparation retains Solver value/configuration fields and owned native definitions while clearing borrowed state/event/model associations and restoring fresh initializing entry, epoch counter and start epoch. It does not resume an active simulation or copy an open I/O session. Native RunSimulator remains responsible for execution and observation preparation. The provider validates that owned setup/tracking identity mappings exist, remaps declared propagation/measurement role names and spacecraft assignments, and reconnects manager ownership and member-container pointers.

Simulator Clone now holds its value through retained-context rebinding. Copy checks concrete native type and retains the previous dependency context across replacement before rebinding the source context. Native guarded copies continue suppressing recursive context rebinding under the existing guard. Full checkpoint/result reconstruction, all foreign plugin providers and arbitrary provider failure rollback remain separate.

Compilation and retained scope
-------------------------------

Configured compiler v1: return 0, 178.89 seconds, last logged step 747/747.

Thirteen existing source/help preimages and four planning snapshots were captured before their first edits/build. Before/build/final hashes, indexed patches, source map, compiler logs and terminal receipts are retained. Final sources match the last successful configured build. All compiler sessions are terminal; added-line whitespace is clean and three help XML files parse.

Adding a virtual function to GmatBase changes native virtual-table contracts. Existing object instance layouts are unchanged by this batch. Matching configured C++ consumers, plugins, bindings and GUI consumers were rebuilt. External binary/plugin compatibility requires matching rebuilt consumers and remains unqualified; existing independently built binaries are not claimed compatible. No installed/shared runtime or release package was changed. The previous unchanged EphemManager.hpp non-SPICE declaration limitation remains open; the known failing conditional run was not repeated.

Estimator/smoother function-value providers; complete active navigation-state/result reconstruction; participant/error/noise/hardware and source/stream update providers; changed-list/removed-component remapping; external borrower/context and public declaration lifetime; arbitrary provider failure cleanup; all remaining clone owners and every original function, nested Target/Optimize, scientific/persistence, specification/external/historical and FRR-36 scope remain open. No all/any parent receives partial credit.

No runtime/GUI/mission/numerical tests, acceptance auditors/readers/seals, evidence admission, fixture/assertion/tolerance/denominator/gate changes, packages, installed/shared runtime updates, commits, pushes or external messages were performed.

Tracking-v86 remains 1093/1158 (94.39%), with 65 other rows. The recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full goal active and incomplete.
