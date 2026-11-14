Actual engine clone identities and native update preparation
============================================================

Propagation setup/component and solver-command updates previously selected many clones by equal names, and the function-owned clone registry handled only direct containing owners. A nested model update could therefore miss its containing engine command, or select an unrelated same-name private resource. This batch extends actual source identity through PropSetup and the native Propagate and Target/Optimize command paths.

Nested source and containing-owner selection
--------------------------------------------

GmatBase source matching follows registered actual source edges, validating every weak lifetime identity against the current registry under its mutex and rejecting source cycles. A direct source remains meaningful. Further traversal through an inactive value/snapshot stops, rather than treating its retained ancestry as a live definition. Owner traversal follows validated containment edges and returns the outermost active owner. Notification snapshots deduplicate that owner, allowing a nested model change to reach the propagation command which must prepare its engine again. Registry helpers do not invoke public locking accessors or virtual object methods while holding the registry mutex. Existing lifetime metadata remains out of native object instances. Same-concrete-type component replacement retargets registered weak source edges from the old component to its prepared replacement. This keeps nested followers connected when owner delivery order differs, while inactive snapshot values remain unchanged. Cross-type declaration edits and arbitrary external native-pointer borrowers still require their original lifetime/reconstruction work.

PropSetup construction and attachment record component containment. Explicit native component attachments record their actual incoming source; self-component replacement preserves the previous definition edge. Native parent copies inherit source edges without subscribing to the original owned child, and assignment reparents the prepared replacement pair to the destination. Runtime command binding explicitly records the actual source setup and its actual propagator/model children. It checks concrete types, shapes and alias separation before binding.

Ordinary scene setup initialization activates the scene owner. Command-owned setups stay inactive as separate subscribers and rely on their containing command's activation. Retained-value initialization remains inactive. Propagate and SolverBranchCommand deactivate at initialization entry and activate on successful native initialization. GmatCommand retirement disables notifications before clearing its function stores/environment. An inner runtime setup therefore does not remain independently subscribed after its command retires.

Propagation and solver updates
-------------------------------

PropSetup whole/property update handling requires the exact owned object or its actual definition relationship, with concrete type agreement. Both setup readiness fields are invalidated before mutation. Owned direct edits invalidate without self-copying or deleting the notification source. Whole ODE-model updates use its concrete virtual Copy. Propagator leaves do not all expose virtual Copy, so whole propagator updates use their complete concrete Clone instead of a base assignment. Root FM/Type properties use actual source components; Type updates prepare the coupled propagator/model pair before retiring the old pair. Other root scalar properties use label/type-aware CopyParameter.

Propagate initialization guards its setup clones, rejects source/existing-group aliases and incompatible native results, and binds the new setup to the actual resolved definition before publication. Its new overrides route both whole and single-property updates through these setup providers. Whole setup configuration updates retain the command's actual propagation state manager contents, rather than importing the scene definition's empty propagated-object set. Borrowed propagator/model/PSM arrays are refreshed after applicable updates and on mutation failure. The native hasFired preparation flag is reset; the next native preparation reconnects the physical model and rebuilds its model/state bindings. This does not add dynamic group-membership editing or qualify every active propagation lifecycle.

Target/Optimize local solver construction/reuse registers its actual source and command owner. Whole and parameter notifications require that relation or a direct owned edit, so equal names alone no longer select a solver. Concrete virtual Copy and the existing native whole-update invalidation policy remain. Parameter updates retain the existing scalar lifecycle policy rather than resetting an active solver loop for every scalar setter. Complete active-loop configuration changes remain an original open scope.

Compilation and retained limits
-------------------------------

Configured compilation v1: 199/199 steps, 83.4 seconds, return 0.
Configured compilation v2: 66/66 steps, 3.13 seconds, return 0.
Configured compilation v3: 747/747 steps, 179.82 seconds, return 0.

Eleven source/help paths have preimages before their first edits (ten before v1, the registry header before v3), before/build/final hashes, indexed deltas, a source map, terminal compiler receipts, summary and four planning snapshots. The final sources match the last successful configured build; all compiler sessions are terminal. Added-line whitespace is clean and all three help XML files parse. Existing instance layouts and virtual slots remain unchanged; the new helpers are nonvirtual and Propagate overrides existing inherited slots. Configured native consumers rebuilt. Runtime, platform, external binary/plugin and concurrent resource operations remain unqualified. Native allocation/copy failures invalidate affected state; this is not universal rollback for arbitrary provider failures or external borrowers.

The previous EphemManager.hpp non-SPICE declaration failure remains open with an unchanged dependency hash. That known failing conditional compilation was not repeated; no full non-SPICE claim is made.

PropagationEnabledCommand's other native consumers, estimator/simulator/smoother clone update providers, force/station/plate ownership, dynamic group reconstruction, active solver configuration lifecycle, complete function I/O/scoping/resource clauses, public declaration/component replacement and lifetime, whole-spacecraft assignment, other graph/list/resize providers and all remaining original scientific/persistence scopes remain open. FRR-36, specification/external dependencies and historical targets retain their original scope. No all/any parent receives partial credit.

No runtime/GUI/mission/numerical tests, acceptance auditors/readers/seals, evidence admission, fixture/assertion/tolerance/denominator/gate changes, packages, installed/shared runtime updates, commits, pushes or external messages were performed.

Tracking-v82 remains 1093/1158 (94.39%), with 65 other rows. The recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full goal active and incomplete.
