Station restoration within existing owned components
=====================================================

Whole native station and receiver assignment replaced owned hardware/error-model values, while imager assignment replaced private fields of view. BodyFixedPoint assignment also retired generated kernels and local frames. Active measurement/estimation borrowers could retain pointers to those retired objects. This batch supplies an explicit native restoration path for the supported unchanged station graph. Spacecraft, ODE/other owner graphs and complete active-runtime reconstruction remain open.

Scoped restoration and owned roles
----------------------------------

GmatBase provides a nonvirtual scoped restoration guard and query. It also suppresses recursive function-value construction while the scoped native copies execute. EstimationStateManager uses the guard for both detached Copy preflight and live Copy application. The normal assignment paths retain their existing lifecycle. Base restoration retains current declaration provenance and automatic-global policy rather than importing snapshot creation metadata.

The new NativeRestoration helper validates corresponding owned collections recursively: counts, null roles, native concrete types and component slot names must agree, with consistent repeated source/target identities. Root snapshot destinations were already selected by actual identity in the prior batch; these component names identify roles within that established owner, not an equal-name substitution across scopes. Unsupported changes fail on detached preflight before live application. Native child Copy updates the selected existing object, retains its name/full name, conservatively retains previous dependency contexts and rebinds covariance owner references into corresponding current components. Universal allocation/provider-exception rollback is not supplied.

Station and hardware owners
----------------------------

GroundStation checks hardware/error-model declarations and signal-path keys, applies captured base and station values, and copies hardware, generic error models and path-specific error models into their existing roots. Existing selected readiness is retained after successful restoration; provider exceptions leave the owner unready. Receiver applies captured fields within existing owned error models and rejects changed declarations. Imager copies an owned private FOV into its current root; borrowed FOV bindings remain selected by the current active imager. RFHardware preserves its current antenna pointer while validating the captured reference role. Sensor rejects changed signal presence before its existing in-place Signal value assignment can allocate or retire signal storage.

Receiver and Imager disclose their owned error-model/FOV values through the existing function-value child interface. Private/caller graph registration can therefore include those nested components. This does not qualify arbitrary foreign subclass providers, every nested hardware owner, all covariance submatrix views or all internal borrowed buffers. Matrix/cache memory and external raw-pointer lifetime contracts remain explicit follow-on work.

Body/frame and kernel bindings
-------------------------------

BodyFixedPoint restoration validates compatible body/solar-system bindings when exposed, central-body and representation roles, radii/flattening and unchanged location/body-fixed geometry. It keeps current body references, owned frames and generated/loaded kernel state instead of calling the ordinary binding-retirement path. Captured state/time bookkeeping is restored. SpacePoint's scoped path retains current frame, origin, solar-system and kernel handles while applying base/display values; normal copies retain their prior behavior. Changed body, geometry or topology requires its native preparation/reconstruction path and remains open. Keeping same-geometry station handles does not qualify shared CSPICE-pool/global ephemeris behavior or every scientific cache.

Remaining live restoration scope
--------------------------------

Spacecraft and ODE whole Copy can still replace hardware/forces and reset active engine associations. Those paths do not gain a complete owner provider merely because they execute inside the guard. Complete identity-preserving restoration, all native covariance/STM/propagation and measurement borrowers, internal arrays/caches, changed declarations and full active estimator/smoother state/result reconstruction remain required. No full active engine restoration or original broad parent requirement is claimed complete.

Compilation and retained scope
-------------------------------

Configured compiler v1: return 0, 178.43 seconds, last logged step 747/747.
Configured compiler v2: return 0, 3.96 seconds, last logged step 67/67.
Configured compiler v3: return 0, 3.44 seconds, last logged step 66/66.

Sixteen source/help paths have fifteen existing preimages and one recorded absent/new helper. Fourteen paths were captured before build v1 and the two body/point source preimages before their edits/build v2; four planning snapshots preceded the first edits. Before/build/final hashes, indexed patches, source map, compiler logs and terminal receipts are retained. Final sources match the last successful configured build. All compiler sessions are terminal; added-line whitespace is clean and three help XML files parse. Existing object instance layouts and inherited virtual slots are unchanged by this batch. Matching configured native consumers, plugins, navigation bindings and GUI consumers rebuilt. External binary/plugin compatibility remains unqualified and requires consumers matching the current source tree. No installed/shared runtime or release package was changed. The previous unchanged EphemManager.hpp non-SPICE declaration limitation remains open; its known failing conditional run was not repeated.

Full spacecraft/ODE/other parent borrower coherence; internal borrowed arrays/covariance cache views; changed body/location/topology preparation; active estimator/smoother state/result reconstruction; complete snapshot aliases/source-role remapping; raw public restoration-buffer borrowers; all native source/stream/participant/error/noise/hardware/filter/physical-model updates; changed-list/removed-component remapping; external borrower/context and public declaration lifetime; arbitrary provider/file failure cleanup; all remaining clone owners and every original function, nested Target/Optimize, scientific/persistence, specification/external/historical and FRR-36 scope remain open. No all/any parent receives partial credit.

No runtime/GUI/mission/numerical tests, acceptance auditors/readers/seals, evidence admission, fixture/assertion/tolerance/denominator/gate changes, packages, installed/shared runtime updates, commits, pushes or external messages were performed.

Tracking-v89 remains 1093/1158 (94.39%), with 65 other rows. The recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full goal active and incomplete.
