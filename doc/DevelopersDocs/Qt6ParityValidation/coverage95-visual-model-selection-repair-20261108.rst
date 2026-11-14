Visual model filename replacement
=================================

The original 3DS test exposed a stale selection in the Qt file dialog: after
entering the replacement path, selectedFiles returned both the previous OBJ
and the new 3DS. Both files existed and the active filter admitted both.
The model editor used the first returned file.

In the widget dialog, typing a filename now clears the old file-view selection.
The typed path controls existing-file validation. Initial filename convenience,
model filters and native platform dialogs are preserved. All seven original
model/transform cases and the slider catalogue pass headless and visible:
16 executions, with no original test, input event, assertion or timeout changes.

Two superseded trials remain recorded: folder-only initialization passed
headless but stalled a visible probe; changing initialization order alone still
failed the 3DS probe. Neither trial is passing evidence for the final repair.

Qt 6.10.2 implementation research is recorded in repair-plan-v4.json; its
selectedFiles/selectFile/accept paths were inspected in the primary source:
https://raw.githubusercontent.com/qt/qtbase/v6.10.2/src/widgets/dialogs/qfiledialog.cpp

Review 059 and checkpoint 046 bind the evidence in
build/nasa-coverage/coverage95-20261108-visual-picker-current-review.
All 1,334 original inputs remain intact. Canonical acceptance stays 1,078/1,158;
native platform dialogs, hardware GPU and full campaign qualification are open.
