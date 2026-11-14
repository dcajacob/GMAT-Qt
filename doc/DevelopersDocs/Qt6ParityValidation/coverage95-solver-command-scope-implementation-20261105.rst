Solver controls in ordinary nested branches, 2026-11-05
=========================================================

Target/Optimize initialization previously bound the cloned solver only to
controls in the immediate command list. Vary/Achieve and optimization controls
inside If/For/While branches could therefore retain a different solver binding
or fail initialization. Optimize also omitted nested objectives/constraints from
its nonempty-goal count. Apply Corrections skipped nested Vary commands.

The protected, nonvirtual GetCommandsInSolverScope helper now walks structural
command links in script order, descends every ordinary branch arm, and stops at
each owning branch's back-link. It uses GetNextWhileExecuting rather than the
execution-dependent GetNext; the latter can re-enter a live branch and a solver
can change publisher state when returning its next command. An explicit work
stack avoids recursive C++ traversal, and repeated graph nodes / unterminated
branches fail before returning a binding list. Each visited command is returned
once. No instance fields or virtual table entries were added.

A nested SolverBranchCommand is included as a command but its children belong
to its own scope. Its controls are neither rebound nor counted by the parent,
including when both sequences name the same configured solver resource. Target
and Optimize use this helper before recursive branch initialization; Apply
Corrections uses it when updating Vary initial guesses. Existing solver-name
matching in each control's SetRefObject remains authoritative. No new name
restriction or fallback to another solver was introduced. Existing objective,
variable and goal execution rules remain. Static binding/counting does not
force conditional branches to run or invent a changing solver problem.

The Target/Optimize reference pages explain these scope and control-flow rules.
No accepted-pass/convergence policy, solver state machine, numerical algorithm,
last-Vary marker or general run-state delivery was changed in this batch.

Remaining scope
----------------

FRC!E7 and FRC!E55 remain partial implementation. This repair does not establish
all native/plugin command, nesting, context, output and lifecycle contracts.
The inspected registry includes ordinary flow, assignments, propagation/burns,
reporting/plots, solver controls, functions, FindEvents, SaveMission, Save/Set,
file thrust, estimation/simulation/smoothing, Python and PrepareMissionSequence;
registration alone does not close any of their contextual obligations. Remaining
review includes last-Vary notification markers, child run-state delivery,
internal event scratch kernels, nested function/solver initialization, immediate
SaveMission, plugin command restrictions and external dependencies. Required
structural grammar (matching branch terminators and proper solver control
contexts) is preserved; arbitrary invalid nesting is not a supported command.

Compilation and accounting
---------------------------

The private integrated Release build passes: 82 steps, 189.83 seconds, session
31558 completed. All enabled core, Qt, console, plugin and language-binding
dependents compiled/linked. Final source hashes match the at-build snapshot.
Only help XML syntax and essential source inspection accompanied compilation;
no test, GUI, script, function, model, solver, numerical or acceptance execution
was performed. Some configured probe/test clients linked but were not run.

The provisional tally remains 1088/1158 (93.96%), with 70 other original rows
partial/unclassified/dependent. Recorded verification remains 1053/1158 (90.9%);
all 105 original acceptance rows remain open. These two rows move from source
review to partial implementation in the planning map: partial 10, source review
19; other disposition counts and tracking-v7 are unchanged. Original assertions,
fixtures, tolerances, row text/fingerprints, denominator, ledger and gates remain
unchanged. No acceptance credit is claimed.

Artifacts: build/nasa-coverage/coverage95-20261105-implementation-solver-command-scope
contains preimages, before/at-build/final hashes, exact patches, build log/receipt
and summary-v1.json. The running source used for the next implementation stage
is exactly the source captured in this stage.
