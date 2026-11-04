Thirteen Plate Parameter owner routes — 2026-11-03
=================================================

Coverage remains **1,019/1,158 (88.0%)**: 139 original requirements open and
**82 more needed** for 1,101. Historical operation coverage remains 1,816/2,367.
All original fingerprints, assertions, whole-row conjunctions and 59 prerequisite
holds remain. No requirement credit is added. Technical, scientific, full-current-
build and full NASA gates remain false.

All thirteen types retained
---------------------------

The complete Plate family now has actual native Parameter-to-Spacecraft
references and distinct attached, initialized Plate identities in main, private
function and explicit Global scopes:

* AreaCoefficient, SpecularFraction and DiffuseFraction retain writable metadata
  and execute command-mode mutations in private and Global scopes.
* Area, LitFraction, the three coefficient sigmas and PlateX/Y/Z retain read-only
  Parameter metadata and read independently configured literal fields.
* PlateNormal reads a three-component initialized vector. The root/private/Global
  inputs [1,2,2], [2,1,2] and [2,2,1] normalize to length one by division by 3.
* PlateNormalHistoryFile reads distinct stored filename strings.

All 321 original system types, metadata and roles remain, together with Array,
String and Variable in the actual complete factory-name check. All 566 matrix
rows and prior fields remain unchanged; new witnesses are additive. No factory
or readable-role exclusion is introduced.

Three typed pilots pass in headless-v2 and are reused; headless-v3 runs only the
remaining ten types. All thirteen pass visibly. **26 GUI passes** check 78
missions, 312 repeated/nested calls and synchronous callbacks, 624 native owner/
attached-plate/direct-field/typed-Write/typed-Report observations and 390 markers.
These comprise 528 scalar fields, 48 vectors (144 scalar components), and 48
stored strings: 720 expanded scalar-or-string values in each output channel.
The numerical bound remains 1e-12; strings compare exactly.

Each callback verifies the actual Parameter reference, owner flags and pointer,
the actual attached Plate clone versus caller/configured objects, the singleton
AddPlates dependency-name list and initialized state. Private maps are inspected
only synchronously while the owning function is executing. No expired private
pointer is accessed after return. Private mutations leave the caller shadow's
selected field, state and sentinel unchanged; explicit Global uses the root
sandbox owner and Plate. Native Parameter/ParameterInfo writable flags match the
original metadata for all thirteen types.

All cases retain function-file keyboard Cancel/Save, exact caller source/history
and configured-prototype isolation, Unicode save/reopen, Build followed by
no-Build repeat, raw Write/Report bytes and actual report viewing. PlateNormal
uses an actual 3x1 Array bridge to report its three components; direct vector
Report/chooser behavior is not inferred. The remaining types use direct Report.

Fixture failures and limits
---------------------------

All three original pilots failed at caller Build because the fixture attempted
the nonexistent Spacecraft script field SRPShapeModel. Exact failed source,
scripts, logs, native receipts and source capsule remain. The corrected fixture
uses the documented AddPlates route. Spacecraft initialization initializes its
attached plates independently of SRP model selection; the actual native
GetSRPShapeModel accessor reports Spherical. PulseFM.SRP remains Off.
No production source or original NASA assertion/bound changes. Both builds pass;
there are no compile failures or interruptions, and all handles are terminal.

FixedInBody and SRP Off are explicit fixture limits. History filenames are stored
strings; Type=File, file loading/interpolation, SunFacing behavior and file-provider
identity are not exercised. The proof's provider-used=false is a configured/source-
mode scope annotation, not an independent I/O trace or observed private pointer.
Plate geometry, SRP illumination/reflectance/forces, sigma/covariance estimation,
general frames, unit labels and read-only Assignment chooser/write contracts
remain unqualified. No physical correctness follows from literal field reads.

Visible custody and unchanged core
----------------------------------

All thirteen visible cases have actual process maps and **221 exposure events**.
Source/image hashes are identical before/after replay. Windows start minimized,
are resizable and nonfullscreen, and use software rendering. Screenshots,
runtime-binary copies and kernel copies are zero. Twelve corrupt-proof controls
check owner/clone/class/list/init/value/vector/string/metadata/repeat/marker
failures. Exact native JUnit, raw files and source versions are audited.

All **242 pending core hashes are unchanged** from the sealed equation-assignment
stage. The unattended receipt binds 234 core paths; eight additional non-C++
sources are statically bound. Visible custody includes all 242. There is no
production increment and no technical or general ABI approval.

Replay and evidence
-------------------

Reusable files under src/qtgui/tests are
NasaFunctionPlateParameterBindingTests.cpp, NasaFunctionPlateParameterCases.json,
RunFunctionPlateParameterBindings.py and AuditFunctionPlateParameterBindings.py.
Both acceptance targets depend on the new GUI target. All thirteen registrations
retain 180-second timeouts and the engine resource lock. Use fresh output paths::

   cmake --build build/linux-gui --target GmatQtNasaFunctionPlateParameterBindingTests -j 6
   python3 src/qtgui/tests/RunFunctionPlateParameterBindings.py --output build/gui-acceptance/plate-parameter-replay
   python3 src/qtgui/tests/RunFunctionPlateParameterBindings.py --visible --output build/gui-acceptance/plate-parameter-visible-replay

Authoritative artifacts are in
build/nasa-coverage/coverage95-20261103-function-plate-parameter-bindings/:
evidence-audit-v1.json, plate-parameter-binding-matrix-v1.json,
remaining-parameter-binding-worklist-v1.json, evidence-inputs-v1.json,
stage-receipt-v1.json and seal-verification-v1.json. Failed/corrected executed
sources are separate; passing pilots and older unaffected evidence are reused.

Native bindings now cover **138/321 types**. **183 remain**: 169 Spacecraft and
14 SpacePoint owners. There are 179 names with any joined bounded context and
142 without; 41 older bounded names remain in the native binding queue. All 321
retain whole obligations. Continue rotation, derived thruster, attitude, frame,
time, orbit, vector/matrix/string and provider routes, plus the remaining Plate
domains above. Preserve strict Solver-clone/unmodeled-System/strict-time/custom-
Star probes, E491/E477/E406 and science/source conflicts. The MATLAB executable
path question is pending; no hold is waived. The full 95% goal remains active
and incomplete.
