Function expression environments and native ephemeris-frame copies, 2026-11-06
===============================================================================

This implementation continues the original campaign; it does not close a broad
Function/resource requirement. No assertion, fixture, tolerance, requirement text,
fingerprint, denominator, acceptance ledger or qualification gate changed.

Native call preparation
-------------------------

MathTree initialization already supplies both scopes to each FunctionRunner.
That runner now resolves its callee from the incoming tables, with local entries
(including null declarations) shadowing global entries. It clears an old callee
before reporting an absent or incompatible binding. MatlabFunction is not an
ObjectManagedFunction and cannot enter this manager through an unchecked cast.
The existing standalone MATLAB call path is not changed by this batch.

Two nonvirtual FunctionManager methods separate expression call-site names and
runtime environments from native setters that rename the shared function.
Replacing a callee uses native definition assignment while the old callee is
still installed, retiring its invocation wrappers/results/FOS. Retained table
storage is held across assignment. Same-callee reentry retains the native stack
and permits another local table. Existing active-stack guards remain; incoming
caller tables owned by the retiring manager are explicitly rejected. Arbitrary
interior references and exceptional replacement are not declared complete.

Ordinary native environment preparation legitimately uses the current scope's
name lookup. The v40 passive retained-value binder still uses actual resource
identities. Engine global/force service sharing is unchanged. This batch adds no
Function resource admission, compiled-command graph copy or active checkpoint.

Owned ephemeris coordinate systems
------------------------------------

GetEphemStates previously shallow-copied coordSysOnFile even though its destructor
owns/deletes that frame. Its native copy constructor and assignment now stage a
separate typed CoordinateSystem Clone. Null/self/destination reuse and wrong
runtime types fail before adopting that pointer. Assignment holds the previous
function context, copies throwing values before publishing staged-frame aliases,
then retires the old frame. Direct output/internal-frame aliases to the source's
owned frame point at the cloned frame. Copy now validates the actual source type.
Other input/output/context references retain native borrowed semantics. General
cross-owner/interior aliases and retained function-resource lifetimes remain open.

Source and compilation
-------------------------

Four production paths, exact preimages, before/build/final hashes and indexed
patches are retained under:
``build/nasa-coverage/coverage95-20261106-implementation-function-environment``.
No fields or virtual slots were added. External platform/runtime behavior remains
unqualified. Build v1 failed because the new FunctionException uses needed their
own include; that include was added. Build v2 passed against the exact final four
source hashes. Receipts: v1 return 1/6.65s; v2 return
0/124.16s, last logged step ('74', '74').
The private integrated build compiled/linked targets, including test executables;
none were executed. New source lines have no trailing-whitespace changes.

No tests, GUI/mission/function/model/solver execution, numerical comparison,
acceptance reader/seal or verification campaign ran. No installed/shared runtime,
package, commit, push or external messages changed. Tracking-v41 remains
1093/1158=94.39%; recorded source-versioned qualified baseline remains 1053/1158
(90.9%), all 105 original acceptance rows open. Full implementation is incomplete.

POV clarification/research
-----------------------------

The user need not supply a dialect/profile or model. POV here means a spacecraft
3D model written in POV-Ray's scene language. The official `mesh2 tutorial
<https://www.povray.org/documentation/view/3.7.0/68/>`_ identifies meshes as a
modeler/converter use case and describes normal/UV/texture-index structures.
That corroborates an exported-mesh use case; it does not define the NASA import
profile or close the full original requirement. A fetch of the Blender exporter
source failed; no inspected-exporter compatibility is claimed. The existing
historical scope report and partial POV row dispositions remain in force.

Next
------

Implement retained Function/GmatFunction/native callable resource body/prototype graphs and passive reference providers without dropping compiled definitions. Continue direct RVECTOR nodes and every original resource/command/provider/global/caller/interior-alias/later/kernel/NAIF/recursive/exceptional/scientific obligation. POV scope still requires independent exporter/model research; do not ask the user to identify a profile. Finish implementation before verification.
