# Safe local resource Rename — 2026-10-03

The previous blanket refusal for any include or configured GMAT function prevented renaming an unrelated main-file resource. The unchanged baseline proves both refusals without modifying source, objects or external files. This is an additional ordinary GUI limitation found after the completed 108 workflow audits; it is not a waiver of their acceptance scope.

## Delivered operation

F2 and Resources/Edit Rename now require exactly one declaration in the current main source. Before mutation, a read-only dependency reader follows literal includes through the engine's current include search order, configured GMAT function files (including aliases), nested includes and nested function dependencies. Both original and proposed identifiers, including an automatically renamed PropSetup companion ForceModel, are checked. Missing, unreadable, dynamic, cyclic, unclassifiable or potentially affected dependencies refuse with an explanation. External files are never rewritten. Conservative identifier matches inside quoted text can also refuse; coordinated external-file Rename remains a script-editor workflow.

The existing pending/running/stale-source guards, protected resources, source/comment/literal preservation, atomic interpretation/rollback, one Undo/Redo edit and Save/reopen behavior remain. No numerical-engine source changed.

## Relevant verification

The actual application build exited **0**, followed by a successful build of the directly affected Rename target and **QtGui.ResourceRename: Passed, 0.62 s, CTest exit 0**. New cases cover unrelated and nested literal includes, aliased and nested GMFs, included-resource ownership, original/proposed-name references and collisions, companion ForceModel references, missing/unreadable/dynamic/cyclic/unresolved dependencies, unchanged external bytes and unchanged rejected source/model/Undo. Accepted edits have exact one-edit Undo/Redo and Unicode Save/reopen. The executing nested-function fixture reports the independently known value **10**; the include fixture reports **12**. Previously passing examples, tutorial missions and unrelated checks were not repeated.

An owned authenticated Xvfb/Openbox/software-X11 session also verifies actual **F2 → Rename → Undo → menu Redo → Save As → Run → file-list reopen → normal Exit**. The main-owned scalar is renamed despite an unrelated literal include and configured unused GMF. Its complete saved source is exactly the original UTF-8 source with the intended identifier replacement; the café comment remains unchanged. The independently authored arithmetic reports **10**. Both external files remain byte-identical. The native case establishes GMF dependency inspection; actual function execution is covered by the focused fixture separately. Application/WM/Xvfb exits are **0/0/0**.

## Runtime and retained attempts

`application/bin/GmatQt`: **6,110,104 bytes**, SHA256 `e955be000a69082e055e4bf95b5ccd41f7a67513cb093435f56e632cf9956b1c`. Base `2082a341...`, util `f9844517...` and startup `5f80be1f...` are unchanged. Exact app/source/startup identities and compiled sources are retained in `build/example-qualification/20261003/safe-local-rename`.

That directory preserves baseline/build/check argv and logs, all three failed new test-fixture attempts, final checks, native inputs/driver, screenshots/action logs/session receipts and `native-verification.json`. Test-fixture corrections concern permission-resolver wording, diagnostic evaluation order and a missing function BeginMissionSequence. Two earlier native attempts retain invalid self-authored GMF declarations and the failed runs; the final native arithmetic fixture avoids requiring that helper to execute. Incorrect Linux Redo key selection and file-dialog autocomplete/input settling are preserved as operator attempts, not attributed as product failures. Final Undo/Redo, successful execution, saved source and reopening are independently visible in the final session.

Live host testing remains stopped. The saved Mutter null Wayland-resource fault, GMAT client attribution, safe host condition, host portal/focus/window behavior and physical GPU acceptance remain distinct and unverified. Windows/macOS and MATLAB are deferred. This repair does not establish full Linux replacement qualification.
