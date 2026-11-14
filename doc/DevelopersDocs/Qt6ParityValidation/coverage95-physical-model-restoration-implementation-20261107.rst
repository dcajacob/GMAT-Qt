Physical-model buffers and native gravity restoration
=====================================================

PhysicalModel copies previously collapsed separate model/raw state arrays and omitted optional mass/time Jacobian columns from copy construction. Assignment retired exposed state/derivative/Jacobian storage, borrowed captured state/space-object references and cleared readiness. The destructor did not release the optional Jacobian columns. This batch supplies complete copying of these owned arrays and a scoped restoration provider for unchanged propagation layouts. Whole ODE/force-model restoration remains open.

Owned numerical values
-----------------------

CopyOwnedArrays prepares all model, separate raw, derivative and optional Jacobian arrays with local ownership before replacing the destination arrays. Raw/model alias relationships are preserved, including absent buffers. Copy construction and ordinary assignment both use the same operation. ReleaseOwnedArrays retires each owned array once, including both Jacobian columns; the existing DEBUG_MEMORY registration path includes the new ownership. The pre-existing totalSTMSize field now initializes to zero and copies with the state-role metadata. Negative allocation dimensions reject. No new instance fields or virtual slots are added.

Scoped restoration
-------------------

The nonvirtual layout validator checks dimensions, total STM size, precision time, Cartesian/STM/A-matrix roles, derivative IDs/names, Jacobian capabilities/fill flags and buffer presence/alias relationships. It also checks actual body, origin, solar-system, state-manager and state identity when both sides expose them. Changed layouts require native preparation. Under NativeRestorationGuard, PhysicalModel restores values/epochs into current buffers and retains current propagation manager, GmatState, spacecraft, body/origin/solar-system and ODE ownership bindings. Successful restoration preserves current readiness. Ordinary copies retain their existing cold initialization lifecycle. Base covariance storage and arbitrary provider-exception rollback are separate remaining contracts.

Gravity and relativistic leaves
--------------------------------

PointMassForce validates unchanged primary-body/spacecraft roles and preserves the shared provider's readiness. HarmonicField validates potential file, degree/order/STM limit and frame roles, retaining current loaded-file state and selected frames/EOP. GravityField additionally validates tide settings and retains its prepared gravity model and J2000 frame. RelativisticCorrection retains selected Sun and owned frames/EOP and applies captured numeric values, including the solar gravitational parameter omitted by its previous assignment. GravityField and RelativisticCorrection copy constructors now rely on inherited derivative/type declarations rather than appending duplicate entries on each clone. These providers preserve existing scientific formulas; runtime numerical correctness is not inferred from compilation.

Remaining owner reconstruction
-------------------------------

ODEModel ordinary Copy still replaces force roots and internal frames and clears prepared engine associations. Drag, solar radiation pressure, finite thrust and file thrust still need complete restoration providers for their own arrays and atmosphere/shadow/burn/file/interpolator bindings. The shared PhysicalModel provider does not make these owner graphs complete. Changed topology and layout must be handled at native preparation boundaries. Full active estimation/smoother checkpoints, covariance/posterior semantics, all external borrowers and original broad function/resource requirements remain open; no partial parent credit is admitted.

Compilation and retained scope
-------------------------------

Configured compiler v1: return 0, 112.58 seconds, last logged step 339/339. All compiler handles are terminal.

Nine existing source/help preimages and four planning snapshots preceded the first edits/build. Before/build/final hashes, indexed patches, source map, compiler log and terminal receipt are retained. Final sources match the successful configured compiler snapshot. Added-line whitespace is clean and three help XML files parse. Matching configured consumers rebuilt; external binary/plugin and other platform compatibility remain unqualified. The unchanged EphemManager non-SPICE declaration limitation remains open without repeating its known failing conditional run. Compiler diagnostics, if any, are retained in summary.json.

Complete ODEModel restoration without retiring its forces/internal frames or prepared propagation/measurement borrowers. Implement the remaining drag/SRP/finite/file-thrust providers, including captured internal arrays, atmosphere/shadow/file/interpolator/burn bindings and snapshot preparation; do not infer full force-model restoration from the shared physical-model provider. Finish covariance matrix/subview lifetime and posterior semantics; changed spacecraft/body/location/topology at native preparation boundaries; complete active estimator/smoother state/result and snapshot role remapping; public restoration-buffer contracts and full original function value behavior. Complete source/stream/participant/error/noise/hardware and top-level filter/physical-model updates, changed-list configuration remapping, external borrower/context lifetime and arbitrary provider/file failure cleanup. Continue remaining clone owners, plate/other-hardware replacement, whole-spacecraft assignment and typed resize/reference providers. Finish all original function I/O/scoping/command/resource and nested Target/Optimize clauses, shared globals and reporting/wrappers; retain unresolved FRR-36/specification dependencies. Continue private renderer, any-Resource Save, body/kernel/NAIF/CSPICE-pool and ephemeris format/B-1/final-iterative/scientific scopes, including the existing EphemManager non-SPICE boundary. Preserve external/historical dependencies and POV/rotation research; defer runtime verification until implementation is complete.

No runtime/GUI/mission/numerical tests, acceptance auditors/readers/seals, evidence admission, fixture/assertion/tolerance/denominator/gate changes, packages, installed/shared runtime updates, commits, pushes or external messages were performed in this batch.

Tracking-v91 remains 1093/1158 (94.39%), with 65 other rows. The recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full goal active and incomplete.
