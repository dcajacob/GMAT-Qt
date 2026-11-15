Mission Tree command and operation scope, 2026-11-05
====================================================

The original All Others!E160 list is E161-E181: 21 command families.
All Others!E196 lists six operations in E197-E202: Open, Close, Insert before,
Insert after, Delete and Command Summary. Rename is an additional GUI feature,
not a substitute for one of these original operations. Solver leaves E182-E188,
expansion E189-E195 and branch deletion E203 are separate sibling requirements;
this pass adds no engineering credit for those siblings.

Source implementation inventory
--------------------------------

The retained source-inventory-v1.json maps every original command and operation
to its owning routes, retaining original texts and fingerprints. It records 21
command entries, six operation entries, 126 shared command/operation route pairs
and hashes of 24 owning sources. These are source routes, not executed cases.

MainWindow::makeCommandPanel supplies the following templates/configuration:

* Optimize and Target: solver header/options and complete enclosed source.
* If/Else, While and For: condition or loop fields and complete enclosed source.
* BeginFiniteBurn and EndFiniteBurn: finite burn/spacecraft controls; draft default
  resources commit only on Apply. Selecting a Begin supplies its End references.
* Maneuver: burn and spacecraft.
* Assignment: destination and expression.
* Toggle, PenUp and PenDown: subscriber selection and Toggle state.
* Propagate: propagation form, group/stopping/color dialogs and source editor.
* Report: report file and ordered parameters.
* Save: objects and filename/style options.
* ScriptEvent: BeginScript/EndScript template and dedicated comment/body editor.
* RunSimulator and RunEstimator: their configured solver and estimator options.
* GMAT and MATLAB function calls: configured function, ordered inputs/outputs.
* Stop: source and optional command name, with no command-specific operands.

CommandFactory supplies the core classes. SaveCommandFactory,
EstimationCommandFactory, GmatFunctionCommandFactory and
CallMatlabFunctionFactory supply the respective plugin classes. Function-call
syntax reaches the appropriate factory through Interpreter. Optional templates
depend on registered providers, and configured resources remain prerequisites.
CommandEditor retains direct source editing alongside the specialized controls;
unrecognized form syntax is not silently rewritten into a template.

The common mutation route is applyMissionChange -> editMission ->
applyModelScript. It requires the current built source and an idle mission.
Native interpretation and solver-placement validation precede source/history
commit. Failed interpretation or validation reconstructs the previous model.
Snapshot ranges retain original text; branch ranges include their children and
both If arms. Insert-after uses the branch's full end; append-inside uses its own
Else or closing node. Solver leaf choices follow the enclosing solver and the
single-Minimize rule. ScriptEvent internals use its body editor.

The six operations use generic routes for these command families:

* Open: tree activation or Edit command opens/reuses the selected command editor.
* Close: closes its owning subwindow, with the unapplied-input discard guard.
* Insert before/after: opens a pending editor and commits through the common
  source transaction after Apply.
* Delete: confirms deletion and removes the entire selected range, including
  branch descendants.
* Command Summary: uses the selected native command and the current run/source
  checks. ScriptEvent maps to its matching EndScript summary.

Structural separators/terminators retain native enclosing-branch editing, as in
the wx MissionTree menu. BeginMissionSequence cannot be independently removed
or preceded by an executable command. These grammar rules do not remove one of
the original 21 configurable command families. A source range that cannot be
aligned safely, including externally included command text, retains script-editor
handling. SaveMission is already editable; its command-label parser exception
disambiguates its filename rather than hiding the command.

Insertion separator repair
---------------------------

editMission previously placed inserted text directly at the mapped boundary.
After a final command without a semicolon or newline, this could concatenate
the existing command and the new one. A trailing comment without a final newline
could also absorb the inserted text. Insert-before/after now prepend a newline
when the preceding source character is not a newline. Existing mapped ranges,
comments, branch placement and normal line-separated source remain intact.
Append already had its own equivalent separator handling.

Compilation and tracking
--------------------------

The private integrated build passes five steps in 5.04 seconds, session 11150
completed. Final source hashes match the at-build snapshot. The stage retains
the exact preimage, before/at-build/final hashes, patches, source inventory,
build log/receipt and summary-v1:
build/nasa-coverage/coverage95-20261105-implementation-mission-tree-scope.

All Others!E160/E196 are added only to provisional engineering implementation
presence. Tracking-v9 is 1091/1158 (94.21%), with 67 other original rows partial,
unclassified or dependent. There are 38 added engineering rows: 32 new/repair/
compatibility and six existing. Recorded verified acceptance remains 1053/1158
(90.9%); all 105 original acceptance rows remain open.

No tests, GUI, mission/function/model/solver execution, numerical comparisons,
acceptance readers or seals were run. Universal plugin serializer compatibility,
source-range/GUI behavior and runtime regressions still need qualification after
the complete implementation pass. No original requirement, text/fingerprint,
assertion, fixture, tolerance, denominator, ledger or gate changed.
