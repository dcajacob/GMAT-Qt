Solver metadata registration in ordinary branches, 2026-11-05
==============================================================

The structural binding and marker repairs did not reach the later Execute-time
control-data registration sweeps. Target had two direct-list sweeps and Optimize
had four. Controls inside If/For/While were omitted even after their local solver
binding was repaired, so the solver could begin with unregistered data.

All six sweeps now call the protected nonvirtual RegisterSolverControlData.
The helper obtains and validates the full structural solver scope before any
registration. It registers only the owner's Vary/Achieve or Vary/Minimize/
NonlinearConstraint controls in script order, including ordinary branch arms.
Nested solver descendants remain excluded. Ordinary assignments, propagation,
branch conditions and loop iterations are not executed during this metadata
pass; problem dimensions are fixed before nominal/perturbation passes begin.
Metadata expressions must already have valid values and subsequent control flow
must provide consistent registered controls. No dynamic problem expansion or
invented loop-index initialization was added.

Preserved native ordering
--------------------------

* Target RunInitialGuess: register controls without a Vary reset; StoreLoopData
  remains after registration.
* Target RunAndSolve: StoreLoopData and plot anchors remain before registration;
  when targeterRunOnce is set, Vary SolverReset remains AFTER its Execute.
* Internal Optimize RunInitialGuess: no Vary reset, StoreLoopData remains after
  registration.
* Internal Optimize RunAndSolve: StoreLoopData remains before registration;
  when optimizerRunOnce is set, Vary SolverReset remains BEFORE its Execute.
* External Optimize RunInitialGuess: no new Vary reset, StoreLoopData remains
  after registration and native progress reporting remains in place.
* External Optimize RunAndSolve: no new Vary reset, StoreLoopData remains before
  registration and subscriber anchors remain after it.

The old direct sweep could run an immediate nested solver of the same command
type to completion because that command's GetNext returns itself while active.
The helper preserves that dispatch explicitly, including the completion and
ExitMode effects of GetNext. A single Execute call is not mistaken for a
completed nested solve. A solver inside ordinary control flow is not preexecuted
before its condition/loop runs. Direct nested solvers of a different command
type retain their ordinary runtime dispatch rather than acquiring a new metadata
execution rule. Other graph navigation uses structural next links and avoids
idle/active GetNext side effects. Failed control or nested-solver Execute results
now stop registration with a CommandException rather than being ignored.

Optimize's restored-nominal Vary reporting refresh uses the same structural
scope instead of its separate recursive GetNext walker. It excludes nested
solver controls and reaches all ordinary branch arms. The scope walker is also
seeded with its owner in the cycle guard, so malformed owner escapes fail before
binding or registration. No fields, public signatures or virtual table entries
were added. The mode-specific state machines and numerical formulas remain.

Run-state review and remaining scope
-------------------------------------

In the current in-tree src/base/plugins source, currentRunState references are
confined to GmatCommand.cpp/.hpp, SolverBranchCommand.cpp and Optimize.cpp.
GmatCommand's setter stores the field; ordinary Propagate checks Publisher's
state. Nested solvers manage their own active state. This review does not
justify recursively assigning parent state into every child command, so no
blanket state-propagation change was made. Publisher transitions at nested
solver boundaries remain part of broader context review.

FRC!E7/E55 remain partial: complete native/plugin command contexts, mixed nested
solver/function behavior, retained kernel-pool selection/coverage, immediate
SaveMission and output/cleanup contracts still need source work. FRR!E406's
final-pass/convergence and private returned dependency policies remain open.
The help pages describe metadata registration and its static-data/control-flow
preconditions without claiming general dynamic solver-problem support.

Compilation and accounting
---------------------------

The private integrated Release build passes 81 steps / 199.45 seconds, session
91316 completed. Enabled core, Qt, console, plugin and language-binding clients
compile/link; final source matches the at-build snapshot. Help XML syntax and
essential source inspection accompanied compilation. Configured probe/test
clients linked only. No tests, GUI, missions, functions, models, solvers,
numerical comparisons, evidence readers, seals or qualification runs occurred.

Stage: build/nasa-coverage/coverage95-20261105-implementation-solver-control-registration.
Preimages, exact patches, before/at-build/final hashes, log/receipt and
summary-v1.json are retained. No new requirement-row or acceptance credit;
tracking-v7 remains 1088/1158 (93.96%), 70 other original rows. Recorded verified
baseline remains 1053/1158 (90.9%), all 105 acceptance rows open. Partial 10 /
source review 19, other dispositions unchanged. Original row texts/fingerprints,
assertions, fixtures, tolerances, denominator, ledger and gates remain intact.
