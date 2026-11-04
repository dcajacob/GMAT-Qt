Live finite-burn lifecycle, hardware binding and native units
============================================================

Coverage remains **1,019/1,158 (88.0%)**, 139 open and 82 more needed for 1,101.
Historical coverage remains 1,816/2,367. Original authorities, denominators,
59 prerequisite hold identities and all qualification gates remain. No exact
whole row or FRR!E491 conjunction is newly qualified.

All seven FiniteBurn parameters (TotalMassFlowRate, TotalAcceleration1/2/3 and
TotalThrust1/2/3) now have live owner, active/stopped value, attached hardware and
corrected Unit-string witnesses in both modes. With prior evidence, 57 of all
321 types have live native bindings; 264 still need them. There are 98 names
with any bounded context and 223 with none; 41 older bounded names remain in the
264-entry queue. Every prior field of all 566 factory-role rows remains intact.
All 321 retain broader owner/dependency/domain/science/whole-scope obligations.

Actual executed scope and hardware
----------------------------------

A test-owned synchronous Publisher subscriber observes main/private/Global
FiniteBurn instances while function objects are alive. Actual Parameter native
class/full name/return/owner metadata and GetRefObject owner identity match the
live scoped burn. Configured prototypes, root/private instances and the shared
caller owner are distinguished. Main global lookup is explicitly a value map
snapshot. Nullable GetOwner bookkeeping and varying Parameter local flags remain
observations. No expired private store or former function spacecraft is read
after return.

Each context ignites separate private/root and shared burns on the same target,
primes their force caches, observes a powered initial publication, stops both
burns, and observes a coast initial publication. Two samples per context check
nonzero firing values and stopped zeros. The global shared burn must resolve the
current target's actual attached thruster through its native thruster map; the
thruster's actual tank array must equal that same target's tank. This graph is
rechecked across direct/direct/nested calls and all three missions. Only live
callback reads establish it; global burn spacecraft pointers are not dereferenced
after their former function target expires.

Root/private/shared thrust magnitudes start at 100/200/300 N. Function attached
C1 values change by 10*Calls and 20*Calls. All other C coefficients are zero;
K1=300, other K coefficients zero, gravitational acceleration=10, duty/scale=1
and inertial direction=[1,2,2], norm=3. Six force/acceleration cases disable mass
depletion and use 2,000/1,000 kg main/private target masses. The mass-flow case
enables depletion and checks -F/(300*10), separate tank budgets and total mass
against measured native elapsed time. It has 48 selected nonzero flow checks.
Scalar force/acceleration/flow bounds are 1e-12, mass budget bound 1e-7 kg; the
observation domain permits 3e-6 seconds cumulative elapsed error. No old bound
is weakened, and the separate strict fixed-30-second failure stays open.

The Parameter internal frame pointer equals the Moderator's actual
InternalEarthMJ2000Eq. The attached thruster's authored CoordinateSystem string
and actual inertial direction are checked. Its stored coordinate-frame pointer
is not directly exposed or observed; no complete hardware-frame pointer graph
is inferred. Variable-mass acceleration in the flow case is recorded, not used
as an independent acceleration oracle. General propulsion, provider, integration,
physical timing and science qualification remain open.

Each case retains actual keyboard file Cancel then Save, exact caller/prototypes/
source/history/files, private caller Anchor isolation, Unicode reopening, a final
mission without explicit Build and actual report viewing. Every process checks
all 321 factory names plus Array/String/Variable. Actual scalar Write and Report
bytes independently match the live native samples.

Demonstrated unit-label repair
------------------------------

The first seven GUI cases passed the value/hardware contract. Stronger native
unit probes then failed all seven: their factory Unit strings were all Km/s.
The exact first passes, failed unit probes and executed sources remain separate.
The calculation-parameter reference specifies Kg/s, Km/s with superscript2 and
Newtons. The final native labels use standard kg/s, km/s^2 and N.

