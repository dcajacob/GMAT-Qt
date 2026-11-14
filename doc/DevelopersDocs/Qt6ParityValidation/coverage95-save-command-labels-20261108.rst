Save command-label repair, 2026-11-08
===================================

Save command labels work again with worksheet filenames, legacy .data filenames
and explicit output paths. The original Save editor and extended summary cases
now pass unchanged. All 17 previously deferred command cases have been executed;
15 passed and two hit the existing nested-solver restriction. The combined latest
status of the original 233-case command/boundary plan is 220 passed and 13 failed,
with no cases left unexecuted. This combines runs on different runtime revisions;
it is not a new complete 233-case replay.

The campaign remains active and incomplete. All 105 original open acceptance
rows, scientific holds and qualification gates remain open. No requirement credit
or percentage change follows from this repair.

Repair and verification
-----------------------

Adding Save's optional Filename parameter exposed an old interpreter heuristic:
commands with filename metadata were assumed to take a leading quoted filename.
The interpreter required four quote characters before recognizing their command
label. That made ``Save 'Probe' Sat`` pass ``'Probe'`` as an object operand.
Save's optional filename belongs inside its output-options block, so its leading
quoted token remains a command label. ParseAndSetCommandName now applies that
existing label grammar to Save while retaining the heuristic for other commands.
Only Interpreter.cpp changes production behavior.

The verification source added in the earlier implementation phase now checks
four named Save routes: Worksheet and LegacyData, each with its default filename
and with an explicit Unicode/apostrophe path. Each route checks the native command
name, generated command spelling, serialized object contents, continuation to
the following assignment and native-script serialization/rebuild/run. The two
existing unnamed filename-style checks remain. All original protected test
sources, assertions, fixtures and tolerances remain unchanged.

The current stage executes 31 distinct checks: 24 pass and seven fail. Passing
checks include the original Save editor/summary, nine deferred finite-burn/Global
tree cases, six deferred ordinary control-operation cases, Report/Write,
SaveMission root/branch and named-function/object-I/O regressions. The worksheet
verification case includes the new named Save matrix and passes.

Retained original failures
--------------------------

The remaining 13 failures in the combined 233-case plan are:

* Five Save fixtures expecting an implicit .data suffix: three tree cases and
  two output-operation cases. They now get past parsing/source-edit checks and
  fail on their old output-path expectations. The default worksheet filenames
  satisfy original FRC!E135, and explicit LegacyData remains supported, as the
  user requested. No duplicate output aliases or fixture changes were added to
  make these old checks pass.
* Five solver commands nested in If: Achieve, Vary Target, Vary Optimize,
  Minimize and NonlinearConstraint. The interpreter rejects those command
  placements. The original guards and failed test results remain intact.
* Three Prepare/BeginMissionSequence operation or summary cases, retained from
  the preceding batch and requiring separate mission-boundary diagnosis.

The two Save output-operation errors saying ``Operation input absent`` refer to
their expected .data files, not to missing GUI controls. The original test source
confirms this at its saved-runtime file read. No claim that those original cases
pass is made. Source/editor/summary passes are separate evidence.

Evidence and next work
----------------------

The stage is ``build/nasa-coverage/coverage95-20261107-save-command-labels``;
work crossed into November 8. Read ``validation-summary-v1.json``,
``command-boundary-overlay-v2.json``, ``retained-failures-v2.json``,
``save-command-labels-v1.patch`` and ``publication-v1.json``. Failed pre-repair
results remain in the preceding command-boundary stage. Effective commands,
registrations, per-case logs, receipts and source/runtime hashes accompany both
current runs. All hashes match the completed runs; all handles are terminal.

All 1,334 original protected inputs and the original requirements, worklist and
tracker remain byte-identical. The earlier spacecraft readiness repair and other
production repairs remain intact. Execution used private Xvfb and establishes
no user-visible native-input, platform or whole-row acceptance.

Next diagnose the PrepareMissionSequence/implicit mission-boundary operation
failures using the actual source mapping and parser behavior before choosing a
repair. Preserve original tests and solver validation. Do not broaden function,
resource or script ownership rules to clear these failures.
