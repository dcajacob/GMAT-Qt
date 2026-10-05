# Actual OF script ordering — 2026-10-05

`DrawFontPosition` now preserves the actual script setter's cyclic counter across scalar, full, partial and repeated arrays. View, Vector and SensorMask script selections now append unique names in source order rather than applying the separate direct whole-list helpers' clearing behavior. Empty braced lists and unquoted empty tokens are no-ops. This supersedes the font-position pending item in the [model-file checkpoint](model-drawing-20261004.md), and older direct-helper reset/prefix claims at this bounded script-conversion scope.

## Contract and implementation

The inspected OF checkout is `0e78aa50ae01fa234155ac105286004ad03884a5`. Its `OpenFramesInterface.cpp:115–118` declares Add/View/Vector/SensorMask as OBJECTARRAY, and `:133` declares DrawFontPosition as STRINGARRAY. GMAT's interpreter separates arrays (`src/base/interpreter/Interpreter.cpp:6832–6862,6999`), with STRINGARRAY entries reaching the scalar setter through `SetPropertyStringValue` (`:6608–6622`) and OBJECTARRAY entries reaching indexed setters. The font scalar setter advances/wraps its own counter (`OpenFramesInterface.cpp:1324–1333`). Indexed View/Vector/SensorMask setters append (`:1463–1468`); AddView/AddVector/AddMask ignore duplicates and blank names (`:1732–1794`). Direct whole-list helpers have different prefix/clear semantics and are not the script path.

The converter now uses the existing interpreter-compatible tokenization for these fields. For two objects A/B, `DrawFontPosition = {'Top-Left'}` followed by `{'Bottom-Left'}` assigns A/B respectively. The previous converter restarted both arrays at A. Mixed scalar/array entries, wraparound, appended/duplicate Add, empty lists, unquoted whitespace arrays, and independent model/font/per-plot counters have focused checks. Unsupported placements retain the original source with a specific conversion error; invalid entries beyond the initial Add count are no longer silently ignored.

Repeated view selections retain the first selected primary camera and append additional unique views. Vectors and masks retain earlier selections across later arrays and empty assignments. The existing Qt metadata editor's removal controls still explicitly remove selections; this change concerns imported OF script statements. No resource schema, renderer geometry, mission engine, propagation algorithm or numerical tolerance changed.

Implementation: `src/qtgui/ScriptCompatibility.cpp`; affected checks are in ConvertedVisibilityTests, SegmentCameraTests, OpenFramesVectorTests and SensorMaskBackendTests. Source snapshots and SHA256 fingerprints are in the receipt/archive.

## Reproduced defect and relevant checks

The new partial-array assertion first failed against the unchanged implementation in **FontMetadata 0.07s**: “Actual script partial font-position arrays did not continue the scalar counter.” It then passed after repair.

Two directly affected older assertions required corrections, both retained in the raw logs. ConvertedFonts initially rejected a trailing comma as invalid; the actual STRINGARRAY helper treats that empty token as a no-op. OpenFramesSyntax initially rejected `{First Close}`; the actual object-array interpreter accepts whitespace-separated names. These now have positive expectations, while unsupported, compound and dynamic source rejection remains covered. Repeated-list reset assertions were corrected to actual append semantics. The production fix was not weakened to satisfy the old assumptions.

Six relevant checks pass across recorded builds; only affected/failing checks were repeated as the concrete changes required.

| Check | Latest relevant result | Raw log |
| --- | --- | --- |
| QtGui.ConvertedFonts | PASS 0.81s | tests-03.log |
| QtGui.FontMetadata | PASS 0.07s | tests-01.log |
| QtGui.OpenFramesSyntax | PASS 0.18s | tests-04.log |
| QtGui.OpenFramesVectors | PASS 0.41s | tests-03.log |
| QtGui.SegmentCameras | PASS 0.54s | tests-03.log |
| QtGui.SensorMaskBackend | PASS 0.33s | tests-03.log |

FontMetadata's pass precedes a fixture-only rebuild; its executable hash was not captured at that instant. SegmentCameras' pass precedes the syntax-only fixture correction, with production code unchanged. They remain bound to those logged builds and are not labeled as fresh passes of later test binaries. ConvertedFonts was rerun after the view-selection production change because its conversion/UI workflow uses views. The final isolated OpenFramesSyntax pass follows only its syntax-fixture correction.

ConvertedFonts retains its complete reused 19-column numerical reference comparison, actual conversion offer, pending/Cancel/correction/Apply, Undo/Redo, Unicode reopen and font presentation checks. SegmentCameras retains actual four-view replay/close-reopen, engine reference recovery and complete independent report comparisons. SensorMaskBackend retains its complete **131-byte** report comparison, three FOV shapes and engine-replacement lifetime checks. OpenFramesVectors retains static conversions of its three directly affected shipped vector sources plus its separate synthetic numerical/reference and replay checks; no shipped vector mission or full example corpus was rerun. No fresh numerical regime or native-renderer pixel qualification is claimed.

## Runtime and remaining gates

Final actual `application/bin/GmatQt`: **6,740,680 bytes**, SHA256 `78e763656adfc1956508a7a6527f623e864283423debe1747e65b8894a874a72`. Base/util/Qt startup are byte-identical to baseline `e7dd2854…`. Final source and current test binaries are fingerprinted separately from earlier logged passes. Checks used private offscreen Qt; no live desktop or physical GPU was tested.

[Receipt](of-script-ordering-20261005.receipt.json) and [raw archive](of-script-ordering-20261005.raw.zip) retain the baseline failure, all intermediate failures/passes, builds, full CTest details/JUnit, exact source snapshots/patch, reused numerical reference, source-contract hashes and manifest. Archive SHA256 `58ffd256bc910f6eb419d1e219409eb7e04b12c0bdccbaf1ef8bb319698737e5`; every manifest member hash was verified.

All nine original acceptance gates remain authoritative. Completed audits, tutorial walks and example corpus were not repeated. Held Lunar/L2, external data/integration and scientific limits remain explicit. Windows/macOS/MATLAB remain deferred. Live host testing stays stopped; the saved Mutter null Wayland resource finding does not establish GMAT client attribution or host/GPU acceptance. **Full qualification remains unproven.** The original `/home/dan/GIT/GMAT/GMAT` checkout is untouched.
