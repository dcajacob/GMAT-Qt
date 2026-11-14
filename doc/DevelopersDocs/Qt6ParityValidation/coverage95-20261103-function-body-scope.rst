Four user-body declarations and existing Sun function scope — 2026-11-03
=======================================================================

Coverage remains **1,019/1,158 (88.0%)**, with 139 open and **82 more needed**.
Historical operations remain 1,816/2,367 (76.7%). No exact requirement is newly
admitted. Original authorities, fingerprints, denominators, assertions, whole-row
conjunctions, all 59 holds and separate approval gates remain unchanged.

Authority and supported operations
----------------------------------

The original NASA creation list is FRR!E213–E217: Moon, Asteroid, Comet and
Planet. The current CelestialBody guide lists the same four custom classes and
states that these resources cannot be modified in the Mission Sequence. Mu is
documented with set access; it is not a registered calculation parameter for
arithmetic expressions. The current validator rejects those equation reads.

All five advertised celestial factory classes remain in the 566-row matrix.
The initial custom-Star probe fails before script settings can supply its empty
central body. That failure remains unqualified; it is neither repaired nor
converted to an eligibility exclusion. The existing Sun has a separate Star-class
scope case, with no custom-Star creation claim. The Star probe remains registered
and available through an explicit filter. The runner defaults to the four NASA
creation classes and the separate Sun case. No fifth custom class is added to the
product or inferred from factory membership, and no original requirement is waived.

The earlier handoff's proposed five-class creation obligation was an inference,
not the four-class creation list in the original authority. This stage preserves
the original failed probe and adds the correct supported contexts without changing
old rows, their fingerprints or any qualification flag. Whole E491 remains open.

Actual body declarations and model sharing
------------------------------------------

Each custom case declares MainBody in the caller with Mu=63 and radius=470,
and FunctionBody only in its function file with Mu=73 and radius=480. Native
Script-style Writes execute in main and direct/direct/nested function calls.
Actual serialized type/name/Mu values and independent native properties must
match those literals. The Sun case uses the existing model with authored Mu/radius
settings in the caller; it creates no Star in either script.

The configured solar system stays separate from the runtime model. A live caller
Spacecraft supplies its actual runtime SolarSystem pointer. Each actual function
must use that same model and resolve the same uniquely owned body through
FindFunctionObject. Custom FunctionBody is absent from the caller script and
configured model, and present in the actual runtime model. These checks avoid
dereferencing a possibly expired function-local object store.

Function resources are created while parsing their declarations: the current
ScriptInterpreter explicitly uses the 2014 script-mode construction design.
The final tests record actual FCS commands and check function declaration lookup
and ownership, without inventing a mandatory runtime Create command. The observed
FCS has no Create commands. This retains the actual engine construction route.

A private Variable named PrivateOnly coexists with a same-name caller Spacecraft.
Independent marker/counter values 42/43/44, caller Keep=103/104, all six caller
Cartesian components, epoch and mass remain checked. Body Global declarations
are absent; only the explicit shared Variable counter uses Global. File editing
uses actual keyboard Cancel/Save, with exact source/history/prototype/file
isolation, Unicode reopen, a third run without explicit Build and actual report
viewing. No unsupported mission-time body mutation or equation read is imposed.

Final selection has **14 GUI passes**: ten new scope passes and four passes of
the unchanged automatic-solar-model and Sun script-preview regressions. New scope
checks 30 missions, 120 GmatFunction calls, 90 reader/model/body lookup identities,
24 custom-body missions, six existing-Sun missions, 168 actual body constructor
lines, 192 actual Mu Writes, 150 raw markers and 180 caller state components.
These counts describe operations and observations, not a number of allocations.

Read-only diagnostic API and source custody
------------------------------------------

Function.cpp/.hpp add GetSolarSystem() const, returning the existing pointer as
const SolarSystem*. This is a nonvirtual diagnostic accessor, with no new member,
virtual slot, ownership transfer, execution behavior or numerical formula change.
It lets native evidence compare the actual function model with the live caller
model. It is a public API addition requiring technical review, not an approval.
Exact two-file preimages and core-v4-diagnostics.patch remain.

