Captured covariance values and storage restoration
==================================================

Spacecraft, error-model and radio copies overwrote captured covariance entries using configured sigma priors. Covariance assignment also replaced matrix objects and retired its parameter-block cache. A restored estimated value could therefore lose posterior entries/correlations and invalidate an estimator's selected matrix storage. This batch preserves captured object covariance values and implements scoped restoration within existing layouts. Complete joint estimator/smoother checkpoints remain open.

Covariance matrix and temporary parameter block
-----------------------------------------------

Under NativeRestorationGuard, Covariance assignment validates dimensions, square sized/unsized representation, element names/IDs/sizes, owner-vector shape and covered ranges. It copies all entries into current matrix/backing storage and retains destination owner associations. If a parameter-block matrix was most recently returned, restoration also validates its shape and refreshes it in current storage. Its recorded metadata slot preserves the exact block selected by the existing first-match getter, including schemas with repeated IDs. Captured copies do not need a corresponding cache: the block derives from the full captured matrix.

Ordinary assignment still prepares a replacement and retires its prior matrix/cache. Parameter-block getter results retain the existing temporary lifetime contract: another block request can replace the previous result. Changed element/block layout requires native preparation and refresh rather than silently retiring an active view. This is not a claim that all public covariance/raw views survive every edit or ordinary assignment.

Posterior values and configured priors
--------------------------------------

Spacecraft and ErrorModel copies now retain the complete inherited covariance, including off-diagonal entries. Spacecraft maps owned covariance references into its corresponding cloned components. Configured OrbitErrorCovariance/sigma fields retain their own captured values; their explicit setters and default construction continue to initialize/update priors. Copying a value does not reconstruct posterior entries from those priors. Error-model restoration validates measurement/solve-for/model/pass roles, retains selected readiness and leaves failed application unready.

The spacecraft default initializer now supplies all nine declared diagonal values: six Cartesian entries, Cd, Cr and atmospheric-density epsilon variance. The previous eight-value initializer omitted the separately computed Cr variance and left the final declared slot unset. Existing variance formulas and configured defaults are used; assertions/tolerances are unchanged. The copy constructor now initializes attitude to null and guards copy-body cleanup of acquired owned components, plates and interpolation objects. Arbitrary foreign provider alias/failure behavior is not fully qualified.

Radio covariance, epochs and signal
-----------------------------------

Transmitter and Oscillator copies no longer overwrite captured frequency-bias, drift or polynomial covariance with configured sigmas. Both carry their resolved initialEpoch values instead of re-parsing configuration during copying; normal initialization/property behavior remains responsible for configuration edits. Transmitter capture keeps the signal value already copied by Sensor rather than replacing it with the configured constant frequency. Native frequency, drift, estimation and time-conversion algorithms are unchanged. Full hardware/measurement scientific equivalence remains unverified.

Compilation and compatibility
-------------------------------

Configured compiler v1: return 0, 182.92 seconds, last logged step 750/750.
Configured compiler v2: return 0, 176.66 seconds, last logged step 750/750.
Configured compiler v3: return 0, 2.12 seconds, last logged step 7/7.

Eight participating preimages preceded v1; three radio/help preimages were captured after terminal v1 and before their first edits/v2. Four planning snapshots preceded the first edits. Before/build/final hashes, indexed patches, capture note, source map, compiler logs and terminal receipts are retained. All compiler sessions are terminal; final sources match the last successful configured build. Added-line whitespace is clean and five help XML files parse. Covariance adds one cache-slot field and is embedded in GmatBase, so matching headers/consumers are required. No virtual slots were added. Full configured native/plugin/navigation/GUI consumers rebuilt; external binary/plugin and other platform compatibility remain unqualified. The unchanged EphemManager non-SPICE declaration limitation remains open without repeating its known failing conditional run. Compiler diagnostics are retained, including existing codecvt/QImage deprecations in full builds.

Finish concrete missing clauses in the original remaining requirement worklist without counting a compiled provider batch as completion of a broad all/any parent. Complete joint estimator/smoother posterior/state/result checkpoints and snapshot role remapping, changed covariance/spacecraft/body/location/topology at native preparation boundaries, public restoration-buffer contracts and full original function value behavior. Finish shared external atmosphere/burn/profile snapshot/value/lifetime contracts and raw/mesh/table/interpolator views beyond the supported unchanged covariance layout. Continue remaining native parent/clone owners and whole-spacecraft/plate/hardware edits, source/stream/participant/error/noise/hardware and top-level filter/physical-model updates, changed-list configuration remapping, external borrower/context lifetime, arbitrary provider/file failure cleanup and typed resize/reference providers. Finish all original function I/O/scoping/command/resource and nested Target/Optimize clauses, shared globals and reporting/wrappers; retain unresolved FRR-36/specification dependencies. Continue private renderer, any-Resource Save, body/kernel/NAIF/CSPICE-pool and ephemeris format/B-1/final-iterative/scientific scopes, including the existing EphemManager non-SPICE boundary. Preserve external/historical dependencies and independently research POV/rotation scope; defer runtime verification until implementation is complete.

No runtime/GUI/mission/numerical tests, acceptance auditors/readers/seals, evidence admission, fixture/assertion/tolerance/denominator/gate changes, packages, installed/shared runtime updates, commits, pushes or external messages were performed in this batch.

Tracking-v95 remains 1093/1158 (94.39%), with 65 other rows. The recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full goal active and incomplete.
