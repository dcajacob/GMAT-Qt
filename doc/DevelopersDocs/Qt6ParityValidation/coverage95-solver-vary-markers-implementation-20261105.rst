Solver-owned last-Vary markers, 2026-11-05
==========================================

SetLastVaryCommand previously searched only direct command lists. Its sandbox
caller visited top-level solver commands, so solvers under ordinary branches or
in function sequences could miss marking entirely. Repeated marking also did
not clear an earlier marker when the last Vary changed.

The action now uses GetCommandsInSolverScope, clears every Vary marker in that
solver's ordinary branch scope, and marks the last Vary in script order. It uses
a checked Vary cast, includes controls inside ordinary branches, and excludes
nested solver descendants. Each solver performs the action in its Initialize
after successful recursive child initialization. The existing sandbox action
remains compatible and idempotent; nested solvers and function-local solvers no
longer depend on that top-level-only caller. BranchCommand::Initialize returning
false is now propagated before publishing a marker/readiness result, rather
than being ignored. No class layout, virtual table or public signature changed.

This preserves the native last-in-script-order convention for ephemeris chunk
notifications. It does not predict which conditional branch will run or create
new numerical/solver state-machine behavior. Control-flow paths must provide
consistent solver variables/goals; registration sweeps inside Target/Optimize
Execute still require their own implementation review. Runtime registration,
child state delivery, kernel-pool isolation, full native/plugin command contexts,
returned dependency ownership and final-pass policies remain open. Marking a
Vary does not establish any event or ephemeris numerical behavior.

The private integrated Release build passes 66 steps in 4.05 seconds; session
61986 completed. Enabled core, Qt, console, plugins and binding clients link;
final source equals at-build source. Preimages, exact patches, before/at-build/
final hashes, log/receipt and summary-v1.json are in
build/nasa-coverage/coverage95-20261105-implementation-solver-vary-markers.
No tests, GUI, mission/function/model/solver execution, numerical comparisons,
proof readers, seals or acceptance qualification were run.

FRC!E7/E55 and FRR!E406 remain partial. Provisional engineering tracking stays
1088/1158 (93.96%), with 70 other original rows partial/unclassified/dependent.
Recorded verification stays 1053/1158 (90.9%); all 105 original acceptance rows
remain open. Planning dispositions remain partial 10 / source review 19 and
other counts unchanged. No original row text/fingerprint, assertion, fixture,
tolerance, denominator, ledger or qualification gate was changed.
