Actual estimation restoration destinations
===========================================

EstimationStateManager previously restored each snapshot to the object at the same vector position, without checking actual identity or concrete type. Spacecraft used its native assignment operator, while most other types were sliced through GmatBase assignment. A reordered buffer could restore another object, and type-specific state could be omitted. This batch adds explicit captured destinations and concrete native restoration preflight. Live owned-child borrower reconstruction remains a required follow-on repair; this batch is not qualified for active estimation use.

Frozen destination identities
------------------------------

GmatBase supplies nonvirtual restoration-target accessors backed by its existing external registry. A snapshot records a pointer plus a weak lifetime token; lookup verifies the token against the current registry before exposing the pointer. Expiration remains distinguishable from an unbound value. Copy construction inherits the captured role, destruction erases it, and assignment between existing snapshot values carries the source captured role. Assignment to an ordinary resource leaves that resource ordinary. The target relation does not participate in live clone notifications or automatic owned-component retargeting.

Native frozen CloneValues capture a normal object's actual identity or preserve an existing snapshot's original target, reject an expired role and clear followed clone-definition edges. Function-value binding remaps the destination through actual private/caller correspondences. Unlisted non-global/local targets, including completed-command-owned values, enter the retained private dependency graph instead of becoming untracked borrowed references. Existing explicit caller/shared-global policies remain. Ordinary public root assignment does not acquire a snapshot marker.

Restoration correspondence and native copies
--------------------------------------------

Restoration retains the original equal-size requirement and requires captured target metadata. Each destination is selected by exact pointer first; a unique same-concrete-type sibling definition may correspond through common live native source ancestry. Names and vector position do not establish correspondence. Repeated, missing, expired, incompatible or ambiguous bindings fail before live mutation. Unmarked externally supplied vectors must be captured with BufferObjects or explicitly bound through the native target API; fabricated positional correspondence is not supplied.

The complete ordered snapshot list is cloned into guarded prepared values. Their root names/full names are set to the corresponding destination names. A detached clone of each destination exercises its concrete Copy contract before live application, so unsupported virtual Copy providers and deterministic Copy errors are detected before changing live objects. Prepared/live/source aliases known to this operation are rejected. Native-copy guards suppress recursive function graph construction during restoration. Live concrete Copy replaces the old base-class slicing, and previous dependency contexts are conservatively retained where needed.

FileThrust lacked a concrete Copy implementation. Its new provider checks the exact native concrete class, applies the existing native assignment logic and rebinds retained value contexts. Clone now holds its new value through context rebinding. No thrust formulas, interpolation algorithms or external file formats were changed.

Live parent/child ownership remains open
----------------------------------------

A concrete whole-object Copy can replace owned spacecraft hardware, station error models or ODE force children. Estimation-state maps, measurement adapters, propagators or other active borrowers can still reference the retired children. Preflighting Copy support does not repair that graph or prove that a running engine can continue after replacement. The next required implementation must preserve those child identities or reconnect every affected active borrower at the correct native boundary. Full active-state/covariance/STM reconstruction, all snapshot role/component remapping and public raw-buffer borrower contracts remain open. Arbitrary exceptions or allocation failures during live Copy do not have universal rollback. This is compilation-only implementation progress, not an active-runtime correctness claim.

Compilation and retained scope
-------------------------------

Configured compiler v1: return 0, 180.5 seconds, last logged step 747/747.
Configured compiler v2: return 0, 178.14 seconds, last logged step 747/747.

Ten existing source/help preimages and four planning snapshots were captured before their first edits/build. Before/build/final hashes, indexed patches, source map, compiler logs and terminal receipts are retained. Final sources match the last successful configured build. All compiler sessions are terminal; added-line whitespace is clean and three help XML files parse. Existing object instance layouts and inherited virtual slots are unchanged by this batch. Matching configured native consumers, plugins, navigation bindings and GUI consumers rebuilt. External binary/plugin compatibility remains unqualified and requires consumers matching the current source tree. No installed/shared runtime or release package was changed. The previous unchanged EphemManager.hpp non-SPICE declaration limitation remains open; its known failing conditional run was not repeated.

Full live parent/child borrower coherence; active estimator/smoother state/result reconstruction; complete snapshot aliases/source-role remapping; raw public restoration-buffer borrowers; all native source/stream/participant/error/noise/hardware/filter/physical-model updates; changed-list/removed-component remapping; external borrower/context and public declaration lifetime; arbitrary provider/file failure cleanup; all remaining clone owners and every original function, nested Target/Optimize, scientific/persistence, specification/external/historical and FRR-36 scope remain open. No all/any parent receives partial credit.

No runtime/GUI/mission/numerical tests, acceptance auditors/readers/seals, evidence admission, fixture/assertion/tolerance/denominator/gate changes, packages, installed/shared runtime updates, commits, pushes or external messages were performed.

Tracking-v88 remains 1093/1158 (94.39%), with 65 other rows. The recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full goal active and incomplete.
