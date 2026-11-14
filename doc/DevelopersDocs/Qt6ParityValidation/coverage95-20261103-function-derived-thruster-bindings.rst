Derived thruster Parameter bindings and units — 2026-11-03
=========================================================

Coverage remains **1,019/1,158 (88.0%)**, 139 original requirements open and
**82 more needed** for 1,101. Historical coverage stays 1,816/2,367. Original
fingerprints, assertions, whole-row conjunctions and all 59 prerequisite holds
remain. No requirement credit is added. All technical, scientific, full-current-
build and full NASA gates remain false.

Active and stopped reads with actual attached hardware
------------------------------------------------------

ThrustMagnitude, Isp and MassFlowRate now have native Parameter-to-Spacecraft
references in main, private function and explicit Global scopes. Each callback
checks the actual attached ChemicalThruster and ChemicalTank clones, the
FiniteBurn-to-thruster reference and the thruster-to-tank reference. Private
objects differ from the caller root; Global references retain the root object.
Direct special methods on Thruster, independent formulas, Write and Report agree.
The three derived getters return zero after EndFiniteBurn, including Isp.

The fixture uses regulated tanks, gravity 10, direction [1,2,2], constant C1/K1
and zero higher polynomial coefficients. Main/private/Global force inputs start
at 100/200/300 N and Isp inputs at 300/400/500 s. Each function call increments
private force/Isp by 10/5 and Global force/Isp by 20/10. Duty factors are
0.5/0.75/1 and thrust scales 1.25/1.5/0.8 respectively. The independent active
recipes are C1*duty*scale, K1 and -C1*duty/(10*K1). Thrust scale affects thrust
and does not affect mass flow, matching the owning implementation and guide.
Each selected active/stopped callback occurs after one/two seconds of firing.

Separate private and Global propagators avoid resetting either spacecraft's
clock. Fuel and total mass are checked with absolute bound 1e-7; elapsed seconds
with 3e-6; derived values with 1e-12. Repeated and nested calls retain cumulative
Global fuel consumption and clock progress. A separate callback after each
private coast compares the exact completed-main caller state, epoch, hardware
identities and fuel/engine values. All six Cartesian components are preserved
exactly across function calls; no Cartesian numerical science oracle is claimed.
Private scope maps are accessed only during synchronous live callbacks.

Three unit labels repaired; write gaps retained
----------------------------------------------

The first three native probes failed because the constructors advertised empty
unit strings. Those failures and exact executed sources remain frozen. Only the
three constructor unit literals in HardwareParameters.cpp change: ThrustMagnitude
to N, Isp to s, MassFlowRate to kg/s. There is no numerical conversion, formula,
setter, writable flag, owner/role/factory change, new member or virtual slot.
The guide supplies these dimensions; repaired factory and live objects agree.

All three historical/runtime Parameter writable flags remain true. Active
idempotent native setter attempts currently refuse with SpacecraftData's unknown
item-id error. The native read cases record outcomes without requiring refusal;
the offline audit verifies the outcome actually recorded by this build. No
script setter contract is qualified. The guide marks ThrustMagnitude/Isp writable
and MassFlowRate nonwritable; those discrepancies remain explicit. This stage
does not silently reclassify any field or translate failed writes into credit.

Evidence and custody
--------------------

**Six GUI passes**, three unattended and three visible, cover 18 missions,
72 function calls, 144 selected callbacks and 72 caller-root control callbacks.
They check 288 owner/engine/tank/burn identities, direct values, unit strings,
fuel budgets, total masses, clocks and values per Write/Report channel, with
144 nonzero active and 144 stopped-zero values, 144 recorded active native setter
refusals and 90 markers. Actual scripts execute 288 Begin/EndFiniteBurn commands
and 432 propagations. Existing keyboard file Cancel/Save, exact caller/source/
history/prototype isolation, Unicode reopen, Build/no-Build repeat and actual
report viewer checks remain.

The visible replay has three actual process maps and 51 exposure events. All
source/image inputs are unchanged before/after the run. Windows start minimized,
remain resizable/nonfullscreen and use software rendering. Screenshots,
runtime-binary copies and kernel copies are zero. Fifteen corrupt-proof controls
check owner/hardware identity, active/stopped values, units, duty/scale, cumulative
Global fuel, fuel mass, clocks, exact caller preservation, repeat and markers.

One test-only compile failure is retained: inherited string field access is
hidden by Thruster overloads. The correction uses the base pointer for generic
field/reference access and the derived pointer for the three special getters.
The corrected fixture builds before the unit probes; the production repair then
builds and passes both modes. There are three retained GUI unit failures and
no native interruptions. All build and native handles are terminal.

The pending core inventory is **243 paths**: all prior 242 hashes unchanged,
plus the newly pending HardwareParameters.cpp with its three unit strings.
The native receipt binds 235 paths; eight additional non-C++ sources are
statically bound. Visible custody includes all 243. Exact preimage, HEAD,
working file, patch and source-version capsules remain available. This bounded
unit repair does not grant technical/scientific or general ABI approval.

Replay and remaining obligations
--------------------------------

Reusable files under src/qtgui/tests are NasaFunctionDerivedThrusterBindingTests.cpp,
NasaFunctionDerivedThrusterCases.json, RunFunctionDerivedThrusterBindings.py and
AuditFunctionDerivedThrusterBindings.py. Both acceptance targets depend on the
native target. All three registrations retain 180-second timeouts and the
engine lock. From the repository root, with fresh output paths::

   cmake --build build/linux-gui --target GmatQtNasaFunctionDerivedThrusterBindingTests -j 6
   python3 src/qtgui/tests/RunFunctionDerivedThrusterBindings.py --output build/gui-acceptance/derived-thruster-replay
   python3 src/qtgui/tests/RunFunctionDerivedThrusterBindings.py --visible --output build/gui-acceptance/derived-thruster-visible-replay

Authoritative artifacts are in
build/nasa-coverage/coverage95-20261103-function-derived-thruster-bindings/:
evidence-audit-v1.json, derived-thruster-parameter-binding-matrix-v1.json,
remaining-parameter-binding-worklist-v1.json, evidence-inputs-v1.json,
stage-receipt-v1.json and seal-verification-v1.json. Current passing native
versions are headless-v2 and visible-v1; failed headless-v1 stays source-bound.

All 566 rows, 321 original Parameter types and prior fields remain. Native owner
evidence now covers **150 types**; **171 remain** (157 Spacecraft, 14 SpacePoint).
There are 191 names with any joined bounded context and 130 without; 41 older
bounded names remain queued. All 321 whole obligations remain open.

Stored thruster coordinate-frame pointer identity and raw cached values before
reevaluation are not directly observed. Arbitrary pressure/temperature polynomials,
unregulated tanks, propulsion/force/frame/time science, electric providers and
script setter support remain open. Retain all R_SB strict write failures and
earlier provider/science gaps, E491/E477/E406 and source conflicts. The user
confirmed no licensed MATLAB installation is available; the question is resolved
and all 37 MATLAB holds remain, without substitute or waiver. Continue independent
attitude/frame/time/orbit/provider work and other prerequisites. The full 95% goal
remains active and incomplete.
