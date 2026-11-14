Owned tracking definitions and native smoother integration
==========================================================

Native MeasurementManager copies previously retained borrowed tracking-set and adapter pointers. Estimator/simulator copies could therefore share generated models and filter progress, while copied estimation-state registrations could still target old definitions. TrackingFileSet assignment manually deleted adapters also present in its ownership tracker and retained stale filter bindings. This batch repairs those native ownership paths and integrates their update providers into navigation resources and commands.

Owned definitions and rebuilt measurement products
--------------------------------------------------

MeasurementManager owns native TrackingFileSet copies; its raw trackingSets list is a view into those values. Copies preserve declaration/configuration values and native filter value-copy policy while excluding generated adapter, data-stream, observation and event runtime associations. Native initialization clears previous manager products and rebuilds adapters from the owned definitions. Native filter initialization remains under its existing policy; this is not a new thinning algorithm or a universal reset of copied filter progress. Direct standalone TrackingDataAdapter registrations remain a borrowed runtime path and independent standalone-adapter value graphs are not supplied here.

Guarded clones reject known borrowed or incompatible native results and record actual definition/containing-owner identity. Copies retain existing dependency contexts; partial updates conservatively retain private source references. Each TrackingFileSet owns its cloned filters and generated adapters through the existing createdObjects tracker. Copy/assignment stages filters before publication; assignment retires the old tracked objects once. Replacing a filter clears generated models without separately deleting tracked adapters twice. Initialization checks filter/model return values and leaves readiness false on failure.

TrackingFileSet retains actual participant/SolarSystem references. Its API propagation-list/map containers copy by value while their PropSetup pointers remain borrowed according to the existing native API contract. Full independent participant, hardware, error/noise, navigation-state and function-private value graphs remain open. Dependency leases are not proof of universal public borrower lifetime or replacement safety.

Actual-source updates and estimation registrations
-------------------------------------------------

A measurement manager records live old-definition/filter origins when copying or replacing owned values. Declared reference names identify configuration attachment slots; notification matching uses actual pointers, weak source ancestry and explicit origin mappings. Same-name objects do not substitute for an update source. Direct edits to current owned roots/filters map into their staged copies. Whole/property updates prepare replacement tracking definitions, invalidate manager products and retain displaced generations until parent estimation-state declarations have been rebound. Removed or ambiguous component correspondence raises an explicit error instead of silently keeping an old target. Arbitrary changed-list remapping and universal allocation/provider-exception rollback remain open.

Estimator and Simulator attach owned tracking definitions, set their measurement manager's containing owner and rebind its propagation pointers to destination member containers after copying. Estimator remaps retained StateManager declarations and numeric state into its manager's actual owned definitions, clears prepared solve-for/consider/STM/covariance associations and reconnects native preparation later. EstimationStateManager no longer leaves stale convenience pointers in copies or accumulates duplicate solve-for/consider slots on assignment; its physical-model cleanup now reaches the owned member list instead of a shadow local. General physical-model copy exception safety remains separate.

Estimator/Simulator whole/property providers include owned tracking/filter sources as well as propagation definitions. Navigation command staging recognizes those relationships. Fresh independent TrackingFileSet scene owners activate notifications; owned native copies remain inactive under their containing navigation owner. This does not finish every cross-scope or foreign plugin function-value provider.

Smoother filter and native restart
----------------------------------

Smoother copies/assignments stage concrete native SeqEstimator clones with actual source identity and context leases, retire the prior owned filter and guard propagation copies during initialization. Its copied manager and estimation-state registrations bind to owned destination tracking definitions. Native filter initialization/reinitialization failures are checked. Reset explicitly restores initializing/forward filtering entry, clears backward/result state and retains supplied forward input for the existing smoother workflow. RunEstimator uses the supported forward action for sequential filters and the explicit reset action for other estimator types. Estimator's native owned-definition provider reaches smoother filter updates through its existing virtual reference getter, without introducing an estimation-base link dependency on the EKF plugin.

Full active-state/result reconstruction, participant/group rebinding, external source replacement and all original smoother/scientific clauses remain open. No runtime, numerical or GUI correctness is inferred from compilation.

Compilation and retained scope
-------------------------------

Configured compiler v1: return 1, 1.89 seconds, last logged step 8/88.
Configured compiler v2: return 0, 34.73 seconds, last logged step 87/87.
Configured compiler v3: return 0, 3.94 seconds, last logged step 8/8.

Compiler v1 failed on a duplicate HasLocalClones declaration supplied by the old default macro and missing explicit name arguments to the existing Estimator reference getter. Both source errors were corrected; the failed log and receipt are retained. The final build also includes explicit mapping for updates to current owned tracking/filter definitions and initializer-order cleanup.

Twenty source/help paths have nineteen existing preimages and one recorded absent/new helper, plus four planning snapshots captured before their first edits/build. Before/build/final hashes, indexed patches, source map, logs and terminal compiler receipts are retained. Final sources match the last successful configured build. All compiler sessions are terminal; added-line whitespace is clean and five help XML files parse.

MeasurementManager instance layout changes through owned definition/origin/owner fields. Estimator/Simulator and derived classes contain it by value, so matching configured C++ consumers, navigation bindings and smoother/EKF consumers were rebuilt. Existing virtual slots are retained; external binary/plugin compatibility requires matching rebuilt consumers and remains unqualified. No installed/shared runtime or release package was changed. The previous unchanged EphemManager.hpp non-SPICE declaration limitation remains open; the known failing conditional run was not repeated.

Full independent function value graphs; measurement participant/error/noise/hardware/navigation-state ownership; standalone adapter providers; source/stream updates beyond these definitions; changed-list/removed-component remapping; external borrower/context and public declaration lifetime; arbitrary provider failure cleanup; active solver/group reconstruction and every remaining original function, nested Target/Optimize, scientific/persistence, specification/external/historical and FRR-36 scope remain open. No all/any parent receives partial credit.

No runtime/GUI/mission/numerical tests, acceptance auditors/readers/seals, evidence admission, fixture/assertion/tolerance/denominator/gate changes, packages, installed/shared runtime updates, commits, pushes or external messages were performed.

Tracking-v85 remains 1093/1158 (94.39%), with 65 other rows. The recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full goal active and incomplete.
