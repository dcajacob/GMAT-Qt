Attached tank and thruster Parameter bindings in functions
========================================================

Canonical coverage remains **1,019/1,158 (88.0%)**, 139 open and **82 more
needed** for 1,101. Historical coverage remains 1,816/2,367. No original row or
whole E491 conjunction is newly qualified. The 95% goal remains active/incomplete.

New evidence
------------

Sixty-one additional scalar Parameter names have live main/private/Global owner
and hardware attachment observations in both modes: 23 ChemicalTank fields and
38 ChemicalThruster fields. Actual native Parameter GetRefObject points to the
live Spacecraft; SpacecraftData's source-backed dependency lookup and separately
observed attached objects establish the hardware route. Private hardware differs
from the caller's same-named owner shadow; Global hardware is the caller's actual
shared instance. Configured hardware prototypes remain distinct and unchanged.
No new Parameter-to-Hardware GetRefObject API is assumed.

The tank list covers fuel CM/MOI components in tank and body frames, Pressure,
Temperature, RefTemperature, Volume and FuelDensity. The thruster list covers
DutyCycle, ThrustScaleFactor, GravitationalAccel, C1-C16, K1-K16 and
ThrustDirection1-3. All 321 factory Parameter names, aliases, roles and original
metadata remain. The new catalog adds selected recipes without changing old rows.

Each case checks three missions: initial file-editor Cancel/Save, Unicode main
Save/reload/Build, then no-explicit-Build repeat. Synchronous propagation callbacks
inspect private objects only while alive; each mission includes main, repeated
private calls and nested use of a Global Spacecraft. Independent distinct literal
recipes and mutations check Parameter values, direct hardware fields, actual
Write blocks, raw Report values and caller/prototype/source/history/file isolation.
The actual Output-tree report viewer displays the checked bytes.

There are 122 selected GUI passes, 366 missions, 1,464 function calls/callback
samples, 2,928 Parameter-owner and attached-hardware identity checks, 2,928 direct
field checks, 2,928 actual Write values, 2,928 raw Report values and 1,830 markers.
Four corrected headless pilot passes are reused; only the other 57 cases run in
headless-v3. All 61 visible cases pass with 61 actual process maps, stable source/
image hashes, initially minimized/resizable/nonfullscreen software rendering and
zero screenshots/runtime-binary/kernel copies. Ten corrupt-proof controls reject
wrong owner/hardware aliases/types/values, broken tank geometry, missing no-Build
repeat and changed markers.

Production code is unchanged. Every one of the 238 pending core identities
matches the prior sealed Solver stage; no core path is added or incremented.
Technical/scientific/full-current-build/NASA approval gates remain false.

Preserved failures and boundaries
---------------------------------

Four failed pilot cases remain source-versioned. Two tank cases completed the
native observation/mission but an inherited root invariant queried a hardware
property directly on Spacecraft. The corrected invariant uses the already
validated actual attached object. Two thruster cases incorrectly inherited the
tank-only identity-rotation assumption. Hardware constructs a rotation from two
directions; FuelTank explicitly selects identity directions, whereas Thruster
keeps the Hardware defaults. Both owning sources and the exact failed driver are
retained. Tank identity remains required. Every hardware case retains zero-offset
and orthonormality checks; thruster orientation is recorded without claiming a
specific current-direction or dynamic refresh relation. No production repair,
original NASA assertion or numerical-bound change is concealed by those fixture
corrections. There are no compile failures or interrupted cases in this stage.

Tank BCS cases require the observed identity rotation and zero hardware offset.
Other CM axes are zero; MOI cases have zero CM, reducing the independent body-frame
formula to the authored tensor entry. These cases do not qualify rotated tanks,
translation/parallel-axis physics or the earlier unmodeled System mass-property
probes. Auxiliary raw R_SB reads do not qualify the nine R_SB Parameter types.
The thruster coefficients/directions/duty/scale/gravity are direct field routes;
thrust, Isp, mass flow, firing, geometry refresh, force/provider and propulsion
science remain separate obligations.

The generic headless wrapper bound 230 of 238 pending core paths directly; its
cpp/hpp filter omitted eight build/Fortran/h-only source paths. All 238 current
hashes match the prior preserved inventory. The visible source review explicitly
adds every core path and checks before/after identities. Historical headless
receipts are unchanged; no omitted-source historical claim is fabricated.

Updated matrix and next work
----------------------------

The full 566-row matrix preserves every prior row/field and adds only hardware
binding witnesses. Live native owner/reference evidence now covers 120 of 321
types; 201 remain: 185 Spacecraft, 14 SpacePoint and two ThrustAngle metadata
owners. There are 161 types with any joined bounded context and 160 with none;
41 older bounded names remain in the native queue. All 321 whole obligations
remain open. Removing the 61 newly witnessed names from this queue is not a
factory or readable-role exclusion.

Continue remaining hardware rotation, derived propulsion, power, plate, attitude,
frame/time/orbit/vector/matrix/string/provider and ThrustAngle routes. Preserve
strict external-clone, unmodeled System, strict-time/custom-Star probes, finite
function-readonly/frame gaps, E491/E477/E406, earlier science/source conflicts
and all 59 prerequisite holds. The licensed MATLAB path question remains pending.

Replay and custody
------------------

Build and run the new cases::

  cmake --build build/linux-gui --target GmatQtNasaFunctionHardwareParameterBindingTests -j 2
  python3 src/qtgui/tests/RunFunctionHardwareParameterBindings.py
  python3 src/qtgui/tests/RunFunctionHardwareParameterBindings.py --visible

Both acceptance targets depend on the new executable. The wrapper accepts a
filter/output path; replays create new receipts without replacing historical
passes. Authoritative artifacts under
build/nasa-coverage/coverage95-20261103-function-hardware-parameter-bindings/ are
hardware-parameter-binding-matrix-v1.json,
remaining-parameter-binding-worklist-v1.json, evidence-inputs-v1.json,
evidence-audit-v1.json, final-sources-v1.tar.gz, stage-receipt-v1.json and
seal-verification-v1.json. Only a passed seal verifies final custody. Replay::

  python3 src/qtgui/tests/AuditFunctionHardwareParameterBindings.py --stage build/nasa-coverage/coverage95-20261103-function-hardware-parameter-bindings --output /tmp/hardware-parameter-audit-replay.json
  python3 src/qtgui/tests/TrackCoreChanges.py --verify build/nasa-coverage/core-change-review/20261103-function-solver-parameter-bindings
