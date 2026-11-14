Native navigation propagation definition ownership
==================================================

Estimator and Simulator resource updates previously threw an unfinished-type exception for nearly every non-spacecraft/non-Parameter source, while their property handlers only printed debug traffic. Their owned propagation copies also lacked actual source/owner metadata. This batch implements the native resource-side propagation provider needed by command restart work; it does not declare RunEstimator/RunSimulator command update handling complete.

Guarded native definitions and publication
------------------------------------------

A shared internal NavigationPropagationCopies helper stages concrete PropSetup clones, rejects source/owner/existing-definition aliases and incompatible concrete results, and records containment. Explicit attachment follows the actual incoming setup and its actual child definitions. Native parent copies inherit existing definition edges rather than subscribing to the original owned setup. Copies remain inactive as independent notification owners and retain existing source frame/body dependency-context leases. Those leases are not a full independent function value graph provider.

Estimator/Simulator copy construction keeps every propagation setup under RAII through later constructor work. Native assignment stages the complete source setup list before changing the destination; a setup-cloning failure therefore leaves its current setup list intact. Publication reparents the prepared definitions, reconnects unambiguous same-definition weak subscribers and retires the old values. Satellite pointer maps are cleared and rebuilt through native policy. MeasurementManager is explicitly rebound to the destination resource propagation vector and spacecraft map after constructor/assignment copying; it no longer retains the source resource vector merely because its own copy retained that pointer.

Typed SetRefObject setup attachment stages its replacement value, indexed list and satellite mapping before publication. It preserves previous source ancestry on self-component replacement, follows an explicit incoming definition otherwise, and retargets compatible weak setup/component definition edges before retirement. Cold reference association can arrive out of order, leaving explicit unresolved slots until all declarations are associated. Real initialization and map/ephemeris queries validate those slots before dereference; named reference lookup skips empty slots. This retains native missing-definition errors without a null dereference.

Actual-source whole and property updates
----------------------------------------

Both resources now forward owned whole/property updates to PropSetup ApplyCloneUpdate. Its existing concrete type/label/property checks and native copy/clone rules govern mutation. Same names alone do not select a setup, propagator or ODE model. Actual setup-root updates are forwarded after an intermediate owned definition changes, with property IDs mapped by label and visited actual identities bounding source-chain traversal. Component updates follow their existing source ancestry. Direct owned mutations invalidate without self-copying. A matching update invalidates resource readiness and satellite mapping state; mutation failure also invalidates readiness. Unrelated objects are outside this provider and do not produce a blanket unfinished-type exception.

Fresh native resource construction and actual ObjectInitializer scene association activate resource owner notifications. Native copy construction does not inherit activation. Owned setup copies remain inactive independently, so frozen native values are not activated merely by being copied. The existing delayed Estimator initialization remains unchanged; resource update handling does not read observation data early.

Compilation and retained original scopes
----------------------------------------

Configured compilation v1: 83/83 steps, 16.63 seconds, return 0.
Configured compilation v2: 8/8 steps, 12.42 seconds, return 0.
Configured compilation v3: 8/8 steps, 12.49 seconds, return 0.

Nine source/help paths have eight existing preimages and one recorded absent/new helper, all captured before first edit/build v1, plus four planning snapshots. Before/build/final hashes, indexed deltas, a source map, logs and terminal compiler receipts are retained. Final sources match the last successful configured build. All compiler sessions are terminal; added-line whitespace is clean and all three help XML files parse. Existing native instance layouts and virtual slots are unchanged; the new resource methods/helpers are nonvirtual. Configured consumers rebuilt. Runtime/platform/concurrent/external plugin behavior remains unqualified. Preparation protects propagation-cloning failures; later arbitrary provider/allocation failures are not a universal whole-resource rollback guarantee.

The previous EphemManager.hpp non-SPICE declaration failure remains open with an unchanged dependency hash. Its known failing conditional compile was not repeated; no full non-SPICE portability claim is made.

RunEstimator/RunSimulator command-level whole/property handlers, command source registration and propagation groups, delayed/repeated/nested restart lifecycle, smoother/filter state, full measurement/tracking/error/noise/participant ownership and independent function value graphs remain open. Existing command TODOs are not converted into a claim of end-to-end support. EstimationRootFinder SetPropSetup currently clones a null input on clear and replaces its snapshot without retiring the previous one; scoped command/event retirement must repair that prerequisite. Other original resource/clone/scoping, lifetime, scientific/persistence, specification/external/historical and FRR-36 scopes remain open. No all/any parent receives partial credit.

No runtime/GUI/mission/numerical tests, acceptance auditors/readers/seals, evidence admission, fixture/assertion/tolerance/denominator/gate changes, packages, installed/shared runtime updates, commits, pushes or external messages were performed.

Tracking-v83 remains 1093/1158 (94.39%), with 65 other rows. The recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full goal active and incomplete.
