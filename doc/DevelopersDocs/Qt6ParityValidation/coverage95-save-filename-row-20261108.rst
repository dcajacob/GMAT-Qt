Save filename acceptance, 2026-11-08
===================================

Exact original FRC!E135 is admitted: Save defaults to ObjectName.ObjectType.
Coverage is1076/1158 (92.9%), with82 original gaps and25 more needed for the
unchanged1101 target. Historical operations remain1816/2367. Full current-build,
NASA, scientific/core, physical-frame, external-ABI, platform and native-OS-input
qualification gates remain open. Universal any-Resource Save FRC!E132 remains open.

Behavior and verification
-------------------------

The existing implementation uses Worksheet as the native and GUI default, with
an empty explicit Filename. Single-root Save writes Saved.Variable; selecting
Saved, Grid and Craft writes Saved.Variable, Grid.Array and Craft.Spacecraft.
Explicit LegacyData retains Saved.Variable.data for one root and the combined
Saved_Grid_Craft.data for multiple roots. No production code changed in this batch.

Four new native cases pass in each of the headless and visible profiles. They
exercise the actual Mission-tree command editor, ordered root selection through
the selector, pending style changes, Unicode/apostrophe explicit paths, style
disable/re-enable, Discard, Apply, exact Undo/Redo, Unicode save/reopen and native
command cloning. Real execution checks the exact output-directory membership,
nonempty files, selected root identities and bounded scalar/array/spacecraft
values. The saved scalar is19 and its successor reaches42 without modifying the
mission source. The eight cases produce12 files and16 selected-root occurrences.

Both final profiles use identical source and binary inventories. The explicit
1440-source/32-ELF before/after guard is unchanged. Visible Xephyr starts minimized,
resizable and nonfullscreen, with74 exposure events and no screenshots. These
Qt widget events do not establish native OS input or cross-platform acceptance.

Preserved failures and admission limits
--------------------------------------

Four earlier visible failures in the new test are retained. A modal selector could
leave QMdiArea.activeSubWindow null even though the retained editor pane existed.
The new Discard helper now finds and closes the pane by its widget identity.
Only the new helper changed; no product code, original assertion or filename
expectation changed. Both final profiles were then run on that same test version.

Five original implicit-.data failures are reused from their actual retained logs
and receipts, without native reruns or waivers. The three tree cases fail their
saved_quantities expectation; the two output-operation cases report Operation
input absent at the hardcoded .data read. Their historical overlay summaries have
stale failure_signature strings. The actual logs and unchanged original sources
are authoritative and are explicitly hash-bound by this admission. They are not
reported passing. Compatibility is available through explicit LegacyData; no
duplicate output files were added to satisfy the old default assumptions.

All1334 protected original inputs, assertions, fixtures and tolerances match the
baseline. The59 formal hold identities/statuses remain42 resolved/17 unresolved;
the37 MATLAB identities and6 unresolved MATLAB holds remain unchanged. This row
is outside that hold inventory. Only its full-audit/closure record changes; the
other1157 requirement and138 closure records remain identical. Source text,
fingerprints, denominators and all broader gates are retained. The separate
all-properties and attachment rows receive no duplicate credit, and selected
Variable/Array/Spacecraft examples do not prove serialization of every resource.

Evidence
---------

Native component: ``build/nasa-coverage/coverage95-20261108-save-filename``.
Canonical admission: ``build/nasa-coverage/coverage95-20261108-save-filename-row-v2``.
The admission includes70 frozen readers, source archives, exact row/delta audits,
a byte-identical ledger replay and the sealed manifest/receipt/prefix. The retained
row-v1 preparation is unsealed: static review caught a missing json import in the
new reader before its CLI ran, and row-v2 was prepared from the corrected reader.
No additional native execution is needed for admission or frozen replay.

The campaign remains active. Continue from the exact remaining-requirement audit;
keep unresolved specifications open and do not repeat completed matrices without
a relevant source change or newly demonstrated defect.
