Attitude Parameters and two dispatch repairs — 2026-11-03
========================================================

Coverage remains **1,019/1,158 (88.0%)**, 139 original requirements open and
**82 more needed** for 1,101. Historical coverage remains 1,816/2,367. Original
fingerprints, assertions, whole-row conjunctions and all 59 prerequisite holds
remain. This checkpoint adds no whole-requirement credit. Technical, scientific,
full-current-build and full NASA qualification gates remain false.

Count correction — 2026-11-03
------------------------------

The original sealed receipt/audit retain a historical total of 1,344 strict
script assignments. The corrected executed total is 1,008: 56 passing script
probe invocations, each with nine ReadShared leaf-body executions and two
assignments per leaf. NestedShared wrapper calls do not execute those statements.
The correction and exact original report are bound by the subsequent Cartesian
frame checkpoint; native passes, original requirement coverage and all gates
remain unchanged. The reusable auditor now derives this total from actual leaf
callbacks; its exact previous source remains bound by that checkpoint.

Thirty-six live attitude Parameter routes
-----------------------------------------

All 36 remaining attitude types have native Parameter-to-Spacecraft references
in main, private function and explicit Global scopes. The witnesses check actual
attached attitude clones, actual EarthMJ2000Eq frame pointers and the live frame/
function SolarSystem. NadirPointing cases also require the actual reference-body
pointer from that live SolarSystem. Configured prototypes differ from runtime
objects; private owners/attitudes differ from their caller roots, and Global
references retain the root objects. Private pointers are used only during live
synchronous callbacks.

The family comprises nine DCM scalars, DirectionCosineMatrix, three Euler angles,
three MRPs, four quaternion scalars, Quaternion, three angular velocities, three
Euler rates, six body alignment/constraint components and three strings. Original
factory keys, native type labels, return types and writable flags remain. In
particular, the historical native label Attitude for the reference-body and
constraint-type Parameters is retained, without merging their factory entries.

Independent recipes normalize quaternion inputs [1,2,3,6], [2,3,5,7] and [3,4,6,9]
for the root/private/Global scopes. Passive quaternion matrices, 321 Euler angles,
MRPs and Euler rates are computed independently in Python with elementary
arithmetic and trigonometry, without calling GMAT conversions. Angular-velocity
inputs are [0.125,0.25,0.375], [0.25,0.375,0.5] and [0.375,0.5,0.625] deg/sec.
Selected values agree with direct native attitude getters and actual Write/
Report output at absolute bound 1e-10. Quaternion and matrix Report paths use
4x1 and 3x3 Array bridges; they retain typed native values and shapes.

NadirPointing uses distinct nonparallel literal body vectors and Earth/Mars/
Jupiter references. The Attitude string uses CoordinateSystemFixed, Spinner and
ThreeAxisKinematic. Thirty-five selected values distinguish all three scopes;
AttitudeConstraintType has only the two valid values OrbitNormal/Velocity, so its
root/Global value repeats while actual owner/attitude identities remain mandatory.
The owner epoch stays exactly 21545. Separate pulse spacecraft execute callback
boundaries; nonzero angular velocities here do not qualify attitude propagation.

Separate script and native write paths
--------------------------------------

All 28 advertised writable types retain strict idempotent private and Global
script assignments. All pass in both modes after repair. Native idempotent
Parameter setters are observed separately from script property wrappers. The
initial full run showed that MRP script assignments passed while all three native
MRP setters refused with AttitudeData's unknown-item error. Strict native mutation
probes reproduced the failures; original assertions and receipts remain.

AttitudeData.cpp now forwards the three existing MRP identifiers to the existing
attached Attitude scalar setters, matching its other orientation fields. Native
probes then change one selected MRP by 1/16, require the actual Parameter and
attached owner field to change, compare all nine matrix components with an
independent MRP-to-quaternion-to-matrix recipe, restore the value, and compare the
full original matrix again. Main/private/Global cases include repeated and nested
calls, Unicode reopen and a no-Build repeat.

The strict Quaternion script probe originally failed because ElementWrapper
evaluated an Rvector RHS but did not forward it to an Rvector LHS. The repair
calls the existing LHS SetRvector method for that matching type. Existing string/
matrix paths and downstream owner dimension checks remain. No conversion formula,
writable flag, factory/role, member or virtual slot changes.

All three advertised mutable system Rvector types are covered: Quaternion and
ThrustAngle1/2. Six new ThrustAngle controls at lengths 1/3/4 first reproduced the
same original assignment failure. After repair they check actual source Parameter/
ThrustSegment references, independent changing source/target vectors, main/private/
Global assignments and both source/target Write output. Existing literal source
updates and Array-bridge Report paths also execute. Arbitrary vector properties,
dimension-error domains and thrust-history force science are not qualified.

Evidence, failures and source custody
-------------------------------------

