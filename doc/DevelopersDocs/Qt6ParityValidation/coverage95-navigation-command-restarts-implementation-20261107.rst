Navigation command restart and scoped event retirement
======================================================

RunEstimator/RunSimulator whole-clone handlers previously ignored spacecraft/Parameter sources and threw an unfinished-type error for nearly all other types. Property notifications fell through to generic propagation behavior, which could not rebuild the complete navigation command lifecycle. This batch adds actual-source command definition staging and native restart handling, together with the event ownership prerequisites for retiring old engines.

Pending definitions and execution boundaries
--------------------------------------------

Each command owns a pending Estimator/Simulator value. A shared native helper selects its configured root by actual scoped pointer, follows registered owned propagation source/component relations, and ignores unrelated objects. It clones the current pending definition, current runtime resource or actual selected resource as appropriate, rejecting known borrowed/incompatible results. Whole root updates clone the actual incoming native type; property updates use existing concrete label/type-aware CopyParameter. Propagation updates use the resource providers. Direct edits to owned definitions/groups map into the staged resource through actual native definition identities, with explicit ambiguity/missing-definition errors instead of same-name fallback.

Staging leaves the current engine intact and subscribed until Execute reaches its restart boundary. Further related edits are accumulated into the pending value. Completed estimator execution sequence becomes zero immediately, so subsequent smoother selection cannot reuse a result invalidated by a definition change. At execution, native initialization rebuilds from the pending definition. Estimator observation loading remains delayed until its execution entry; repeated invocations retain their fresh native clone policy. Pending values are released only after successful native preparation. The remaining full measurement/filter/private-value graphs are not inferred complete from these providers.

Command-owned root and PropSetup clones record actual sources and containment. Group sharing uses the actual selected setup pointer rather than its display name; native ephemeris participants still receive separate groups. Guarded group values remain owned through insertion. A copied owned setup list maps source edges among its own setups/components into the corresponding new nodes. Native setup/component dependency contexts are retained, with conservative private dependency leases for partial updates. This does not qualify all external borrower or graph retirement cases.

Native resource and propagation initialization return values are checked. RunEstimator no longer prints and swallows propagation initialization exceptions. Failure guards leave commands unready and keep existing definition subscriptions available for corrective edits/retry. Running state is cleared after old stream processing is closed and before installing a new engine, so failed new initialization does not treat unopened replacement streams as an old active run. Runtime completion state is reset at preparation entry. Native state-machine reset actions remain under their existing class policy.

Stream and event ownership
--------------------------

MeasurementManager adopts and later deletes stream objects. Command-supplied DataFile roots resolved from a scene are therefore cloned before adoption, including ramp-table streams. Native concrete cloning preserves the configured stream/filter values while giving the manager its own object. Source/owner identity and dependency contexts are recorded. Complete stream/measurement update providers and arbitrary foreign clone failures remain open; this is not a full data-file graph qualification.

Commands detach their own borrowed events and their actual propagation-group registration before deleting engines or retiring function bindings. Unrelated commands' registrations are not cleared by a global EventManager CleanUp. The command owns and releases its propagation groups; its base deliberately omits deletion when overridePropInit is set. Native command assignment performs cold cleanup and RunSimulator now copies inherited command syntax/options through RunSolver assignment. Function retirement releases runtime/pending values while the function store is still live and clears stale completion state.

EstimationRootFinder stages and type-checks a replacement snapshot before retiring the previous one. Null clearing no longer calls Clone through a null pointer. Self-snapshot input preserves its previous definition edge, copies retain the actual source relation and snapshots remain inactive. Destruction/assignment release owned snapshots and clear borrowed event/buffer pointers. ClearPropSetup matches the actual registered source, allowing a command to detach its own group without clearing another command's snapshot. EventManager SetObject borrows measurement-owned events; CleanUp releases registrations instead of deleting those borrowed objects. Individual removal also aligns available status/name/activation entries; clear-all resets the corresponding state. Root-location formulas and iteration behavior were not changed.

Compilation and retained original scopes
----------------------------------------

Configured compilation v1: 16/16 steps, 13.46 seconds, return 0.
Configured compilation v2: 10/10 steps, 13.79 seconds, return 0.
Configured compilation v3: 8/8 steps, 3.31 seconds, return 0.

Twelve source/help paths have eleven existing preimages and one recorded absent/new helper, all captured before first edit/build v1, plus four planning snapshots. Before/build/final hashes, indexed deltas, a source map, logs and terminal compiler receipts are retained. Final sources match the last successful configured build. All compiler sessions are terminal; added-line whitespace is clean and all three help XML files parse.

RunEstimator and RunSimulator C++ instance layouts change because they now own pending definition pointers. Existing inherited virtual slots are retained. Matching configured C++ consumers, navigation bindings and EKF/smoother consumers rebuilt. External binary/plugin compatibility requires matching rebuilt consumers and remains unqualified; no release/package or shared installed runtime was changed. Preparation guards protect native root/group clone failures and selected readiness/error paths. This is not universal rollback/cleanup for arbitrary provider exceptions, resource destruction ordering or failed I/O during retirement.

The previous EphemManager.hpp non-SPICE declaration failure remains open with an unchanged dependency hash. Its known failing conditional compile was not repeated; no full non-SPICE portability claim is made.

Full smoother/filter, measurement/tracking/error/noise/participant ownership; source/stream providers beyond propagation; independent function value graphs; pending configuration remapping across changed lists; public declaration/component lifetime; external borrower/context ownership; active solver/group reconstruction and every remaining original function, nested Target/Optimize, scientific/persistence, specification/external/historical and FRR-36 scope remain open. Native command compilation is not evidence of end-to-end numerical or GUI acceptance. No all/any parent receives partial credit.

No runtime/GUI/mission/numerical tests, acceptance auditors/readers/seals, evidence admission, fixture/assertion/tolerance/denominator/gate changes, packages, installed/shared runtime updates, commits, pushes or external messages were performed.

Tracking-v84 remains 1093/1158 (94.39%), with 65 other rows. The recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full goal active and incomplete.
