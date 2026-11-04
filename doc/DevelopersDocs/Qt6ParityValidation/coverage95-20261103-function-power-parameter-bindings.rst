Nuclear power Parameter owner bindings — 2026-11-03
=================================================

Coverage remains **1,019/1,158 (88.0%)**, with 139 original requirements open
and **82 more needed** for 1,101. Historical operation coverage remains
1,816/2,367. No requirement credit is added by this bounded stage. All 59
prerequisite holds, original fingerprints, assertions and whole-row conjunctions
remain. Technical, scientific, current-build and full NASA gates remain false.

Three new native owner routes
-----------------------------

``TotalPowerAvailable``, ``RequiredBusPower`` and ``ThrustPowerAvailable`` now
have actual Parameter-to-Spacecraft references and distinct attached
NuclearPowerSystem identities in main, private function and explicit Global
scopes. Complete factory checks retain all 321 system types plus Array, String
and Variable; all 566 matrix rows and prior fields remain unchanged.

Three supported cases pass unattended and visibly: six GUI passes check 18
missions, 72 repeated/nested function calls and synchronous callbacks, 144
Parameter owner/attached power/direct field/Write/Report observations and 90
markers. The same owner names deliberately shadow the caller. Root and Global
owners match their sandbox objects; the function-private owner and attached
power are distinct clones. Private maps are inspected only while their owning
function is executing, never through expired pointers after return.

The independent numerical fixture uses nuclear generation
``P0 * 0.9 ** ((A1Epoch - 21545) / 365.25)``, constant bus load, and
``max(0, (1 - margin / 100) * (generation - bus))`` for thrust power. Root,
private and Global initial powers are 10/20/30, bus constants 2/4/20 and margins
10/20/25. Annual decay is 10 percent; BusCoeff2 and BusCoeff3 are zero.
Controlled epochs are 21545, ``21910.25 + Calls * 365.25`` and
``22275.5 + Calls * 365.25``. The actual owner epoch and every native input are
checked independently of the evaluated value, with absolute bound 1e-12.
There are 96 generation/thrust and 48 constant-bus formula checks, including
12 clamped-zero thrust-power observations. All 144 input sets retain nonzero
annual decay.

The native runtime, ParameterInfo cache and hardware property report read-only
flags. Existing empty Parameter unit strings are retained; this stage does not
qualify unit labels or actual Assignment destination/Report chooser behavior.
PowerSystem's internal spacecraft pointer has no public getter here. Distinct
epoch-dependent behavior supports the connection, but its pointer identity is
not directly observed. SolarPowerSystem, distance-dependent bus terms, eclipse,
electric-thruster providers and general time/force science remain open.

Retained absolute epoch failures
--------------------------------

All three original probes fail before the first function callback:
``GmatCommand::SetWrapperReferences`` cannot find ``PrivateOnly.A1ModJulian``
for the LHS-only statement
``PrivateOnly.A1ModJulian = 21910.25 + Calls * 365.25;``.
The exact failed source capsule, native logs, raw main callback, receipts and
three original registrations remain. No root-cause repair or qualification is
claimed. The original tests keep the label
``qt-gui-power-absolute-epoch-probe`` and their unchanged arguments.

The separate passing ``SelfReferencedEpoch`` controls read the current private
epoch before adding ``Calls * 365.25`` and the current Global epoch before adding
365.25. Fresh function-private initialization and Global persistence produce the
same strict expected epochs/formulas. These controls do not qualify the original
absolute writes. No original NASA assertion or numerical bound is weakened.
No compile failures or interruptions occurred; both builds and all native
handles are terminal.

GUI lifecycle and custody
-------------------------

Each case exercises actual function-file keyboard Cancel/Save, exact caller
source/history/configured-prototype isolation, Unicode save/reopen, explicit
Build followed by no-Build repeat, checked Write and Report bytes and the actual
report viewer. Visible inputs are identical before/after all three cases.
Three actual process maps and 51 native exposure events are recorded with
initially minimized, resizable, nonfullscreen software-rendered windows.
Screenshots, runtime-binary copies and kernel copies are zero. Ten deliberately
corrupt proof controls must be rejected by the offline auditor.

No production increment occurs. All 241 pending core identities match the
preceding sealed ThrustAngle stage; 233 are bound by the unattended receipt and
the remaining eight are additionally bound statically. Visible source custody
includes all 241. The existing technical review still applies; passing bounded
controls do not approve those engine changes.

Replay and evidence
-------------------

Reusable files are ``NasaFunctionPowerParameterBindingTests.cpp``,
``NasaFunctionPowerParameterCases.json``,
``RunFunctionPowerParameterBindings.py`` and
``AuditFunctionPowerParameterBindings.py`` under ``src/qtgui/tests``.
Both acceptance build targets depend on the new native target. The three
supported and three original probes retain 180-second timeouts and the engine
resource lock. From the repository root::

   cmake --build build/linux-gui --target GmatQtNasaFunctionPowerParameterBindingTests -j 6
   python3 src/qtgui/tests/RunFunctionPowerParameterBindings.py --output build/gui-acceptance/function-power-parameter-replay
   python3 src/qtgui/tests/RunFunctionPowerParameterBindings.py --visible --output build/gui-acceptance/function-power-parameter-visible-replay

Use a fresh output directory for every invocation. The original failed probe
is explicitly replayable, for example with
``--filter '^QtGui.NasaFunctionPowerParameterBindingTotalPowerAvailable$'``.
It is expected to fail at the recorded boundary until repaired.

Authoritative artifacts are in
``build/nasa-coverage/coverage95-20261103-function-power-parameter-bindings/``:
``evidence-audit-v1.json``, ``power-parameter-binding-matrix-v1.json``,
``remaining-parameter-binding-worklist-v1.json``, ``evidence-inputs-v1.json``,
``stage-receipt-v1.json`` and ``seal-verification-v1.json``. Exact executed
source capsules preserve the failed and passing source versions separately.
Earlier source-versioned passes are reused without replaying unaffected cases.

Native evidence now covers **125 of 321 types**; **196 remain**: 182 Spacecraft
and 14 SpacePoint owners. There are 166 names with any joined bounded context
and 155 without; 41 older bounded names remain in the native binding queue.
All 321 retain whole obligations. Continue rotation, derived thruster, plate,
attitude, frame, time, orbit, vector/matrix/string and provider routes. Retain
the new absolute epoch probes, earlier strict-clone/unmodeled-System/strict-time/
custom-Star probes, E491/E477/E406 and science/source conflicts. The MATLAB
installation-path question remains pending; no hold is waived. The full 95%
goal remains active and incomplete.