The only production increment is three constructor literals in
src/base/parameter/BurnParameters.cpp, covering the seven types. No numerical
conversion, formula, member, virtual slot, owner/dependency, setter flag or
eligibility changes. Unit API consumers now receive the corrected labels;
no plotted-label rendering claim is made. Exact HEAD preimage and
core-v3-unit-labels.patch remain. Factory and actual main/private/Global Parameter
units are asserted, including copied/initialized function instances.

The original catalog/constructors retain settable=true while the guide marks
these seven parameters N. That read-only/writeability discrepancy remains
unqualified. No unsupported setter is declared acceptable and no parameter is
excluded. Original metadata fields remain at their historical source version;
additive unit-transition fields describe the new display strings.

The schema-2 core inventory is
build/nasa-coverage/core-change-review/20261103-function-finite-burn-bindings/:
238 pending paths (225 tracked differences, 13 untracked). All previous 237
working hashes remain unchanged; BurnParameters.cpp is the sole new pending
path. TrackCoreChanges.py adds its review description. Technical review remains
open and no earlier core increment is implicitly approved.

Results and custody
-------------------

Final selection has **14 GUI passes**: headless-v3 and visible-v1, seven each.
The seven value-only v1 passes remain superseded for unit qualification; all
seven v2 unit failures remain failed. There are no compile failures or weakened
original assertions. All build/native handles are terminal.

Final scope checks 42 missions, 168 GmatFunction calls, 336 live callbacks
(252 inside functions), 168 active/168 stopped samples, 672 Parameter/owner
checks, 672 direct burn-value checks, 672 typed Writes, 672 Report values,
672 Begin/End commands, 504 propagations, 14 unmanaged factory-unit checks,
672 live unit checks, 672 burn/thruster and 672 thruster/tank identities and
210 marker scalars. Numerical counts do not qualify integration or time science.

Visible-v1 has seven actual mapped processes and stable source/image hashes,
initially minimized/resizable/nonfullscreen windows and zero screenshots.
Native exposure count is in the audit. Software GL is selected; no human or
physical GPU claim. No runtime-image or kernel bytes are copied. Ten corruptions
are rejected: configured/caller owner, native unit, thruster, tank, mass flow,
stopped value, stopped engine firing, target mass and omitted no-Build repeat.
The offline audit passes on its first invocation. All scientific/technical/
full-current-build/full-NASA gates remain false.

Authoritative files are in
build/nasa-coverage/coverage95-20261103-function-finite-burn-bindings/:
evidence-audit-v1.json, evidence-inputs-v1.json,
finite-burn-binding-matrix-v1.json,
remaining-parameter-binding-worklist-v1.json, fixture-scope-review-v1.json,
core-identity-continuity-v3.json, final-source-identities-v1.json,
stage-receipt-v1.json and seal-verification-v1.json. Earlier source/binary
identities remain attached to their original witnesses; source capsules are text.

Replay and continuation
-----------------------

Replay only new/affected work or an explicit watch request::

  cmake --build build/linux-gui --target GmatQtNasaFunctionFiniteBurnBindingTests -j4
  python3 src/qtgui/tests/RunFunctionFiniteBurnBindings.py --filter '^QtGui.NasaFunctionFiniteBurnBindingTotalMassFlowRate$'
  python3 src/qtgui/tests/RunFunctionFiniteBurnBindings.py --visible --filter '^QtGui.NasaFunctionFiniteBurnBindingTotalAcceleration2$'
  python3 src/qtgui/tests/AuditFunctionFiniteBurnBindings.py --stage build/nasa-coverage/coverage95-20261103-function-finite-burn-bindings --output /tmp/function-finite-burn-audit.json

The wrapper defaults to all seven ordinary native-X CTest cases, with software
GL, engine lock and both acceptance target dependencies. Continue the complete
264-entry live-binding queue: 246 Spacecraft, 14 SpacePoint, two Solver and two
UnknownObject metadata owners (ThrustAngle routes remain explicit). Preserve
all 57 bounded types' remaining obligations, finite read-only/frame/force gaps,
nine unmodeled System probes, strict-time/custom-Star failures, E491 conjunctions,
E477/E406, earlier science/source conflicts and all 59 holds. The full 95% goal
remains active and incomplete.
