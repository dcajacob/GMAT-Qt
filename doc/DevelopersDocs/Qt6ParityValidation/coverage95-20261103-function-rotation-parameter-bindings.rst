Nine hardware rotation Parameter routes — 2026-11-03
===================================================

Coverage remains **1,019/1,158 (88.0%)**, 139 original requirements open and
**82 more needed** for 1,101. Historical coverage stays 1,816/2,367. Original
fingerprints, assertions, whole-row conjunctions and all 59 prerequisite holds
remain. No requirement credit is added. All technical, scientific, full-current-
build and full NASA gates remain false.

Nine reads and an unresolved write contract
-------------------------------------------

All R_SB11 through R_SB33 types now have native Parameter-to-Spacecraft
references and actual attached ChemicalTank clone identities in main, private
function and explicit Global scopes. Every selected scalar distinguishes all
three scopes. Actual scalar getters and the complete native RotationMatrix
agree with independently authored rational matrices. The fixtures use Construct,
zero hardware offset, orthogonal integer directions, orthonormal matrices and
determinant +1, with absolute bound 1e-12.

The root/private/Global primary and secondary directions are respectively
[1,2,2]/[-2,-1,2], [6,22,3]/[-13,6,-18], and [7,26,2]/[-22,7,-14]. The matrices
are the following integer rows divided by 3, 23 and 27 respectively::

   Root:       [  2  -2   1;   2   1  -2;   1   2   2 ] / 3
   Private:    [-18   3  14;  13  -6  18;   6  22   3 ] / 23
   Global:     [-14   2  23;  22  -7  14;   7  26   2 ] / 27

The guide specifies normalized z from Direction, x from z cross SecondDirection,
and y from z cross x. No identity-orientation substitution is used. Actual native
Parameter references, distinct hardware clones, flags, literal input directions,
all nine matrix components through two getters, determinant and orthogonality
are checked synchronously while each function scope is alive.

All nine constructors and ParameterInfo/runtime objects still advertise
IsSettable=true. Hardware's R_SB fields are read-only, and SpacecraftData's
Parameter setter branches are behind disabled DEBUG_HW_SET_R_SB. Live read
callbacks record 432 idempotent native setter refusals with unchanged owner
matrix and reevaluated value. Raw cache state before reevaluation is not directly
observed. The native read tests record setter outcomes without requiring rejection;
they permit future supported idempotent setter success.

Separate strict WritableProbe cases require mission completion after idempotent
private and Global script assignments. **All nine fail in both modes** at the
first private assignment with ``SpacecraftData::SetReal() Not settable or unknown
item id``. Only the main callback has occurred; the subsequent Global script
assignment is not reached. Global native API refusals are observed separately
in the read callbacks. The strict completion assertion, original writable
metadata, registrations and failed statuses remain; no WILL_FAIL conversion,
read-only reclassification or role/factory exclusion is made. FRR!E554 describes
spacecraft direction-cosine components read/write without specifically naming
attached hardware R_SB; no row credit or requirement reinterpretation follows
from that ambiguity.

Current evidence and historical fixtures
----------------------------------------

**18 current read GUI passes** cover nine cases unattended and visibly:
54 missions, 216 calls/callbacks, 432 Parameter owner/attached tank/scalar/Write/
Report observations, 3,888 components from each full matrix getter, 2,592 input
direction components, 432 determinants and 270 markers. Existing function-file
keyboard Cancel/Save, caller/source/history/prototype isolation, Unicode reopen,
Build/no-Build repeat and actual report viewer checks remain. Private maps are
never inspected after the owning callback returns.

An earlier private matrix shared root scalar values at R_SB21 and R_SB31.
Its 18 read passes remain source-versioned historical evidence and are superseded
for the stronger per-leaf owner discrimination. A new private orientation and
explicit C++/auditor pairwise-distinct check affect all nine cases, so all nine
read/write pairs were rerun. The earlier strict failures also remain: **36 failed
strict write attempts total**, eighteen historical and eighteen current.
No failed write obligation is relabelled as a negative-control pass.

Current visible replay records eighteen actual process maps and 252 exposure
events: nine passing read processes/153 events and nine failed write processes/
99 events. The earlier eighteen maps are retained separately. Both visible source/
image inventories are stable before/after their executions. Windows start
minimized, are resizable/nonfullscreen and use software rendering. Screenshots,
runtime-binary copies and kernel copies are zero. Twelve corrupt-proof controls
check owner, clone, class, rational matrix/input/value, metadata, setter outcome,
reread/owner preservation, repeat and marker changes.

Three builds completed; there are no compile failures or native interruptions.
One intermediate offline fixture-edit helper stopped at an ambiguous C++ source
anchor; its frozen preimage, exact failed fragment and captured error remain.
The correction uses a unique main-setup anchor and rebuilds only after success.
An automatic approval review timed out before creating the second visible
process; the explicitly permitted single retry ran and completed. That pre-start
timeout is retained separately and is not a native interruption. All handles
are terminal.

All **242 pending core hashes are unchanged** from the sealed Assignment stage.
The native receipt binds 234 paths and eight additional non-C++ sources are
statically bound; visible custody includes all 242. There is no production
increment, metadata transition or general ABI approval.

Replay and remaining work
-------------------------

Reusable files under src/qtgui/tests are
NasaFunctionRotationParameterBindingTests.cpp,
NasaFunctionRotationParameterCases.json,
RunFunctionRotationParameterBindings.py and
AuditFunctionRotationParameterBindings.py. Both acceptance targets depend on the
new native target. All eighteen registrations retain 180-second timeouts and
the engine lock. WritableProbe cases additionally carry the
qt-gui-rotation-writable-probe label. From the repository root::

   cmake --build build/linux-gui --target GmatQtNasaFunctionRotationParameterBindingTests -j 6
   python3 src/qtgui/tests/RunFunctionRotationParameterBindings.py --output build/gui-acceptance/rotation-full-replay
   python3 src/qtgui/tests/RunFunctionRotationParameterBindings.py --visible --output build/gui-acceptance/rotation-full-visible-replay

Use fresh output paths. Default replay includes the strict write probes and
currently returns 1 after a completed selection. To replay only read cases, use
``--filter '^QtGui.NasaFunctionRotationParameterBindingR_SB[123][123]$'``;
this selection does not qualify or remove the failed write obligations.

Authoritative artifacts are in
build/nasa-coverage/coverage95-20261103-function-rotation-parameter-bindings/:
evidence-audit-v1.json, rotation-parameter-binding-matrix-v1.json,
remaining-parameter-binding-worklist-v1.json, evidence-inputs-v1.json,
stage-receipt-v1.json and seal-verification-v1.json. Current executed versions are
headless-v3 and visible-v2; earlier versions/capsules/contracts remain intact.

All 566 rows/321 original Parameter types and previous fields remain. Native
owner bindings now cover **147 types**; **174 remain** (160 Spacecraft and
14 SpacePoint). There are 188 names with any joined bounded context and 133
without; 41 older bounded names remain queued. All 321 retain whole obligations.
Supported R_SB script writes, arbitrary/degenerate orientations, refresh after
direction changes, translation, general frames/forces/mass-property transforms
and all earlier provider/science gaps remain open.

The user answered **No** to availability of a licensed MATLAB installation.
The availability question is resolved; 37 MATLAB holds remain unresolved and
no substitute or hold waiver is assumed. Continue independently executable
derived thruster/attitude/frame/time/orbit/provider work and other prerequisites,
retaining earlier failed probes, E491/E477/E406 and science/source conflicts.
The full 95% goal remains active and incomplete.
