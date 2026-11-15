Native private interpolation cache preparation
=============================================

Native restoration now stages owner-specific private storage through a common preparation plan after validating the selected GmatBase-owned graph. Spacecraft prepares private CM/MOI Lagrange caches and FileThrust prepares private linear/not-a-knot caches when captured presence or layout differs, including lazy creation after capture. Compatible caches keep their objects/storage. Native type and owner checks restrict replacement to these exclusive unexposed caches; typed public/borrowed interpolator restoration remains unchanged. Plans allocate/clone before applying pointer selection and retain displaced caches until copying completes. Existing owner declarations/environment/physical-model checks are factored and preserved, and preparation failures leave owners unready. Both shared owned-graph restoration and direct native owner Copy routes use preparation, covering estimation direct copies. Ordinary copying, interpolation equations, native state/time/math routines and owned-node identity/covariance gates remain unchanged. Eleven source/help paths compile and three help XML files parse. The new GmatBase virtual contract requires matching rebuilt clients/plugins; Formation plans derive from the common plan. Full owned-component/topology/function/command/session/scientific scope remains open. No row or verification credit.

Ownership and preparation
-------------------------

Compatible interpolation restoration previously rejected optional roots that appeared after capture or needed different storage. Native CM/MOI and thrust interpolation pointers are exclusively owned and unexposed: their fields are only referenced in the two owner implementations. This allows owner-specific replacement at a command boundary while retaining the old cache until the copy finishes. The public/borrowed typed interpolation provider contract remains fixed-layout; no general replacement fallback is added there.

GmatBase now exposes a staged preparation plan. NativeRestoration validates the entire selected owned-node correspondence, stages plans for all pairs and only then applies private selections before its existing Copy loop. Source/target native cache and owner types are checked; source copies and compatible layout are validated before a plan is returned. Compatible cache storage receives no replacement. Captured absence is restored as absence, including undoing a lazily created spline. Source data is cloned without opening input files or initializing services. Existing owned component name/type/cardinality and covariance mapping remain.

Owner Copy entry points also prepare because EstimationStateManager directly calls concrete Copy under its native guard. Shared-copy callers see an already compatible private selection and do not allocate another replacement. Original declaration, environment, physical-model, participant and state-role checks are preserved in factored validators. Preparation/copy failures leave the owner unready. This preserves native failure signaling; it is not a general atomic transaction for every provider.

Algorithms and compatibility
----------------------------

Every existing Spacecraft and FileThrust method/value-copy body is reproduced by reinserting the old validation and removing only the new preparation entry/helper code. All interpolation implementations and typed restore providers are unchanged references. No interpolation equation, numerical tolerance, force evaluation, native state/time method or ordinary value-copy policy changes. No new resource parameters or instance fields. GmatBase gains a virtual contract and the typed Formation plan inherits the common plan, requiring matching rebuilt native consumers/plugins.

Compilation and retained evidence
---------------------------------

Build v1 returned 0 in 181.39 seconds, progress 747/747; 3 existing deprecation warnings (MATLAB string conversion and/or Qt image mirroring); no compiler errors.

Build v2 returned 0 in 9.56 seconds, progress 67/67; no compiler diagnostics; no compiler errors.

Both compiler sessions are terminal. Final hashes match v2. Eleven source/help preimages and four planning snapshots preceded edits; fourteen initial unchanged references and one direct-estimation-copy reference retained after terminal v1 are preserved. Indexed/combined patches, hashes, compiler receipts and source/research notes are retained. Delta whitespace is clean and three help XML files parse. Configured probes may compile/link; none run. The initial owner edit stopped at an exact-signature lookup before any C++ write; its unfinished edits were completed with the corrected signature, without rerunning the capture.

No new tests, runtime cache/layout/clone/estimation/solver/function/lifetime checks, GUI/model/helper/MATLAB/mission/numerical execution, acceptance auditors/readers/seals or evidence admission occurred. Original requirements, text, fingerprints, fixtures, assertions, tolerances, denominator, gates and dispositions remain unchanged. Tracking-v124 retains 1093/1158 (94.39%), 65 unresolved engineering rows; recorded verified baseline 1053/1158 (90.9%), all 105 original acceptance rows open. Full implementation audit and goal remain incomplete.

Next implementation
-------------------

Continue coherent owned spacecraft/hardware/force/measurement component and propagation/interpolation-storage preparation, including firing transitions, changed declared inputs and public/borrowed interpolation and Hermite extents. Keep lazy CM/MOI/spline creation/removal, incompatible private layouts, direct estimation/native copies, rejected owner/environment changes, allocation failure, cache lifetime and numerical equivalence for deferred verification. Complete original function I/O/scoping/property/output staging, retained dependency/shared-global lifetimes, all-command target/optimizer/estimator behavior and nested MATLAB sessions. Continue any-Resource serialization/default/input/reference/portability, body/frame/kernel/ephemeris, resource-tree, representative POV/exporter and every explicit specification/external/historical dependency. Preserve original assertions and defer runtime verification until the full implementation pass is finished.
