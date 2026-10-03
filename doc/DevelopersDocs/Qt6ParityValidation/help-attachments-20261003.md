# Readable Help script attachments and Find Return — 2026-10-03

**Passed at bounded private-GUI scope.** Linked local `.script` and `.gmf` documents now display as read-only literal text, preserving line breaks, indentation, tabs, Unicode and markup. Their original file URLs and Back/Forward history remain. Return in Help Find searches the current document without also activating Back, another navigation button or Close. Viewing an attachment does not import or execute a mission.

## Concrete baselines and repair

The first [publication](help-publication-20261003.md) native process on SHA483ed090… successfully opened ReportTutorial.script at the corrected local path, but screenshot005 showed collapsed lines. The new focused fixture independently failed in0.06s and also caught interpreted `<tag>` text. A first escaped bare-pre candidate preserved content but Qt discarded its final newline; that failed0.05s check and source snapshot remain.

HelpBrowser now overrides only local `.script`/`.gmf` HTML-resource loads, validates UTF-8 and escapes literal content into a pre/span wrapper. Other HTML/image/style and existing external-link handling retain their routes. Existing Help load validation preserves missing/malformed pending URLs and Retry. The span preserves boundary blank lines; the browser retains its source URLs and history.

The first repaired app, SHA80435b96…, passed the literal-content fixture and showed readable lines in a separate native process (`help-attachments/native/isolated-x11-1ad7dl9x`). Its actual Ctrl+F/text/Return check then exposed another real defect: Return also activated the dialog auto-default Back button. Screenshot003 shows the chapter instead of the attachment. This session closed0/0/0; `native-display-and-search-failure.json` preserves the failure rather than claiming it as passing Find/history evidence. The new actual-key fixture reproduced it in0.06s. Disabling autoDefault/default on the Help dialog's buttons preserves ordinary clicks/Space and the existing Find returnPressed action without that extra activation.

## Final rebuilt application and affected checks

`application/bin/GmatQt` resolves to GmatQt-R2026a, **6,146,632 bytes**, SHA256 `12a9dddf3cbdd63a729df8858a5cdde0393bd8e25443549b153d90d67af2abe8`. The final actual app rebuild exits0. Base `2082a341f5f123e81bad362972cd2b28a0a4753047ae51b5c09609e246be430c`, util `f9844517a2de0dfa1ddbd7b8180a63452aa6917a8fcd0b87217f304547a717a8` and startup `5f80be1f2dbe46faeb6d226328cace194d7770d086d5447c8b44928fb858559a` remain byte-identical.

Final **QtGui.HelpAttachments passes0.06s**; directly affected **QtGui.Help passes0.46s**. The new fixture needs no engine initialization. It checks exact script/function contents (leading/trailing newlines, tabs, Unicode and `<>&`), no-trailing-newline GMF, literal Find, actual Return/keypad Enter after navigation/search focus, retained URL/selection, read-only view, Back/Forward, unchanged source bytes and missing/invalid UTF-8 recovery/Retry. The affected existing Help check covers ordinary HTML/topic/F1/button/modal/search/history/pending-source and recovery behavior. No unchanged numerical mission, old walkthrough or full suite was rerun.

Raw evidence is in `build/example-qualification/20261003/extra-help/publication/help-attachments`; `find-return/final-runtime.json` binds final source/test/app/dependency hashes and all final logs. Earlier baseline/first candidate/80435 receipts remain at their actual identities. An independent read-only diff review found no actionable ordinary regression; that review does not substitute for these checks.

## Final actual-input retry

`find-return/native/isolated-x11-yhavtygp` and `find-return/native-result.json` bind actual input to final SHA12a9dddf… and the unchanged original startup. The clone changes OUTPUT_PATH only. Screenshot002 shows readable ReportTutorial lines;003 shows BeginMissionSequence selected after Ctrl+F and Return while the attachment remains open.004 returns to the chapter;005 returns forward to the attachment. The Help/Welcome windows close normally, then main Alt+F4 produces application/WM/Xvfb **0/0/0**. No attachment was executed or imported. This closes the two observed ordinary Help usability findings; other chapter walkthrough evidence remains bound to its original app.

The18-chapter offline publication, Lunar/L2 holds, external/scientific limitations and all nine original gates remain unchanged. Live host testing stays stopped; GMAT client attribution, safe host condition and physical GPU/driver acceptance remain unproven. Windows/macOS/MATLAB remain deferred. Full replacement qualification is not claimed.
