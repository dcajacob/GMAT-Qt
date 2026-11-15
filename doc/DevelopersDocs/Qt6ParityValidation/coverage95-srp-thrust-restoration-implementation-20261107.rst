Solar radiation pressure and thrust restoration
==============================================

SRP assignment replaced or abandoned numerical buffers and its shadow object. Finite-thrust assignment cleared active burn connections. File-thrust assignment cleared interpolators and did not capture their scratch data. Native restoration therefore needed concrete leaf paths in addition to the shared PhysicalModel buffer provider. This batch implements those paths within existing prepared configurations; it does not complete whole ODE or independent external-resource checkpoints.

SRP owned buffers and selected shadow bindings
----------------------------------------------

Native SRP copies capture independent three-element Sun vectors, force vectors sized to the physical model, and shadow bookkeeping including Sun/body vectors, last shadow epoch/body and body-list cache. Assignment stages buffers/shadow replacement before retiring the previous owned values. Destruction releases an allocated Sun vector regardless of analytic mode, fixing the previous ownership leak. Fresh runtime vectors initialize to zero before their first calculation.

Scoped restoration requires the same analytic/shadow/direction/shape configuration, extra shadow bodies, satellite/vector shapes and internal buffer presence. Actual Sun identity is checked when both references are exposed. Current Sun, shadow object and numerical buffers remain selected while captured values copy in place; inherited state/derivative/Jacobian storage uses the shared provider. Failed application leaves the SRP object unready; successful application retains its prior readiness. ShadowState contains environment references, not a separate dynamic history, and its current environment remains selected. Scientific shadow/force accuracy remains unverified.

Finite-thrust roles and lifetime checks
---------------------------------------

Native clones retain burn references, weak lifetime tokens, reference-role maps and spacecraft indexes. Function-value remapping still rebuilds its detached evaluator through the existing provider; frozen native captures suppress that remapping. Restoration checks current/source live burns, actual burn/reference and spacecraft connections, propagation/mass-flow roles and configured target identities. It preserves those selected connections and updates the inherited physical-model numerical values. Expired or changed bindings require native preparation before application. Burn object contents and firing transitions are separate whole-resource/checkpoint work.

File-thrust interpolation state
-------------------------------

New FileThrust instances initialize data/scratch arrays and index markers. Native clones capture scratch data, spacecraft/index roles and independently copied linear/spline interpolator objects, with local guards until publication. Restoration checks profile/segment vector identity, spacecraft/index and mass-flow roles, scale/angle estimation slots and interpolator presence. It applies captured thrust scale, warnings, data scratch and interpolator values into current objects, keeping current profile, spacecraft and frame bindings. Failed application leaves the force unready; success retains prior readiness. Internal interpolator array views and immutable independent external profile snapshots remain open. Ordinary assignment retains its existing cold interpolator lifecycle.

Compilation and retained scope
-------------------------------

Configured compiler v1: return 0, 4.46 seconds, last logged step 68/68. All compiler handles are terminal.

Six existing source/help preimages and four planning snapshots preceded the first edits/build. Before/build/final hashes, indexed patches, source map, compiler log and terminal receipt are retained. Final sources match the successful configured build. Added-line whitespace is clean and three help XML files parse. No new instance fields or virtual slots were added; matching configured consumers rebuilt. External binary/plugin/platform compatibility remains unqualified. The unchanged EphemManager non-SPICE declaration limitation remains open without repeating its known failing conditional run.

Finish drag/atmosphere restoration and its captured numerical buffers and selected atmosphere/frame/weather state. Complete ODEModel restoration without retiring its forces/internal frames or prepared propagation/measurement borrowers, including previous-state/dynamic-role and nonanalytic derivative values. Finish independent external burn/profile checkpoint values and raw/interpolator view lifetime; do not infer whole force-model restoration from leaf providers. Finish covariance matrix/subview lifetime and posterior semantics; changed spacecraft/body/location/topology at native preparation boundaries; complete active estimator/smoother state/result and snapshot role remapping; public restoration-buffer contracts and full original function value behavior. Complete source/stream/participant/error/noise/hardware and top-level filter/physical-model updates, changed-list configuration remapping, external borrower/context lifetime and arbitrary provider/file failure cleanup. Continue remaining clone owners, plate/other-hardware replacement, whole-spacecraft assignment and typed resize/reference providers. Finish all original function I/O/scoping/command/resource and nested Target/Optimize clauses, shared globals and reporting/wrappers; retain unresolved FRR-36/specification dependencies. Continue private renderer, any-Resource Save, body/kernel/NAIF/CSPICE-pool and ephemeris format/B-1/final-iterative/scientific scopes, including the existing EphemManager non-SPICE boundary. Preserve external/historical dependencies and POV/rotation research; defer runtime verification until implementation is complete.

No runtime/GUI/mission/numerical tests, acceptance auditors/readers/seals, evidence admission, fixture/assertion/tolerance/denominator/gate changes, packages, installed/shared runtime updates, commits, pushes or external messages were performed in this batch.

Tracking-v92 remains 1093/1158 (94.39%), with 65 other rows. The recorded source-versioned verified baseline remains 1053/1158 (90.9%); all 105 original acceptance rows remain open. Full goal active and incomplete.
