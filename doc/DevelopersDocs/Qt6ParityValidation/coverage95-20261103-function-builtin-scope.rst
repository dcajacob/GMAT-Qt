Three missing built-in function routes and GetLastState repair — 2026-11-03
=========================================================================

Coverage remains **1,019/1,158 (88.0%)**, with 139 open and **82 more needed**.
Historical operations remain 1,816/2,367 (76.7%). No exact requirement is newly
admitted. Original source hashes, denominators, fingerprints, assertions,
whole-row conjunctions, all 59 holds and separate approval gates remain intact.

New actual main and function-local use
-------------------------------------

Angle, GetLastState and GetEphemStates were the three built-in spellings without
joined function-local execution contexts. Existing main GetLastState/GetEphemStates
checks were found first and reused for scope selection. The original built-in
driver/catalog and original main-menu regression are unchanged. A separate driver
derives their file-editor, history, prototype, typed-result and report workflow
from the original and adds distinct main/private literals, direct/direct/nested
calls and a third mission without an explicit Build.

Angle checks an authored 45-degree caller geometry and a private 90-degree
geometry. GetLastState checks two different six-component Cartesian literals.
GetEphemStates reads two hand-authored degree-one OEM endpoint fixtures, with
different caller/private states and exact UTC Gregorian endpoint strings. The
OEM files are literal inputs, not runtime kernel copies or an empirical oracle.
No interpolation or propagation occurs. Supported strcmp expressions and actual
Script-style epoch-string Writes retain exact epoch comparisons.

All three cases pass both modes: **six new GUI passes**. Together with the
unchanged main-menu Commands regression in both modes, final selection is
**eight GUI passes**. New scope checks 18 missions, 72 GmatFunction calls,
54 local and 18 main built-in calls, 504 raw report scalars, 48 exact returned
epoch strings and 54 actual shared-counter store identities. It also checks
66 same-name caller object states/arrays/scalars/strings against independent
main literals, caller helper native automatic/global flags, private-name and
configured-prototype isolation, actual function-file keyboard Cancel/Save,
exact source/history/files/run lock, Unicode reopen and actual report viewing.

The runtime caller checks were added after the first bounded passes to close
a real assertion gap: configured prototypes alone would not detect mutation of
the caller's runtime Spacecraft, state Array or epoch String. Only the three
affected new cases repeat to v7. These retain distinct caller/private outputs;
none of the expected literals or 1e-9 bounds are relaxed.

Two binding defects in one production file
-----------------------------------------

GetLastState initially returns six zeros from all three private calls, while
its main call returns the authored nonzero state. A focused native control
distinguishes the actual input wrapper's Spacecraft from two formal placeholders.
The original implementation reads the placeholder values, including a nonzero
decoy. GetLastState.cpp now reads the actual bound Spacecraft reference and
rejects an absent/wrong bound input. This passes the input controls but the GUI
case still returns zeros; the intermediate failure is retained.

Expanded native controls then show that the returned Array is detached from the
formal output object. The existing implementation allocates another Array while
the function frame's retained formal Array remains unchanged. The final repair
updates that existing typed Array when present and returns a wrapper referring
to it, matching the other built-ins' formal-output convention. The legacy
detached-output route remains for callers without a formal output object.
Three final native controls pass input literals, unchanged input/decoy state,
output values and exact formal-output identity. Main/direct/nested GUI calls and
the unchanged main-menu regression then pass. No new member, virtual slot or
state formula changes; general output/manager ownership remains unqualified.

Exact GetLastState.cpp preimage, wrapper-only core-v5 patch/source and final
core-v6.patch remain. The schema-2 core review inventory is **234 pending paths**,
221 tracked differences and 13 untracked: all previous 233 hashes are unchanged,
with GetLastState.cpp newly pending. This is a worktree/HEAD review inventory,
not exclusive authorship, release comparison or technical/scientific approval.

Failures, versions and visible execution
---------------------------------------

All **seven failed CTest attempts** remain: four GUI failures and three support
failures. Two GUI failures are the unchanged GetLastState zero result, before
and after the input-only repair. Two are the GetEphemStates fixture's unsupported
String operands on ConditionalBranch. The second captures the actual Build
diagnostic; replacing the unsupported If form with strcmp preserves both exact
epoch literals and records the actual returned strings. No production parser
change is made for that fixture error.

Support v1 is a startup-only failure before any binding control; the launcher
then supplies the existing startup file. Support v2 retains two input-binding
failures and one successful subcontrol. Input-only v3 passes but does not test
formal-output identity. Expanded v4 fails all three output-identity controls;
final v5 passes all three. The input-only support pass and four earlier bounded
GUI passes remain superseded for the final expanded scope. All exact executed
source versions and partial observations are retained. There is no compile
failure and no weakening of an original test or numerical bound.

The final headless selection reuses the unaffected original regression from v6
and takes the three expanded cases from v7. Visible v1 passes all four selected
cases with four exact mapped processes and stable before/after source/runtime
inputs. Windows are initially minimized, resizable and nonfullscreen, with zero
screenshots or runtime/kernel copies. Process identity does not establish a
physical GPU or human observation. All build/native handles are terminal.

The auditor rejects five corrupt proofs: zero private state, mutated caller
shadow, missing nested output, omitted repeat and wrong shared-counter pointer.
All 566 prior role rows and contexts remain unchanged; each of the three new
built-in rows gains two bounded witnesses and no qualification flag changes.

Custody and remaining scope
--------------------------

Authoritative stage:
build/nasa-coverage/coverage95-20261103-function-builtin-scope/.
Use evidence-audit-v1.json, evidence-inputs-v1.json, stage-receipt-v1.json,
seal-verification-v1.json, builtin-scope-matrix-v1.json and all executed/final
source capsules. Core inventory:
build/nasa-coverage/core-change-review/20261103-function-builtin-scope/.

Four nominal automatic entries still have no joined execution context in this
matrix: Star, Moon, Comet and Asteroid. Planet retains existing Earth model use
only; creation/use of all five celestial classes inside functions remains an
obligation. Function-local PropSetup/CoordinateSystem creation and actual shared
instance identity, other owned method/axes routes and 280 exact system-parameter
retained-context searches stay open. These three built-in routes do not establish
all provider/frame/domain behavior or close whole E491/E493/E497. Its nine child
rows are already recorded and earn no duplicate credit. E477/SaveMission,
E406/terminal output, prior science/source conflicts and all holds remain.
Full 95% goal active and incomplete.

::

  cmake --build build/linux-gui --target GmatQtNasaFunctionBuiltinScopeTests GmatGetLastStateBindingTests -j4
  python3 src/qtgui/tests/RunFunctionBuiltinScope.py
  python3 src/qtgui/tests/RunFunctionBuiltinScope.py --visible
  ctest --test-dir build/linux-gui --no-tests=error -R '^QtAcceptance.GetLastStateBinding$' --output-on-failure
  python3 src/qtgui/tests/AuditFunctionBuiltinScope.py --stage build/nasa-coverage/coverage95-20261103-function-builtin-scope --output /tmp/function-builtin-scope-audit.json
  python3 src/qtgui/tests/TrackCoreChanges.py --verify build/nasa-coverage/core-change-review/20261103-function-builtin-scope

The runner accepts --filter/--output; both acceptance build targets include the
new executables. Native X launches require sandbox escalation. Reuse completed
source versions and run only new or affected cases.
