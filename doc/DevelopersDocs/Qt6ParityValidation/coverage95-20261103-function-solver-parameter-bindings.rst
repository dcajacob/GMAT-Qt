Solver Parameter owner references and caller status-cache isolation
=================================================================

Canonical coverage remains **1,019/1,158 (88.0%)**, 139 open and **82 more
needed** for 1,101. Historical coverage stays 1,816/2,367. No original row,
whole E491 conjunction, factory role or prerequisite hold is newly qualified.
The full 95% goal remains active and incomplete.

Observed defect and bounded repair
----------------------------------

SolverState and SolverStatus expose a live scoped Solver through GetRefObject,
but their Evaluate methods read GmatGlobal's process-wide name cache. Actual
private DifferentialCorrector Target clones converge inside a function while
the caller's same-named untouched owner remains Created. Before this increment,
the caller nevertheless reads 1/Converged after the function returns. Both new
Scoped probes fail on that exact post-return mismatch.

FunctionManager.cpp now snapshots caller non-Global Solver cache entries before
function preparation and restores those integer values on scope exit. Explicit
Global solver entries remain shared. The same cases exercise a Global solver
that actually converges and retains its completed status after return. A runtime
array-bounds failure after both solves checks unwinding and then a successful
recovery. Numerical formulas, clone selection, Parameter factory membership,
status codes/string mappings, members and virtual slots are unchanged.

The retained working preimage is FunctionManager.cpp.before, not repository HEAD.
The exact increment is core-v6-caller-solver-cache.patch. The pending core census
remains 238 paths: 237 prior hashes unchanged, one existing FunctionManager.cpp
incremented and no new path. Technical/scientific/full-current-build/NASA gates
remain false.

Evidence and preserved failures
-------------------------------

Twelve selected GUI passes comprise four new Scoped observations and eight
unchanged local-resource/output and Global-resource DifferentialCorrector/Yukon
regressions. Each new case has three normal missions with exact file Cancel/Save,
Unicode reload and no-explicit-Build repeat, plus a runtime failure and a recovery
mission. Across both modes the new cases record 60 live callback samples, 120
store-owner references, 120 actual Write values, 120 raw Report values and 72
live Target-clone observations. Four failure/recovery pairs retain private cache
0, Global cache1 and exact caller/prototype/history/file invariants.

Six visible processes have actual image maps, stable before/after source and
binary hashes, initially minimized/resizable/nonfullscreen software rendering,
and zero screenshots/runtime-binary/kernel copies. Nine corrupt-proof controls
must reject private leaks, Global rollback, wrong references/clones, missing
post-return/no-Build observations, and broken error/recovery custody.

Two compile failures remain source-versioned: the initial API/hidden-overload
mismatch and a later authoring scope/type mismatch. The first native attempt was
interrupted at the exact revalidated runner PID3314354 because the test walked
virtual execution order. Its terminal interrupted receipt and exact sources are
retained; the second test was unstarted, not relabeled as failed or passed.
The corrected walk uses GmatCommand::GetNext.

Two initial strict-reference/legacy-label probe failures remain distinct from
the two real caller-cache failures. GetRefObject returns the scoped store owner;
it does not expose the Target execution clone. Current APIs return raw enum
codes and Created/Converged strings, while older unused SolverData normalization
says Ready. The original strict external-clone probes remain registered and
unqualified under qt-gui-solver-external-clone-probe. New separately registered
Scoped cases measure the actual cache contract and caller isolation. No original
whole-row assertion is weakened or credited by changing that fixture assumption.

Scope and remaining work
------------------------

All 566 original factory-role rows and every prior field remain, with additive
solver_parameter_binding_witnesses only. All 321 system Parameters retain whole
obligations. Live store-owner/reference witnesses now cover 59 types; 262 still
need them: 246 Spacecraft, 14 SpacePoint and two ThrustAngle metadata owners.
There are 100 types with any joined bounded context and 221 with none. The 41
older bounded names remain in the native queue. Removing the two observed Solver
names from that queue is not a factory/readable-role exclusion.

Parameter evaluation remains name-cache based. Actual Target clone pointers and
scalar literal convergence are checked independently while function objects are
alive; no Parameter-to-execution-clone pointer identity or private-pointer access
after return is claimed. Other status codes, solver classes, recursive/passed
objects, Global promotion and general exception combinations remain unqualified.
Finite frame/provider/function-readonly flags, nine unmodeled System probes,
strict-time/custom-Star probes, E491/E477/E406, earlier science/source conflicts
and all 59 holds remain. The licensed MATLAB installation-path question remains
pending, with no hold waived.

Replay and custody
------------------

Build and run only the new cases::

  cmake --build build/linux-gui --target GmatQtNasaFunctionSolverParameterBindingTests -j 2
  python3 src/qtgui/tests/RunFunctionSolverParameterBindings.py
  python3 src/qtgui/tests/RunFunctionSolverParameterBindings.py --visible

Run the affected unchanged resource checks::

  python3 src/qtgui/tests/RunFunctionSolverParameterBindings.py --filter '^QtGui.NasaFunction(GlobalResource|Resource)(DifferentialCorrector|Yukon)$'

The default selects SolverStateScoped and SolverStatusScoped; the original strict
probes remain separately registered. Both acceptance targets depend on the new
executable. Replays create new receipts; historical receipts are not overwritten.

Authoritative artifacts under
build/nasa-coverage/coverage95-20261103-function-solver-parameter-bindings/ are
solver-parameter-binding-matrix-v1.json,
remaining-parameter-binding-worklist-v1.json, evidence-inputs-v1.json,
evidence-audit-v1.json, final-sources-v1.tar.gz, stage-receipt-v1.json and
seal-verification-v1.json. The report states the bounded outcome; only a passed
seal verifies the current final custody. Audit and core inventory replay::

  python3 src/qtgui/tests/AuditFunctionSolverParameterBindings.py --stage build/nasa-coverage/coverage95-20261103-function-solver-parameter-bindings --output /tmp/solver-parameter-audit-replay.json
  python3 src/qtgui/tests/TrackCoreChanges.py --verify build/nasa-coverage/core-change-review/20261103-function-solver-parameter-bindings
