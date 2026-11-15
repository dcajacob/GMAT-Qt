Native ephemeris source updates and reader retirement
====================================================

SolarSystem previously changed source flags, metadata and receiver bindings before all file operations and virtual receiver calls succeeded. A failed DE replacement could restore its previous reader allocation yet leave the source selector or some bodies changed. Successful replacement could also delete the old reader before all consumers were rebound. This batch supplies a shared native update boundary for those paths.

Rollback token and receiver state
--------------------------------

SolarSystemEphemerisTransaction is an internal stack type with targeted friendship from SolarSystem and CelestialBody. It captures the global source flag, current source/order, DE/SPK filename tables, selected SPK filename, owned/default reader and active ephemeris pointer. Affected default bodies and special points retain actual identities and snapshot their source/file binding, filename/body ID, SPICE-ready flag, two-body reset flag and Real/GmatTime ephemeris epochs.

A failed call restores those native fields before releasing abandoned readers. Strings/vectors restore by swaps and primitive/time fields restore directly; rollback does not call the failed virtual setters again. Capture/allocation precedes registration or native mutation. Null receivers reject before an update starts. Supported virtual receiver calls remain in the execution path.

A thread-local stack distinguishes nested updates and different SolarSystem instances. Nested updates of the same system transfer displaced-reader retirement to the nearest enclosing update. A later outer failure can therefore restore its previous reader and bindings even if an inner operation succeeded. Retirement allocation precedes pointer publication. Outermost successful completion disposes displaced readers after receivers have been rebound; rollback retains the restored owned reader and disposes abandoned replacements. This protects the reviewed native update interval, not arbitrary external borrowers after successful replacement.

Selection and filename paths
----------------------------

SetSource validates the enum before indexing source names, prepares the next source/order metadata and validates the associated SPK before changing body source flags. The guarded body/special-point calls then publish the new metadata. EphemerisSource assignment encloses source selection and matching DE-reader activation in one update. The deprecated Ephemeris spelling uses the same activation path after its existing warning.

DEFilename and SPKFilename edits guard their coupled reader/name-table changes. SetPlanetarySourceName retains its existing provisioning behavior and accepted families while guarding reader/name publication. The helper does not redefine inactive-source provisioning semantics or claim complete dataset-selection/scientific correctness.

CreateDeFile retains the earlier candidate/file-validation staging and now reconnects consumers before success. Cached same-file/type readers also reconnect consumers, preserving the previous callers' rebinding behavior. Old readers remain retained until the enclosing update commits. SolarSystem SetSourceFile guards all affected bindings. CelestialBody SetSourceFile rejects null, obtains the provider filename/body ID first, then swaps/publishes the binding; lookup failure leaves its previous local binding intact.

The token covers fields changed by the reviewed native implementations. Arbitrary plugin/provider side effects, reentrant deletion or replacement of configuration objects, concurrent calls, retained interior addresses and external service/kernel-pool transactions are not qualified. It is not an all-application transaction for a multi-field GUI form.

Cache and scientific limits
----------------------------

Failure restoration includes the native cache flags and epochs changed by source setters. Successful source/file changes still require complete state/acceleration cache-coherence work: a reused epoch and a positive update interval can expose an older cached value after a reader/configuration change. The batch does not claim that all successful changes invalidate every cache or that state/acceleration/precision caches are independent. Private caller/frame and external reader lifetimes remain open.

No orbit, force, interpolation, covariance, attitude or ephemeris formula changes. The scope is configuration consistency and reader ownership during the reviewed update paths. Independent scientific truth, every body/frame/input mode and complete original persistence/function/resource requirements remain unproven.

Conditional builds and compilation
---------------------------------

Without __USE_SPICE__, SolarSystem DE-source selection does not access the unpopulated SPK-name list and a SPICE selection is rejected. A conditional syntax compilation exposed an existing unguarded SpiceOrbitKernelReader type/getter. An opaque forward declaration and null-return getter make the declared API available without that backend; the configured SPICE getter retains its behavior.

Configured compilation v1: 602/602 steps, 265.86 seconds, return 0.
Configured compilation v2: 66/66 steps, 3.96 seconds, return 0.
Configured compilation v3: 601/601 steps, 138.65 seconds, return 0.

Initial non-SPICE syntax compilation v1: return 1 at the unguarded type declaration; its log, receipt and source snapshot remain retained.
Corrected non-SPICE syntax v2, src/base/solarsys/SolarSystem.cpp: 0.48 seconds, return 0.
Corrected non-SPICE syntax v2, src/base/solarsys/CelestialBody.cpp: 0.46 seconds, return 0.

These conditional checks use the configured Linux compiler and omit the SPICE define. They do not link or qualify a complete non-SPICE build, other operating systems or runtime behavior. The final configured build also reports a QImage::mirrored deprecation warning from OrbitRenderer.cpp, a file outside this batch.

Five source/help paths have preimages before first edits, before/build/final hashes, indexed deltas, source map, logs and terminal compiler results. Final sources match configured v3 and corrected conditional v2 snapshots. All compiler sessions are terminal, added-line whitespace is clean and the help XML parses. Friend declarations and an internal token add no existing instance fields, public method signatures or virtual slots. Matching configured consumers rebuilt; external binary/plugin compatibility remains unqualified.

No runtime/GUI/mission/numerical tests, acceptance audits/readers, evidence admission, assertion/fixture/tolerance/denominator changes, packages, installed/shared runtime changes, commits, pushes or external messages were performed. The help describes individual native source/file assignments, not an all-GUI-form guarantee. No original all/any parent earns partial credit.

Tracking-v73 remains 1093/1158 (94.39%), with 65 other rows. The recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full goal active and incomplete.
