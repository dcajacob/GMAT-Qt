Function-declared propagator/frame scope and retained strict timing gap
=====================================================================

Coverage remains **1,019/1,158 (88.0%)**, with 139 open and **82 more needed**.
Historical operations remain 1,816/2,367 (76.7%). No exact requirement is newly
admitted. Original authorities, fingerprints, assertions, whole-row conjunctions,
denominators, all 59 prerequisite holds and separate approval gates remain.

Actual declarations and use
--------------------------

The new driver declares FunctionOwned only in its function file. PropSetup uses
a function-declared PrinceDormand78 integrator and Earth point-mass force model;
the caller uses a separately configured RungeKutta89 setup. The frame case uses
a function-declared MJ2000Ec coordinate system and a main MJ2000Eq system.
Actual propagation or nonidentity frame conversion executes through direct,
direct and nested function calls. No Global declaration names these resources.

The actual function global store must refer to the caller's runtime Shared
resource, distinct from the configured prototype. Native function declaration
templates are recorded separately from the Propagate command's executed owned
clone. The latter retains local=true despite nominal automatic/global flags;
flags alone do not establish shared ownership or general eligibility exceptions.
No expired function-local object store is accessed. No runtime Create command
is invented for the declaration-time parser route.

The final selection has **eight GUI passes**: four new scope passes and four
passes of unchanged original frame/automatic-propagator regressions. New scope
checks 12 missions, 48 GmatFunction calls, 36 reader/main-global/declaration
contexts, 18 executed propagator configurations and duration observations,
24 actual propagations, 348 raw scalars (288 state components and 60 markers),
72 separate caller anchor components and 84 Script constructor lines.

The independent state calculations use synthetic Mu=400000 point-mass circles
or a fixed 84381.448-arcsecond obliquity rotation. Position/velocity bounds remain
1e-7 km/1e-10 km/s for propagation and 1e-9 per frame component. Markers remain
exact. A private Variable shadows a same-name caller Spacecraft without changing
its mass, epoch or Cartesian state. Actual keyboard file Cancel/Save, exact
source/history/prototypes, Unicode reload, a third run without explicit Build
and actual report viewing remain checked. Qualification of other methods, axes,
providers and physical truth remains open.

Strict endpoint failure remains open
-----------------------------------

The original new PropSetup probe compares state with an exact 30-second circle.
It fails: the executed integrator and force model report
30.0000002627157 seconds, producing about 1.985e-6 km (1.985 mm) of position
error against that fixed endpoint. A separate 84.375-second binary-day interval
experiment retains the same offset and does not explain or resolve it. Returning
to 30 seconds with StopTolerance=1e-12 also fails the fixed state bounds.

The strict probe remains registered as QtGui.NasaFunctionOwnedPropSetup with
its original fixed-30-second oracle and numerical bounds. Its tighter stopping
tolerance is retained. No production timing or integration code is changed.
This result is not promoted to a passing strict-timing contract.

PropSetupStoppedTime is an additional, explicitly separate scope witness. It
compares state with an independently calculated circle at the actual executed
integrator time and separately requires abs(time-30)<=1e-6 seconds, matching the
current Propagate constructor's default timeAccuracy. The default generic
StopTolerance property is 1e-7; it is distinct from that default timeAccuracy.
Integrator and force elapsed times must agree. Existing state bounds are kept;
the headless residual at the measured endpoint is at most 3.64e-12 km in the
position components. This added witness neither repairs nor qualifies the strict
probe, and assigns no requirement credit. The default runner selects the added
case and frame case; the strict probe remains available by explicit filter.

Retained failures, custody and validation
----------------------------------------

All **four failed native CTest attempts** remain with exact sources. Three fail
the fixed-endpoint oracle (30/default, 84.375/default and 30/tighter tolerance).
One diagnostic attempt crashes after mission completion by querying
StopCondition::GetStopDifference after its function-local parameter expired.
The source-bound gdb replay retains that crash and backtrace; it is a test
diagnostic lifetime error, not an asserted engine mission crash. The final driver
removes that unsafe observation and retains safe command-owned elapsed-time reads.

One compile failure remains: a duplicate number() helper conflicted with the
existing FormationAcceptance helper. The driver now uses the existing helper.
The early headless frame pass is retained as superseded by the final proof
schema. Both original headless regression passes are reused; new/affected cases
alone repeat. No original assertion, numerical bound or authority is weakened.

Visible v1 passes all four selected cases with four actual mapped processes,
stable before/after source/runtime inputs, initially minimized/resizable/
nonfullscreen windows and zero screenshots. Seven corrupt-proof controls reject
configured-prototype substitution, wrong frame/method, wrong raw state, an
endpoint outside the duration bound, false strict-time promotion and an omitted
repeat. Every build/native/debug handle is terminal. No ELF, kernel or runtime
binary bytes are copied into evidence/source capsules. Process identity does not
establish physical GPU or human observation.

There are **no production changes** in this stage. All 236 prior pending core
identities remain unchanged (223 tracked differences, 13 untracked files),
verified against the sealed body-scope inventory. Technical/scientific/current
build/full-NASA approval gates remain false.

Replay and next work
--------------------

Authoritative stage:
build/nasa-coverage/coverage95-20261103-function-owned-scope/.
Use evidence-audit-v1.json, evidence-inputs-v1.json, stage-receipt-v1.json,
seal-verification-v1.json, owned-scope-matrix-v1.json, native-contract-v1/v2.json,
all executed/final source capsules and the retained gdb-v1 artifacts.

All prior fields of all 566 factory rows remain. PropSetup/CoordinateSystem gain
two bounded creation/use contexts each; PrinceDormand78, RungeKutta89, MJ2000Eq
and MJ2000Ec gain their selected owned-component contexts. All 14 propagator and
22 axes entries remain. Other methods/axes are not qualified by these witnesses.
The strict time failure remains attached to PropSetup without an exclusion.

Next search retained owner/parameter contexts for the 280 unjoined system
parameters and remaining owned-method/axis routes. Keep all E491 conjunctions,
already-counted child scope, E477/SaveMission, E406/terminal output, earlier
source/science conflicts, custom-Star failure and all 59 holds. The full 95%
goal remains active and incomplete.

::

  cmake --build build/linux-gui --target GmatQtNasaFunctionOwnedScopeTests -j4
  python3 src/qtgui/tests/RunFunctionOwnedScope.py
  python3 src/qtgui/tests/RunFunctionOwnedScope.py --visible
  python3 src/qtgui/tests/RunFunctionOwnedScope.py --filter '^QtGui.NasaFunctionOwnedPropSetup$'
  python3 src/qtgui/tests/AuditFunctionOwnedScope.py --stage build/nasa-coverage/coverage95-20261103-function-owned-scope --output /tmp/function-owned-scope-audit.json
  python3 src/qtgui/tests/TrackCoreChanges.py --verify build/nasa-coverage/core-change-review/20261103-function-body-scope

Both acceptance build targets include the driver. Native X launches require
sandbox escalation. Replay only new or affected cases; the explicit strict
probe is a retained known failure.
