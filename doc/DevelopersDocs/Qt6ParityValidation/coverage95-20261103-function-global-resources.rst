Global resource identity and nested returns — 2026-11-03
========================================================

Coverage remains **1,019/1,158 (88.0%)**, 139 open and **82 more needed**.
Historical operations remain 1,816/2,367 (76.7%). No exact requirement is
newly admitted. All 59 prerequisite holds and separate technical, scientific,
full-current-build and full-NASA gates remain unchanged and open.

New bounded scope
-----------------

Twenty-eight existing resource witnesses now declare the same resource Global
in the main mission and direct/nested functions: ChemicalTank, ChemicalThruster,
Transmitter, Transponder, GroundStation, ImpulsiveBurn, DifferentialCorrector,
BatchEstimator, ExtendedKalmanFilter, Simulator, Yukon, ElectricTank,
ElectricThruster, SolarPowerSystem, NuclearPowerSystem, ConicalFOV, RectangularFOV,
ErrorModel, ProcessNoiseModel, EstimatedParameter, OrbitView, ReportFile, Plate,
FirstOrderGaussMarkov, Receiver, Antenna, Formation and FiniteBurn.
Original catalog bytes, seeds, field kinds and dependency specifications are
unchanged. Barycenter/LibrationPoint remain in the complete role table and
retained automatic-global evidence; they are not discarded from the universe.

Each resource window performs actual function-file keyboard Cancel/Save,
main/direct/direct/nested property Write, three missions (initial, Unicode
reopen and repeat without explicit Build), typed field/decoy checks, independent
literal markers, caller continuation, source/history/prototype isolation,
actual function-global-store pointer identity and actual report viewing.
A function's output wrappers are deleted on return, so the test inspects the
live global store rather than dereferencing a stale output wrapper. Global
whole-object output is an additional supported witness here, not a requirement
imposed on every other resource class. The literal marker does not claim an
unsupported generic object-property Assignment RHS; actual field use is pinned
by the four Write outputs and typed native getters in each phase.

A separate Array case checks a nested scalar result into a global Array element,
a nested global whole-Array return, a root whole-Array result, independent exact
component values, local/caller/decoy/prototype isolation, Unicode reopen,
no-explicit-Build repeat and actual report viewer. It also checks the existing
attached-tank Write output remains intact. No force, navigation-provider,
process-noise or conversion physics is inferred from these scope operations.

Final selected evidence: **58 new GUI passes**, 12 affected original regression
passes, 174 new missions, 696 function calls, 966 raw report scalars, 672 property
Write literals and 516 actual function-global-store identity checks. The six
regressions preserve their prior assertion-source hashes: TotalMass read-only,
isolated function validation, burn-object loop impulse, recursive Array frames,
direct recursion and mutual recursion. The nine first-function output-parser
support cases also pass; they are separate from GUI/mission totals.

Demonstrated repairs and failures
--------------------------------

Six production paths change, with exact preimages and core-v11.patch retained:

* Validator.cpp obtains declared parameter-owner metadata from an unmanaged
  factory prototype when the isolated parser has not yet populated the owner
  cache. It then preserves the existing direct-field/foreign-owner distinction;
  matching-owner read-only parameters retain their original route. Parent GUI
  metadata was already warm; no cold-parent claim is made.
* FunctionManager.cpp resolves nested callee outputs and caller destinations
  local-first/global-second, borrows global identity across local-frame restore,
  avoids same-object Copy and resolves a global indexed-Array owner through the
  same scope lookup. Existing local cloning and scalar/shape/wrapper guards stay.
* ObjectManagedFunction.cpp/.hpp expose the existing global-store pointer through
  a const-map, nonvirtual diagnostic getter. This is a new public C++ API,
  requiring technical review; it adds no data member or virtual slot.
* UserDefinedFunction.cpp skips sandbox-borrowed identities before dereferencing
  template-map entries during cleanup. The diagnostic records the identical
  Shared.Element1 pointer in function/automatic/sandbox lists; the sandbox had
  already deleted that borrowed parameter before function destruction.
* ObjectPropertyWrapper.cpp serializes an object-array property's ordered
  reference-name list when it does not select an attached object. The existing
  attached-object serialization branch is preserved and checked with FuelMass=23.

Eleven failed CTest attempts remain: two unsupported new RHS fixtures, two
uncached-owner parser failures, one missing nested global output, one reopen
crash, two blank list-output failures, two diagnostic crash reproductions and
one offscreen-registration failure before the new Array mission could start.
The Array registration was corrected to native X. The early RHS fixtures were
corrected to actual supported Write operations without reducing any original
assertion or numerical bound. One compile failure and two setup-only invocations
(unbuilt executables) are retained separately and count as no native passes.
A separate debugger crash trace remains diagnostic evidence. Early failed
receipts v2–v5 did not explicitly bind Validator.cpp; they are failure diagnostics,
not qualification witnesses. Final repaired witnesses bind the relevant sources.
Temporary ownership instrumentation is archived and absent from final source.

The first full visible invocation passed 35 cases with 35 exact process maps.
Simulator's consumed shipped navigation script was missing an explicit input
binding, so only Simulator repeated unattended and visibly with that binding.
Its original two passes and six earlier regression passes are retained as
superseded; final selection remains 70 passes. The additional visible invocation
has one mapped process. Both have unchanged before/after inputs, initially
minimized, resizable, non-fullscreen windows and zero retained screenshots.
Loaded-image hashes establish process/provider identity, not physical GPU or
human observation. No runtime ELF or kernel copies are in the evidence bundle.

Qualification and custody
-------------------------

The full 566-row role table remains intact in global-scope-matrix-v1.json;
29 rows gain the new bounded contexts (28 resources and Array), and every
whole-scope qualification flag remains false. The finite complete eligible-global
universe still needs source-backed resolution, including owned components,
alias spellings and automatic-global exceptions. Whole E491/E493/E497 stays
open; E493 does not require whole-object output for every resource. E477,
E406, scientific/source conflicts and every prior hold remain unchanged.

Core inventory is now **162 pending paths**: 156 of the previous 158 hashes are
unchanged, Validator/FunctionManager have explicit increments, and four paths
are newly pending. No numerical model/formula or object-layout change occurs
in this six-path increment. Technical and scientific approvals remain false.

Authoritative stage: build/nasa-coverage/coverage95-20261103-function-global-resources/.
Reuse evidence-audit-v1.json, evidence-inputs-v1.json, stage-receipt-v1.json,
seal-verification-v1.json and their exact source capsules. Five negative controls
reject wrong identity, wrong field, missing nested identity, missing repeat and
lost global Array-element output. The canonical cumulative ledger remains
build/nasa-coverage/coverage95-20261103-default-orientation/coverage-snapshot-v1.json.
All handles are terminal at this checkpoint. The full goal remains active.

Replay only new or affected work
-------------------------------

::

  cmake --build build/linux-gui --target GmatQtNasaFunctionGlobalResourceTests GmatQtNasaFunctionGlobalArrayTests -j4
  python3 src/qtgui/tests/RunFunctionGlobalResources.py
  python3 src/qtgui/tests/RunFunctionGlobalResources.py --visible
  python3 src/qtgui/tests/AuditFunctionGlobalResources.py --stage build/nasa-coverage/coverage95-20261103-function-global-resources --output /tmp/function-global-audit.json

The runner accepts --filter and --output for focused continuations. All 29 new
registrations are included in both acceptance build targets. Native/visible
launches require X-socket sandbox escalation. Reuse successful cases at their
recorded versions; do not replay this family merely to recover context.
