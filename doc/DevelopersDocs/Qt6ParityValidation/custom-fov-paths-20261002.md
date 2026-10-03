# CustomFOV file-path lifecycle — 2026-10-02

The Qt creator and script reconstruction now preserve an existing `FOV_DATA_PATH` expression, including startup aliases. A bare relative mask available only in that configured fallback can be created, applied, rebuilt and reopened without losing its lookup path. Pending selection, Help/F1 and Cancel retain both the configured path and its prior absence.

## Observed defect and bounded fix

A new standalone baseline compiled against unchanged `82418383` frontend/runtime produced **15 failures** ([raw baseline](custom-fov-paths-20261002.baseline.txt)). `CustomFOV` construction unconditionally installs `../data/hardware` in FileManager. The pending factory draft could overwrite the user's fallback before validation; every Qt stream interpretation also reconstructed the object and reset it. Apply could validate a relative file and commit its authored filename, then a later Initialize failed to resolve that same file.

`resourceDraft` now restores the FileManager snapshot immediately around pending CustomFOV construction. Detached grouped validation keeps a guard alive through file resolution, initialization and copyback. It restores prior state on acceptance or rejection while permitting temporary first-use defaults when no fallback existed.

The Qt interpretation wrapper preserves only an already-defined FOV path after success, a false result or an exception. All five routes use it: Build, folder Build, folder original-model restoration, rejected-edit model restoration and Apply. Other interpreter path effects remain intact. An additive read-only FileManager getter returns the stored expression without alias expansion; using a resolved pathname would discard the authored alias. Accepted interpretation with no prior FOV path still installs the engine's ordinary default. No CustomFOV numerical algorithm or engine constructor was changed.

## Focused verification

The application and selected targets rebuilt successfully before tests. [Raw checks](custom-fov-paths-20261002.checks.txt) record:

| Check | Result | Relevant scope |
| --- | --- | --- |
| New `QtGui.CustomFovPaths` | Pass, 0.31 s | Raw alias and absent path; actual creator selection/Help/F1/Cancel; first absolute-file detached validation; default installation on accepted creation; valid/invalid relative-file creation and Apply; rejected interpreted candidate and model restoration; failed Build/rebuild; exact source/Undo; Unicode Save/reopen; initialization of reconstructed file masks |
| Affected `QtGui.CustomFovControls` | Pass, 0.35 s | Changed detached validation guard; paired modes, interpolation, file transitions, rollback and existing known interior/exterior checks |
| Affected `QtGui.FolderRun` | Pass, 2.60 s | Changed batch/original-model interpretation routes; existing bounded run and engine/path/document restoration checks |

The new check uses a four-row owned cone/clock file in a separate temporary fallback directory, absent from the Unicode mission directory. It initializes clones after reconstruction and checks the exact literal `FOV_TEST_PATH/` definition. It performs no mission propagation. FolderRun retains its existing bounded execution; no completed audit, tutorial, example corpus or unrelated passing suite was repeated.

The baseline rejected-Array fixture was malformed and did not reach interpretation. The strict test corrects it to valid JSON with an undefined operand, and confirms rejection through the mission interpreter followed by restoration. The first application build failed because the wrapper accepted `std::istream` while Moderator requires `std::istringstream`; the corrected signature rebuilt successfully. Both initial failures remain recorded, with no numerical fixture substitution.

## Runtime binding and limits

[Receipt](custom-fov-paths-20261002.receipt.json) binds source hashes, compiled-source snapshots, build/check records and runtime identities. Current GmatQt is **6,027,456 bytes**, SHA256 `02d7711e11dc83843e07c4cbef469ec5557d2842777b74fd11a8ee8d039708ee`. The base library is byte-identical to the preceding checkpoint, SHA `2082a341…`; startup remains SHA `5f80be1f…`. Util changes to SHA `f9844517…` for the additive read-only path getter. Raw build logs and the standalone baseline source/compiler command remain under `build/example-qualification/20261002/custom-fov-paths/`.

This qualifies the specified Qt fallback lifecycle. Existing script-relative subpath semantics, broader interpreter/file regimes and FOV scientific regimes remain distinct. The unchanged core's exact-boresight discrepancy remains unqualified. No new native desktop/GPU result is claimed; the prior private Help/paired-FOV session remains at its recorded runtime and scope.

Live host testing remains stopped. The saved Mutter NULL Wayland-resource fault is established, while GMAT client attribution, a repaired/safe host condition and physical-GPU acceptance remain unproven. All nine acceptance gates, external/data/domain limitations and deferred Windows/macOS/MATLAB remain in the [current acceptance map](current-acceptance-20261002.md). Full qualification is not claimed.