**146 current GUI passes** comprise 67 attitude cases and six vector controls in
both unattended and visible modes. Fifty-seven unaffected unattended passes are
reused at their exact pre-repair source versions; ten affected attitude cases and
six vector controls pass after repair. The visible run replays all 73 on the final
build. The checkpoint checks 438 missions, 1,752 calls/callbacks and 3,504 target
owner observations, with 2,190 markers. Attitude-specific observations include
3,216 clone/frame/SolarSystem/typed-output checks, 3,888 expanded values per output
channel, 1,008 strict script assignments, 2,832 accepted native idempotent setters
and 144 non-idempotent MRP mutation/restore roundtrips with 2,592 matrix components.
The related vector controls add 288 source/target checks and assignments and
768 components per source or target output channel.

Actual file-editor keyboard Cancel/Save, exact source/history/prototype/caller
isolation, report viewing, Unicode reopen and Build/no-Build controls remain.
The visible run records **73 process maps and 1,241 exposure events**, unchanged
source/image inventories, initially minimized resizable/nonfullscreen windows
and software rendering. Screenshots, runtime-binary copies and kernel copies are
zero. Nineteen attitude corrupt-proof controls and separate vector source/shape
controls remain auditable.

**Thirteen failed GUI receipts** remain: three configured-SolarSystem fixture
comparisons, one original Quaternion script assignment, three strict native MRP
mutations and six original dynamic vector assignments. Twelve earlier passes are
retained as superseded: six initial pilots and six pre-repair MRP cases. No failed
obligation is renamed as a passing negative test. Five builds passed; no compile
failure or native interruption occurred. All native/build handles are terminal.

Two offline authoring guards failed before native work: an overly broad leftover
token check matched the debugger's valid kind argument, and an incomplete patch
context rejected the atomic edit before changing any file. A source-freeze
assertion later caught an offline auditor edit: RunAcceptance automatically binds
Python helpers even when native drivers do not execute them. The exact earlier
auditor bytes were recovered from an already frozen capsule and matched the
receipt SHA-256. The partial capsule, original records and recovery provenance
remain; native driver/core inputs were unchanged by that auditor edit.
The final offline auditor also distinguishes vector arity case arguments from
factory type names when checking raw statements. Its inventoried preimage and
exact final revision are separately bound; native assertions and images do not
change, and no extra native replay is substituted for offline verification.
The first final audit stopped on a missing metadata key in its temporary RHS
source-Write specification. Supplying the unchanged original metadata corrects
that parser input; the failed verifier and exact log remain separately bound.

Core inventory is **245 paths**: all prior 243 working hashes unchanged plus
AttitudeData.cpp and ElementWrapper.cpp, both clean HEAD before this checkpoint.
Exact HEAD/working files, patches and source-version capsules remain. The affected
unattended receipt binds 237 paths; eight non-C++ paths are additionally bound
statically. Visible custody binds all 245. This does not grant general ABI or
full-current-build approval.

Replay and remaining work
-------------------------

Reusable files are NasaFunctionAttitudeParameterBindingTests.cpp,
NasaFunctionAttitudeParameterCases.json, FunctionAttitudeBindingOracle.py,
RunFunctionAttitudeParameterBindings.py, AuditFunctionAttitudeParameterBindings.py,
NasaFunctionVectorAssignmentTests.cpp and RunFunctionVectorAssignments.py under
src/qtgui/tests. The vector controls reuse the unchanged complete ThrustAngle
catalog. Both acceptance targets depend on both executables. All 73 registrations
retain 180-second timeouts and the engine lock. Script/native writable probe
labels remain separate. From the repository root, use fresh output paths::

   cmake --build build/linux-gui --target GmatQtNasaFunctionAttitudeParameterBindingTests GmatQtNasaFunctionVectorAssignmentTests -j 6
   python3 src/qtgui/tests/RunFunctionAttitudeParameterBindings.py --output build/gui-acceptance/attitude-replay
   python3 src/qtgui/tests/RunFunctionAttitudeParameterBindings.py --visible --output build/gui-acceptance/attitude-visible-replay
   python3 src/qtgui/tests/RunFunctionVectorAssignments.py --output build/gui-acceptance/vector-assignment-replay
   python3 src/qtgui/tests/RunFunctionVectorAssignments.py --visible --output build/gui-acceptance/vector-assignment-visible-replay

Authoritative audit, matrix, worklist, manifest, receipt and seal are v1 in
build/nasa-coverage/coverage95-20261103-function-attitude-parameter-bindings/.
Current attitude unattended witnesses use headless-v2 or affected headless-v4;
vector witnesses use vector-headless-v2; all visible witnesses use visible-v1.
Earlier native versions stay source-bound. All 566 original rows, 321 Parameter
types and previous fields remain. Native owner evidence covers **186 types**;
**135 remain** (121 Spacecraft, 14 SpacePoint), including 41 older bounded names.
There are 227 names with any joined bounded context and 94 without. All 321
whole obligations remain open.

Other Euler sequences, singular/shadow/sign branches, propagated attitude,
arbitrary frames/providers, general native/script mutations and read-only GUI
destination contracts remain open. Preserve earlier R_SB strict writes, derived
thruster write/propulsion gaps, E491/E477/E406 and other provider/science conflicts.
No licensed MATLAB installation is available per the user; all 37 MATLAB holds
remain without substitute or waiver. All 59 prerequisite holds remain. Continue
time/frame/orbit/provider work and substantive prerequisite resolution. The full
95% goal remains active and incomplete.
