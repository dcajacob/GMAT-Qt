Estimator and smoother native function-value bindings
======================================================

Native BatchEstimator, ExtendedKalmanFilter and Smoother definitions previously had no retained function-value provider. Their owned configurations and estimation-state registrations could not be rebound as a complete private definition graph. Several ordinary copy paths also retained old runtime references or left smoothing pointers uninitialized. This batch supplies concrete native definition providers and repairs the required ownership paths. It does not claim full active-session checkpoint or all original function clauses complete.

Owned definitions and passive state binding
-------------------------------------------

The three concrete factory classes opt in through exact native type checks. Estimator supplies shared preparation and reference binding. Its disclosed children include owned propagation setups, tracking definitions, estimator-level filters and physical-model values from its estimation-state manager. Sequential filters and smoothers also disclose owned restoration snapshots; a smoother includes its owned sequential filter. Shape and actual identity correspondence must be registered before reference binding. Unsupported foreign subclasses do not silently inherit a capability claim from these concrete leaf providers.

The retained function graph maps actual participant, setup, tracking and filter references. Propagation assignments and solve-for/consider/participant declarations follow resolved object labels. EstimationStateManager rebinds retained objects, properties and numeric state through actual mappings, then clears prepared solve-for/consider/STM/covariance and propagation associations for native reconstruction. Owned physical-model copies are guarded, and native measurement rebinding maps the corresponding physical-model identities. Function binding stays passive: native RunEstimator/RunSmoother later prepare data streams, covariance and live measurement/process-noise associations.

Estimator copy construction retains consider declarations and guarded owned estimation filters through completion. Assignment prepares filter copies before publication, retires old runtime plotting/frame references and resets the actual stateCovariance convenience pointer instead of assigning NULL to the inherited covariance value. Existing file writer/session policy remains under native lifecycle; full rollback for arbitrary file/provider failures is not supplied.

Actual estimation filter association
------------------------------------

Estimator exposes its propagation, tracking and data-filter reference types, names and getters. Named filter attachment checks the declared slot, stages a concrete owned clone and retains actual source identity/context. Declared aliases name the attached value; self-replacement preserves the previous source edge. Native initialization now checks duplicate configured declarations separately from already associated values, prepares and initializes the complete selected filter list, checks false returns and publishes only after preparation. Already associated private filter values are used before the legacy configured-map fallback for unassociated names. Complete fallback scoping and updates to every filter/physical-model source remain open.

Sequential and smoother ownership
---------------------------------

Sequential constructors initialize isSmoothing/forwardFilterInfo and observation counters. Copies own their restoration buffers and copied update statistics. Assignment clears old live process-noise and measurement associations and closes the old output stream; copied error-bar data pointers target the destination sigma member. Destruction releases owned restoration snapshots. Concrete EKF assignment/preparation clear calculatedMeas/currentObs, and native batch copies no longer inherit a prepared hasInitialized flag. Batch assignment also copies the correct boolean resetBestRMSFlag member.

A smoother copies its forward-pass data, filter and restoration snapshots, starts at its filtering entry and points its filter at the destination's forward-pass vector. SetObjectBuffer clones supplied objects before retiring the previous owned buffer, so it no longer borrows another filter's snapshot lifetime. SetForwardFilterInfo swaps prepared data and rejects a missing filter before dereferencing it. Backward/result runtime state is rebuilt. Native filter/provider binding validates the actual owned filter identity.

EstimationStateManager BufferObjects stages internal and external snapshots and both raw pointer arrays before replacing either collection. Known borrowed/new-copy aliases are rejected; overlapping retired lists are deleted once. Snapshots have disabled notifications and no followed source edge. The function graph sends owned spacecraft, station, coordinate/formation and setup snapshots through their complete native resource binders instead of only generic reference setters. Complete restoration semantics under changed public names/components, arbitrary foreign providers and external raw-buffer borrowers remain open.

BatchEstimator, ExtendedKalmanFilter and Smoother Clone hold the new native value through retained-context binding. Copy rejects incompatible concrete input, retains the previous context across replacement and rebinds the source context. The existing native-copy guard prevents recursive binding during native graph construction. These hooks preserve independently bound definition values; they do not resume an active fit, integration, smoothing cursor or open I/O session.

Compilation and retained original scope
---------------------------------------

Configured compiler v1: return 1, 7.77 seconds, last logged step 8/85.
Configured compiler v2: return 0, 18.91 seconds, last logged step 81/81.

Compiler v1 caught an incorrect const-qualified call to the existing non-const GetFullName API in the alias helper. The helper now accepts actual mutable resource references, and the failed compiler log/receipt are retained. The corrected final sources match the last successful configured build.

Twenty-one existing source/help preimages and four planning snapshots were captured before their first edits/build. Before/build/final hashes, indexed patches, source map, compiler logs and terminal receipts are retained. All compiler sessions are terminal; added-line whitespace is clean and four help XML files parse. This batch overrides existing virtual slots and adds nonvirtual helpers; existing instance layouts remain unchanged. Matching configured navigation, EKF, bindings and GUI consumers rebuilt. External binary/plugin compatibility remains unqualified and requires consumers matching the current source tree. No installed/shared runtime or release package was changed. The previous unchanged EphemManager.hpp non-SPICE declaration limitation remains open; the known failing conditional run was not repeated.

Complete active estimator/smoother state/result reconstruction; snapshot aliases and original source-role remapping; raw public restoration-buffer borrowers; top-level filter/physical-model and full participant/error/noise/hardware/source/stream updates; changed-list/removed-component remapping; external borrower/context and public declaration lifetime; arbitrary provider/file failure cleanup; all remaining clone owners and every original function, nested Target/Optimize, scientific/persistence, specification/external/historical and FRR-36 scope remain open. No all/any parent receives partial credit.

No runtime/GUI/mission/numerical tests, acceptance auditors/readers/seals, evidence admission, fixture/assertion/tolerance/denominator/gate changes, packages, installed/shared runtime updates, commits, pushes or external messages were performed.

Tracking-v87 remains 1093/1158 (94.39%), with 65 other rows. The recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full goal active and incomplete.
