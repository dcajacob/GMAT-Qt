Actual clone-source identities and function notifications
=========================================================

Spacecraft clone notifications previously selected owned components by name within their family. A public hardware declaration could therefore update an unrelated private same-name component. Native GMAT function execution also lacked forwarding of hardware-change notifications to spacecraft owners. This batch introduces actual weak source/owner relations, uses them for spacecraft matching and connects function execution to registered live owners.

Source and owner identity
-------------------------

GmatBase exposes nonvirtual source/owner access, source matching, live-owner snapshot and notification-activation methods. Their state lives in the existing lifetime/context registry, not in native object instances. Each relation records the actual source/owner pointer and its weak lifetime identity. Registry lookup checks the current live identity, so an expired declaration or reused address cannot match a new resource just because its pointer/name was reused. Weak links do not create ownership of public roots. Object destruction removes its registry entries.

Native copy construction inherits only the source-definition edge. It does not inherit the old containing owner or activate the new value as a live subscriber. Assignment leaves the destination's native source/owner relationship intact. The containing spacecraft registers each new owned component against itself. This covers tanks, thrusters, power systems, plates, generic hardware and attitudes, including native parent construction/assignment and separate attachment paths. Explicit new attachments record their actual incoming definition. Renaming a current owned value preserves its previous definition edge rather than subscribing to the displaced component about to be retired.

Spacecraft::FindOwnedClone now requires the actual source relation or the exact owned object for a direct mutation; equal labels alone do not select a component. Concrete type/ambiguity checks remain. Attitude updates also require the actual relation. A direct change to the current owned attitude marks it for reinitialization without deleting the notification source; a definition update retains its actual source on the replacement.

Function values and subscriptions
---------------------------------

The existing function value graph binder now maps clone-source edges as well as ordinary reference edges. Copied/private definitions resolve through the actual copied graph, explicit caller mappings retain their mapped identity, and shared globals follow the existing Global/local-role policy. Non-global definitions absent from the original scene are captured as private definitions, rather than borrowing an ambient same-name resource. Private definition copies are owned by the retained context. Assignment into an already owned public root preserves its destination declaration edge; new value/native-child construction maps its copied edge.

Live owner activation is explicit. Fresh scene spacecraft and successful ordinary initialization/association activate notifications. Native copy construction and retained-value initialization do not automatically activate copied roots. Existing destination activation survives assignment. This keeps the source relations of solver loop/propagation snapshots available for restoration without turning those frozen values into live subscribers. Source review confirms the existing loop/propagation capture routes use native Clone and do not associate their buffer roots as scene resources. This is source evidence only, not an executed solver/propagation qualification.

Function control sequence delivery
----------------------------------

After an executed command reports AffectsClones, GmatFunction obtains its actual updated object/property and snapshots that source's registered live owners. Owner/source weak identities are revalidated before delivery, owners are deduplicated and the existing concrete whole/property update methods receive the notification. An explicit shared definition can therefore reach an actual caller/global spacecraft outside the active FOS, while an unrelated same-name resource has no matching subscription. Fresh cold declaration graphs still use their ordinary association after declaration values are complete; already-live owners observe edits.

If an initialized owner reports invalidation, initialization success is checked. References/associations are rebuilt through the current ObjectInitializer only for actual function-owned local objects in the FOS. Caller/shared-global owners retain their existing bindings instead of resolving their frame/body names through a potentially shadowing local store. The current registered native provider is Spacecraft; this is not a claim that every other HasLocalClones implementation has been converted. Native Sandbox dispatch still handles its existing owner routes, with spacecraft matching now using actual identity.

Compilation and original scope retained
---------------------------------------

Configured compilation v1: 747/747 steps, 174.19 seconds, return 0.
Configured compilation v2: 747/747 steps, 179.28 seconds, return 0.
Configured compilation v3: 4/4 steps, 2.15 seconds, return 0.

Nine source/help paths have preimages before their first edit (eight before v1, ObjectInitializer before v2), before/build/final hashes, indexed deltas, source map, logs, terminal receipts and four planning preimages. Final sources match the final successful configured build. All compiler sessions are terminal; added-line whitespace is clean and both help XML files parse. Existing native instance layouts and virtual slots are unchanged; APIs/helpers are nonvirtual and metadata uses the out-of-instance registry. Matching configured consumers rebuilt. External binary/plugin, concurrent resource operations, platform and runtime compatibility remain unqualified. Existing deprecation diagnostics outside this batch are retained in build summaries.

The prior EphemManager.hpp non-SPICE declaration failure remains open; its dependency hash is unchanged. The known failing conditional compile was not repeated, and no full non-SPICE portability claim is made.

Other native clone owners, public declaration/component replacement and lifetime, whole-spacecraft assignment, complete error-model/FOV/resource graphs, reference/resize providers, shared-global/callable/command/estimator/simulator/private renderer and every remaining original scientific/persistence clause remain open. FRR-36, other specification/external dependencies and historical targets retain their original scope. No all/any parent receives partial credit.

No runtime/GUI/mission/numerical tests, acceptance auditors/readers/seals, evidence admission, fixture/assertion/tolerance/denominator/gate changes, packages, installed/shared runtime updates, commits, pushes or external messages were performed.

Tracking-v81 remains 1093/1158 (94.39%) with 65 other rows. The recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full goal active and incomplete.
