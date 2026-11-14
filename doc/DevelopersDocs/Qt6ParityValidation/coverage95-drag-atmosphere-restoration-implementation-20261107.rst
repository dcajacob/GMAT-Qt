Drag and owned atmosphere restoration
=====================================

Drag copies previously dropped density/prefactor/translated-state values and spacecraft coefficient vectors. Assignment abandoned old numerical buffers and replaced its atmosphere. Atmosphere copies reset angular-velocity/geodetic/weather state, and solar-flux reader copies reset coverage/index metadata. This batch provides owned value capture and scoped restoration for the unchanged prepared drag/owned-atmosphere path. Whole ODE and shared external-resource restoration remain open.

Drag storage and atmosphere ownership
-------------------------------------

CopyDragBuffers prepares all three independent numerical arrays before retiring prior storage, and preserves absent buffers. Destruction and ordinary assignment release prior arrays. Fresh native preparation zeros scratch arrays before their first capture. Exact-type atmosphere Clone validation rejects borrowed/source/current/owner aliases and stages the owned result with source/owner/context metadata. Copy construction retains numerical spacecraft/coefficient vectors, captured prefactor/cache values and force-local Sun/body samples; the owned atmosphere's vector pointers reconnect to the new force's embedded arrays. GetFunctionValueOwnedObjects discloses the internal atmosphere through an existing virtual slot.

Scoped restoration validates propagation/estimation indexes, vector shapes, shape/body/weather/file configuration, buffer and selected atmosphere roles, angular-velocity presence and exposed frame identity. It copies values into existing raw/vector storage, applies concrete Copy to the existing internal atmosphere and maps covariance owners. Current atmosphere, angular-velocity and frame pointers remain selected. A scope guard leaves failed drag restoration unready; success retains prior readiness. Ordinary assignment remains cold and stages its atmosphere clone before replacement. Arbitrary provider/allocation rollback is not supplied.

Atmosphere and solar-flux values
--------------------------------

Atmosphere copies capture an independent SolarFluxReader, loaded status, both constant/nominal geomagnetic amplitudes, flux samples, angular velocity, geodetic/GHA caches and the last flux-data record. Fresh geomagnetic samples initialize before capture. Ordinary assignment retires its previous reader and its owned J2000 frame under the native cold lifecycle. Scoped restoration validates body/file/source/representation and exposed environment/frame identity, retains the existing reader, frame ownership, Sun/body-vector pointers and environment references, and restores captured scalar/array/reader values. Failed scoped application leaves the atmosphere unready.

SolarFluxReader copies now preserve loaded tables, coverage ranges, daily end, Schatten indexes, warning flags, source/interpolation settings, offsets and static input tags. The temporary parse-line pointer is reset instead of borrowed. File streams remain separate native handles; file-stream positions, arbitrary file/provider failure cleanup and every external reader borrower are not qualified.

Concrete atmosphere tables and plugins
--------------------------------------

ExponentialAtmosphere prepares all three tables with local array ownership before publication, preserves smoothing state and retires prior tables on assignment. Scoped restoration requires matching file/band roles and updates existing table storage. SimpleExponentialAtmosphere and NRLMSISE00 Copy now enforce concrete native type instead of slicing/base fallback or unchecked casts. NRLMSISE00 native copies retain fileData state. Existing MSISE86/MSISE90/Jacchia native providers use the shared atmosphere path. Scientific density and external-library/global Fortran state remain unverified.

Remaining restoration scope
---------------------------

A body-owned/shared external atmosphere remains selected; its independent snapshot/value/lifetime contract is still open. This batch does not claim to capture every external dependency simply because the force's own fields are captured. ODEModel still needs whole force-list/frame/engine restoration, and complete burn/profile/active estimator/smoother checkpoints, covariance/posterior semantics, changed topology/layout preparation and every original broad function/resource clause remain open. No all/any parent receives partial credit.

Compilation and retained scope
-------------------------------

Configured compiler v1: return 0, 32.83 seconds, last logged step 81/81.
Configured compiler v2: return 0, 3.66 seconds, last logged step 67/67.

Eleven existing source/help preimages and four planning snapshots preceded the first edits/build. Before/build/final hashes, indexed patches, source map, compiler logs and terminal receipts are retained. All compiler sessions are terminal; final sources match the last successful configured build. Added-line whitespace is clean and three help XML files parse. No instance fields or virtual slots were added; new methods are nonvirtual or override existing slots. Matching configured native/plugin/navigation/GUI consumers rebuilt, including the NRLMSISE00 source. External binary/platform compatibility remains unqualified. The unchanged EphemManager non-SPICE declaration limitation remains open without repeating its known failing conditional run.

Complete ODEModel restoration without retiring force roots/internal frames or prepared propagation/measurement borrowers, including previous-state/dynamic-role/nonanalytic derivative values. Finish shared external atmosphere snapshot/lifetime/value contracts, independent burn/profile checkpoint values and raw/table/interpolator view lifetime; do not infer whole force-model restoration from leaf providers. Finish covariance matrix/subview lifetime and posterior semantics; changed spacecraft/body/location/topology at native preparation boundaries; complete active estimator/smoother state/result and snapshot role remapping; public restoration-buffer contracts and full original function value behavior. Complete source/stream/participant/error/noise/hardware and top-level filter/physical-model updates, changed-list configuration remapping, external borrower/context lifetime and arbitrary provider/file failure cleanup. Continue remaining clone owners, plate/other-hardware replacement, whole-spacecraft assignment and typed resize/reference providers. Finish all original function I/O/scoping/command/resource and nested Target/Optimize clauses, shared globals and reporting/wrappers; retain unresolved FRR-36/specification dependencies. Continue private renderer, any-Resource Save, body/kernel/NAIF/CSPICE-pool and ephemeris format/B-1/final-iterative/scientific scopes, including the existing EphemManager non-SPICE boundary. Preserve external/historical dependencies and POV/rotation research; defer runtime verification until implementation is complete.

No runtime/GUI/mission/numerical tests, acceptance auditors/readers/seals, evidence admission, fixture/assertion/tolerance/denominator/gate changes, packages, installed/shared runtime updates, commits, pushes or external messages were performed in this batch.

Tracking-v93 remains 1093/1158 (94.39%), with 65 other rows. The recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full goal active and incomplete.
