Spacecraft restoration within existing components and state storage
===================================================================

Native whole-spacecraft assignment retired owned hardware/attitude/power roots and state/STM storage, and reset selected runtime bindings. Active estimation/propagation borrowers could retain those retired pointers. This batch supplies a scoped restoration path for an unchanged spacecraft graph and state layout. It does not complete full active-engine restoration.

Owned identities and state storage
----------------------------------

Spacecraft discloses direct attitude and power-system slots followed by tanks, thrusters, other hardware and plates through the existing function-value owned-object interface. Stable canonical slots avoid duplicate aggregate getter roles. NativeRestoration validates native concrete types, role names and repeated identities, then copies captured values into existing roots, retains component names/full names and contexts, and maps covariance owner links to current children. Failed spacecraft restoration leaves the owner unready; arbitrary provider-exception rollback remains open.

GmatState gains nonvirtual layout validation and value restoration. State size, precision-time mode, type descriptions, data IDs and associated-element roles must agree. State values and both epoch representations are copied elementwise without replacing state/ID/association arrays. Newly allocated ID/association slots initialize to -1 rather than remaining indeterminate. SpaceObject uses this path under the restoration guard and retains current origin bindings. Ordinary assignment still follows its existing allocation policy.

Spacecraft validates matching component declarations, frame/process-noise roles, solve-for/STM roles, shape models and SPAD file/interpolation settings, plus actual environment identity when both bindings are exposed. Same-shape full STM and A-matrix values copy elementwise; existing storage stays selected. Environment pointers, external STM sources, solve-for parameter objects, SPAD readers and frames remain selected. Origin-relative state and coordinate-transform caches invalidate for recomputation. Private mass-property interpolators still use native copies. This is not a claim about every internal buffer or external view.

Component runtime bindings
--------------------------

Thruster restoration requires unchanged tank/frame roles, mixture size and firing status. Current solar-system, origin, frame, spacecraft, tank and derivative bindings remain selected while captured native thrust fields copy. Power systems retain current owner/Sun/origin references. Solar power additionally requires the same shadow configuration and reader presence, retaining current shadow body references and ShadowState. Internal shadow history is not a complete checkpoint.

Attitude retains current spacecraft/frame/body associations while copying its existing native numerical state. Changed reference roles, attitude model or AEM path require preparation. CCSDS and SPICE attitude subclasses keep the current reader for compatible file/kernel roles; this does not reconstruct or qualify global CSPICE state. Plates keep current history readers and frame bindings for compatible history roles, applying native captured area, normal, reflectance and location values. Changed file/component/burn configuration needs native preparation; that broader path remains open.

Covariance and remaining restoration work
-----------------------------------------

Covariance owner pointers map to current components, but GmatBase covariance assignment can still retire matrix/submatrix storage. Spacecraft retains its existing covariance recomputation formulas, so arbitrary posterior covariance restoration is not claimed. ODEModel and PhysicalModel still require providers that preserve force identities, native buffers and prepared engine associations. Complete changed-topology restoration, all native borrowers, active estimator/smoother results and scientific equivalence remain open. No broad parent requirement receives partial credit.

Compilation and retained scope
-------------------------------

Configured compiler v1: return 0, 138.85 seconds, last logged step 467/467. The terminal tool receipt closed session 80004.

All sixteen existing source/help preimages and four planning snapshots preceded the first edits/build. Before/build/final hashes, indexed patches, source map, compiler log and terminal receipt are retained. Final source hashes match the successful configured build. Added-line whitespace is clean and four help XML files parse. The build emitted the existing Qt QImage::mirrored deprecation warning in OrbitRenderer.cpp. No new instance fields or virtual slots are added by this batch; Spacecraft overrides an existing slot and GmatState adds nonvirtual methods. Matching configured native consumers, plugins, navigation bindings and GUI consumers rebuilt; external binary/platform compatibility remains unqualified. The unchanged EphemManager.hpp non-SPICE declaration limitation remains open without repeating its known failing conditional run.

Implement identity-preserving restoration for ODE/force and remaining parent graphs, including native state/derivative/Jacobian storage and propagation/measurement borrowers. Finish covariance matrix/subview lifetime and posterior semantics; changed spacecraft/body/location/topology at native preparation boundaries; complete active estimator/smoother state/result and snapshot role remapping; public restoration-buffer contracts and full original function value behavior. Complete source/stream/participant/error/noise/hardware and top-level filter/physical-model updates, changed-list configuration remapping, external borrower/context lifetime and arbitrary provider/file failure cleanup. Continue remaining clone owners, plate/other-hardware replacement, whole-spacecraft assignment and typed resize/reference providers. Finish all original function I/O/scoping/command/resource and nested Target/Optimize clauses, shared globals and reporting/wrappers; retain unresolved FRR-36/specification dependencies. Continue private renderer, any-Resource Save, body/kernel/NAIF/CSPICE-pool and ephemeris format/B-1/final-iterative/scientific scopes, including the existing EphemManager non-SPICE boundary. Preserve external/historical dependencies and POV/rotation research; defer runtime verification until implementation is complete.

No runtime/GUI/mission/numerical tests, acceptance auditors/readers/seals, evidence admission, fixture/assertion/tolerance/denominator/gate changes, packages, installed/shared runtime updates, commits, pushes or external messages were performed in this batch.

Tracking-v90 remains 1093/1158 (94.39%), with 65 other rows. The recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full goal active and incomplete.
