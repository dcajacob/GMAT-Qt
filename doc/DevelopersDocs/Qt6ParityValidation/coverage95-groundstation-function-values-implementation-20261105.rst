Owned GroundStation function values, 2026-11-05
===============================================

FunctionManager explicitly borrowed GroundStation inputs rather than cloning
them. Function code could therefore change a non-global caller station without
returning it. Stations were also excluded from recursive value snapshots, local
object-result ownership and cleanup. Removing those exemptions alone was unsafe:
GroundStation's copy constructor omitted owned hardware/error models, assignment
retained old collections, and copied BodyFixedPoint frames require initialization.

Owned component copies
------------------------

GroundStation now copies hardwareList, configured errorModels and per-signal-path
errorModelMap values. StationObjectCopies stages all clones and reconnects
intra-station hardware references, including antennas, before swapping the owned
collections into the destination. The same guard destroys the destination's old
components after replacement and retires partial new allocations on failure.
Null/source-returning Clone failures are handled without deleting the source.

Configured and path-specific error-model full names follow the destination
station; SetName updates them for formal aliases and renamed resources. The
existing station ID and path model values remain copied data. Cached visibility
resets and copied stations are marked uninitialized, consistent with detached
BodyFixedPoint local frames/kernels. The destructor uses the same owned-collection
cleanup. Generated Imager/FOV ownership remains in the existing Imager copy path.

This stages the owned graph before publication; it does not add a strong
transaction around every native base/scalar assignment failure. External FOV,
body/frame and other references outside the station-owned collections remain
part of the general private/transitive dependency scope.

Function and assignment paths
------------------------------

GroundStation inputs now follow the normal owned formal clone and wrapper path.
They are renamed to the formal input, marked local/non-global and evaluated
through the formal object store. Repeated inputs reuse/copy their formal root.
A broken input Clone returning its source fails before ownership is taken.

The general ownership predicate, recursive saved local values, unnamed local
object-result snapshots and cleanup now include stations. True globals retain
their borrowed identity. Subscriber-before-dependency cleanup ordering is
unchanged. EmptyObjectMap's old station exemption is removed; its historical
DeleteObjectMap call remains disabled, as before, rather than changing unrelated
legacy map ownership.

ObjectInitializer::InitializeGroundStation is a nonvirtual targeted method: it
restores the selected station's J2000/body/reference bindings and initializes
that station. It does not call InitializeObjects(GROUND_STATION), which also
initializes spacecraft/regions and can include global stores. SetRefFromName
recognizes existing station-owned hardware/error-model attachments before
looking for external templates, so an owned component need not also be Global
inside a function and retains its copied values.

GmatFunction initializes uninitialized local station values after its ordinary
initialization path, covering refreshed inputs and recursive PopFromStack
restoration even when full object initialization is skipped. False station
initialization fails the call. ObjectWrapper initializes a copied station
assignment/output before subsequent commands use its detached frames; self/
shared-global identity assignments do not add initialization.

Remaining scope and compilation
--------------------------------

General private/transitive returned resource graphs, other resource classes,
function-definition assignment/rebuild and shared SPICE kernel-pool isolation
remain open. Generated horizon masks retain native initialization/reload/name
behavior. File resolution, masks/external FOVs, error-model metadata, cached
parameter references, geometry and recursive ownership still require their
later qualification and broader graph implementation review. No universal
function I/O, scoping or resource-clone parent closure is claimed.

The private integrated build passes 184 steps / 174.31 seconds, session 7554
completed. Core, Qt, console, enabled plugins and Python bindings compile/link.
The existing OrbitRenderer QImage::mirrored deprecation warning remains. Final
source hashes match at-build-v1; XML syntax was checked during editing. No tests,
GUI, missions/functions/models/solvers, numerical comparisons, memory checks,
acceptance readers or seals were run. Linked test/probe clients were not run.

Stage: build/nasa-coverage/coverage95-20261105-implementation-groundstation-function-values.
Eight preimages, before/at-build/final hashes, exact patches, source notes,
build log/receipt and summary-v1 are retained. Tracking-v11 updates implementation
notes without adding rows: 1092/1158 (94.30%), 66 other original rows. Recorded
verified acceptance stays 1053/1158 (90.9%); all 105 acceptance rows remain open.
No original requirement/text/fingerprint, assertion, fixture, tolerance,
denominator, ledger or qualification gate changed.
