Control, dynamics and solver operations with explicit placement restrictions
=============================================================================

Coverage remains **1,019/1,158 (88.0%)**, 139 open and **82 more needed**.
Historical operations remain **1,816/2,367 (76.7%)**. No original key is admitted.
All Others!E196 retains its whole-command conjunction and original fingerprint;
all 59 prerequisite holds and scientific/core/current-build/full-NASA gates remain
open.

Supported contexts and retained rejected attempts
------------------------------------------------

Seventeen fixture families cover Propagate, Maneuver, BeginFiniteBurn,
EndFiniteBurn, For, If, IfElse, While, Target, Achieve, VaryTarget, Optimize,
VaryOptimize, Minimize, NonlinearConstraint, Assignment and Stop. This corresponds
to fifteen distinct native factory types, with the assignment exposed as GMAT.
The Assignment fixture adds the explicit labeled expression Count=2*U+1 before
the retained Witness report; every other mission script is unchanged from its
retained reference.

Every supported context exercises actual tree Open/reuse, double-click Open,
tree Close, before/after insertion, Delete Cancel/confirmation, exact source
preservation, parent/sibling identity, Undo/Redo, context Summary and Unicode
save/reopen. Entire selected branches are handled as source intervals, preserving
their bodies and closing commands. Each restored mission runs twice.

Original placement is recorded from the native tree. Achieve/VaryTarget are direct
children of Target; VaryOptimize/Minimize/NonlinearConstraint are direct children
of Optimize. They are not described as root commands. Other selected commands
start at root. For guarded placement, ordinary commands are wrapped by If, and
solver children keep their direct solver parent while the entire Target/Optimize
block is wrapped by If.

The initial thirty-four attempts produced twenty-seven passes, two Stop-summary
failures and five rejected placements. The five attempted direct If placements
of solver children are explicitly rejected by Interpreter::ValidateSolverCmdLevels
with "Solver command, and cannot be nested inside a If Control Logic command"
(the engine includes quotes around If). This source rule is retained and bound.
The Mission Tree guide also identifies the required Target/Optimize contexts.
The supported guarded alternatives use a separate driver and five new
registrations; the original driver and assertions remain unchanged.

The original direct-If probes remain ordinary failing registrations, with no
WILL_FAIL, skip or assertion inversion. They are reproduced visibly and recorded
as ten rejected-placement outcomes, never as GUI passes. The default replay
selects supported contexts; --include-rejected-placements includes those five
probes and intentionally exits failing. A full registration run therefore still
contains these known failures and must not be reported globally green.

Stop summary repair and native results
--------------------------------------

Stop correctly interrupted the mission and suppressed the following report, but
its actual command summary lacked spacecraft state. The sole production change
adds BuildCommandSummary(true) immediately before the existing CommandException.
The exception text, interrupted-run result and subsequent-command suppression
remain unchanged. Exact preimage, one-line core-v2.patch, original failures,
source versions and before/after GmatBase runtime identity are retained.

After that repair, only the two Stop cases, five supported solver-parent cases
and existing Script Event Stop regression ran unattended. All eight passed.
The earlier twenty-seven unattended passes were reused. Visible execution then
covered the entire attempted census: thirty-four supported cases and one affected
regression passed, while the five unsupported placement probes failed exactly
as recorded. Forty visible process maps bind the actual loaded runtime.

This gives **68 new GUI passes**, **two affected-regression passes**, **136 raw
twelve-field operation rows** and **816 summary components**. All visible windows
start minimized, remain resizable/non-fullscreen, and retain zero screenshots.

Independent references
----------------------

The orbit uses literal mu=398600.4415 and radius=7000 km over twenty seconds.
Propagate's selected first command summary is checked at ten seconds; the other
summaries are checked at twenty seconds. Maneuver adds the prescribed inertial
0.01 km/s X impulse after propagation. The finite-burn pair has zero propagation
interval: three live debugger observations prove inactive/active/inactive states
before BeginFiniteBurn/EndFiniteBurn/Report, and the pair ends inactive. This does
not claim integrated thrust qualification.

For/While execute three increments; If selects +7, and IfElse selects -7.
Assignment gives 2*1+1=3. Target solves 2*U+1=9, so U=4. The unconstrained quadratic
Cost=(U-2)^2 has U=2, Cost=0; the constrained U>=3 case has U=3, Cost=1. These
literal algebraic results are independently compared with raw reports. Stop must
retain the pre-stop report and leave the following report absent in both original
and guarded positions, including Unicode reopen.

The predeclared new-test bounds are 1e-4 s, 2e-5 km, 2e-8 km/s, and 1e-12 scalar
values for exact nonsolver fixtures. Solver scalar bounds retain the original
reference driver's 2e-5 tolerance. Summary display bounds remain 2e-5. No bound
or numerical assertion changed after a native failure.

Audit, census and next work
---------------------------

AuditCommandControlOperations.py checks source-v1 reused passes and source-v2
repaired/new/visible evidence, the exact Stop delta, unchanged original assertion
sources, native placement diagnostics, actual parent identities, raw numerical
reports, command-epoch summaries, burn phases, stopped execution, stdout/JUnit,
visible exposure and process maps. Four corrupt controls reject wrong state,
missing tree Close, missing reopen and wrong native parent. Its status is
passed-bounded-supported-control-operations-with-placement-rejections, with an
empty qualified-key list.

The bounded stage is build/nasa-coverage/coverage95-20261103-command-control.
Its parent-operation-status-v1.json retains all 53 factory entries and previous
audited evidence. **37 entries** now have bounded six-operation evidence in the
selected supported contexts. **16 remain**: Assignment, BeginMissionSequence,
BeginScript, CallFunction, Create, Else, EndFor, EndIf, EndOptimize, EndTarget,
EndWhile, EndScript, Equation, NoOp, ScriptEvent and PrepareMissionSequence.
Assignment/Equation aliases have not been silently counted from the native GMAT
case. Structural restrictions and starter semantics remain explicit obligations.
This worklist does not qualify the whole parent or all possible positions.

The authoritative ledger remains the default-orientation coverage-snapshot-v1.
The stage receipt retains all 59 holds and storage/source hashes. Regular-file
bindings exclude runtime symlinks; no runtime binaries or large kernels are
copied. Continue the structural/starter/alias conjunction using the retained
Mission Tree guide, Interpreter placement rule and actual tree actions. The full
95-percent goal is active and incomplete.

Replay
------

.. code-block:: sh

   cmake --build build/linux-gui --target GmatQtNasaCommandControlOperationTests GmatQtNasaCommandSolverParentOperationTests GmatQtNasaScriptEventCommandTests -j4
   python3 src/qtgui/tests/RunCommandControlOperations.py
   python3 src/qtgui/tests/RunCommandControlOperations.py --visible
   python3 src/qtgui/tests/RunCommandControlOperations.py --include-rejected-placements
   python3 src/qtgui/tests/AuditCommandControlOperations.py \
     --stage build/nasa-coverage/coverage95-20261103-command-control \
     --output /tmp/command-control-audit.json

Supported/rejected contexts retain distinct registration suffixes Original,
OuterIf and SolverOuterIf. The third replay command deliberately includes the
five native rejected-placement probes. The existing Stop regression retains its
original registration and assertions.