The schema-2 core inventory is **236 pending paths**, 223 tracked differences and
13 untracked files. All previous 234 hashes are unchanged; Function.cpp/.hpp are
newly pending. The inventory compares this worktree with HEAD and does not assert
exclusive authorship, release equivalence or technical/scientific qualification.

The four custom cases reference the existing Ceres SPK only as a configuration
fixture. Its path/hash are recorded and verified before and after runs. No kernel
is copied into source/evidence capsules. There is no ephemeris query, propagation,
physical-body equivalence or provider accuracy claim. Sun retains its configured
provider; the kernel identity check does not turn that case into Ceres execution.

Retained failures and final validation
-------------------------------------

All **nine failed native CTest attempts** remain. The first five include four
unsupported Mu-expression fixtures and the independent custom-Star creation
failure. The next four complete their missions but fail the test's incorrect
runtime-Create expectation. Exact source versions and partial output remain.
The supported Script Write/native-literal and declaration-identity contracts
replace those fixture assumptions using owning source evidence. No original test,
authority or numerical bound is weakened.

One compile failure remains with exact source: CelestialBody hides label overloads,
so the driver now uses its public parameter-ID accessors. One early Sun pass is
superseded by the final explicit declaration-lookup identity contract. The two
original regressions from headless v2 are reused; only new cases repeat to v3.
No production creation or parser policy is changed to make a test pass.

Visible v1 passes all seven selected cases with seven actual mapped processes,
unchanged before/after source/runtime inputs, initially minimized/resizable/
nonfullscreen windows and zero screenshots or runtime/kernel copies. Every
build/native handle is terminal. Process identity does not prove physical GPU or
human observation. Six corrupt-proof controls reject wrong body lookup, configured
model substitution, configured-body leakage, omitted repeat, wrong body literal
and false promotion of Sun use to custom-Star creation.

Two offline auditor authoring failures remain with exact source/logs: the older
frame regression emits its proof in the native log without a separate proof.json,
and a broad substring check matched FunctionBodyScope in a temporary directory.
The final auditor handles that original proof schema and checks actual declaration
syntax. Native runs are not repeated for those offline corrections.

Replay and remaining requirements
--------------------------------

Authoritative stage:
build/nasa-coverage/coverage95-20261103-function-body-scope/.
Use evidence-audit-v1.json, evidence-inputs-v1.json, stage-receipt-v1.json,
seal-verification-v1.json, body-scope-matrix-v1.json, authority/policy reviews and
all executed/final source capsules. Core inventory:
build/nasa-coverage/core-change-review/20261103-function-body-scope/.

All 566 prior rows and contexts remain. Five body-class rows gain two bounded
witnesses each; Star additionally retains the failed custom-creation observation.
No whole-scope flag or eligibility exclusion is assigned. Next prove actual
function-side PropSetup/CoordinateSystem construction and search retained contexts
for the other owner/parameter routes, including the 280 unjoined system parameters.
Preserve all method/axes/provider obligations, E491 conjunctions and already-counted
child scope, E477/SaveMission, E406/terminal output, previous source/science conflicts
and all 59 holds. Full 95% goal remains active and incomplete.

::

  cmake --build build/linux-gui --target GmatQtNasaFunctionBodyTests -j4
  python3 src/qtgui/tests/RunFunctionBodyScope.py
  python3 src/qtgui/tests/RunFunctionBodyScope.py --visible
  python3 src/qtgui/tests/RunFunctionBodyScope.py --filter '^QtGui.NasaFunctionBodyStar$'
  python3 src/qtgui/tests/AuditFunctionBodyScope.py --stage build/nasa-coverage/coverage95-20261103-function-body-scope --output /tmp/function-body-scope-audit.json
  python3 src/qtgui/tests/TrackCoreChanges.py --verify build/nasa-coverage/core-change-review/20261103-function-body-scope

The explicit Star probe is a known failed custom-creation route, not part of the
default positive selection. Both acceptance build targets include the driver;
native X launches require sandbox escalation. Replay only new or affected cases.
