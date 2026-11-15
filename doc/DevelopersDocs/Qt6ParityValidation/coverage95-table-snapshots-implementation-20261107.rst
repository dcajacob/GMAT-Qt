Dynamic table snapshot layout and positional appearance
======================================================

DynamicDataDisplay explicit Save snapshots now emit AddParameters = {} before rebuilding their rows, preventing earlier rows/cells from surviving a shorter or empty appended snapshot. The native scalar/index-zero empty setter reuses SetParamSettings and its existing wrapper-release policy, then resets layout selectors/counts/name caches. Interpreter preserves this exact empty-list operation before generic reference lookup. Four appended write-only row properties retain backgrounds, explicit Auto/Manual text-color mode and warning/critical bounds by cell position, including blank cells and fixed manual black. Explicit snapshots use those complete row forms instead of name-based background/bound fragments. Ordinary serialization adds the forms only when older syntax cannot represent the configured appearance. Existing parameter IDs, class instance fields/virtual slots, append behavior without reset, UpdateData alert formulas, copy/initialization and invocation/reference binding methods remain unchanged. Existing warning/critical setters now check for exactly two bounds before indexing. Five source/help paths compile and two help XML files parse. New row forms require an updated reader. Runtime table reload/rendering, class-wide subscriber lifecycle and the complete any-Resource/function/campaign scope remain open. No row or verification credit.

Reconstruction trigger
----------------------

The native AddParameter method chooses the current length of an existing row as its next column. It also skips a nonblank name already present in the table. Re-reading a saved row therefore could append cells or leave earlier names/settings, rather than reproduce the later resource. Explicit Save now writes a complete empty AddParameters assignment before every table layout, including empty layouts. Ordinary additions retain their existing cumulative behavior.

The empty setter delegates grid replacement and wrapper cleanup to SetParamSettings, then resets counters, selectors, cached reference names and the one-time display-error flag. Interpreter recognizes only the DynamicDataDisplay AddParameters empty-list case before generic reference creation. Shared Subscriber wrapper-release and invocation lease policies remain unchanged; no parallel ownership implementation was introduced. Empty rows and blank cells are then recreated through the existing row/parameter syntax.

Positional appearance
---------------------

RowBackgroundColors, RowTextColorModes, RowWarnBounds and RowCritBounds append four native parameter identifiers. Their lists start with an existing one-based row number and address consecutive cells. Names are not used for lookup, so separate blank cells retain separate configuration. Row modes select Auto or Manual, including a manual black text color. Auto resets the initial text color to black and lets the unchanged UpdateData method apply warning/critical colors. Existing name/row color setters retain their black/zero interpretation.

Explicit snapshots write all four positional forms for every row, including native default values. The older name-based background/warning/critical fragments are skipped in that scope. Outside it, ordinary script serialization adds a positional form only for appearance that the older syntax cannot express: blank-cell settings or manual black. Nonblank, representable legacy forms remain unchanged. Derived warning/critical output colors remain automatic configuration rather than serialized manual overrides. Bounds use existing stream precision and native range conversion/order policy; no numeric domain or tolerance change is inferred. New row methods check row/column bounds and paired bound structure. Existing named warning/critical setters now check length before indexing their parsed range.

Ownership and compatibility
---------------------------

Existing parameter IDs remain in their original order. Only four enum entries and one private nonvirtual helper are added to the header; normalizing those additions reproduces the preimage exactly. Instance fields and virtual slots do not change. The existing grid replacement, copy/initialization, append/duplicate suppression, native UpdateData and invocation/reference-binding methods compare byte-identical. Snapshot serialization only writes a stream and does not call the new reset on the live resource. Save still prepares every export before file I/O. New row properties require the updated reader; older engines are not qualified to read them. Full runtime reconstruction, wrapper/context lifetime, background/UI rendering and broader original subscriber/function/resource persistence remain unqualified.

Compilation and retained evidence
---------------------------------

Configured build v1 returned 0 in 32.09 seconds; last printed progress 75/75. Session 16517 is terminal. No compiler diagnostics.

Five source/help preimages, four unchanged references and four planning snapshots preceded edits. Indexed patches, before/build/final hashes, source/research notes, build log and terminal receipt remain in the stage. Final sources match the successful compiler capture, changed-line whitespace is clean, and Save plus DynamicDataDisplay help XML parse. Existing configured test/probe binaries were relinked as dependencies; none were executed.

No table/serializer/helper/script/GUI/MATLAB/function/mission/numerical runtime tests, new tests, Save/reload round trips, bounds/color evaluations, renderer checks, acceptance auditors/readers/seals or evidence admission occurred. Original requirements, fingerprints, source text, fixtures, assertions, tolerances, denominator, gates and dispositions remain unchanged. FRC!E132 retains its original disposition. Tracking-v119 retains 1093/1158 (94.39%), 65 unresolved engineering rows; source-versioned verified baseline 1053/1158 (90.9%), all 105 original acceptance rows open. The complete implementation audit and goal remain incomplete.

Next implementation
-------------------

Continue remaining specialized any-Resource serializers and native defaults/lists, equation/alias/input policies, hidden/read-only reconstruction, reference graphs and external-file portability. Keep dynamic table reconstruction and reader compatibility in the deferred verification list. Review the original function I/O/scoping/resource/property/output staging and command/solver/estimator semantics against owning source, including retained dependencies and supported caller/session lifetimes. Continue body/frame/kernel/ephemeris, resource-tree, representative POV/exporter scope and every explicit specification/external/historical requirement. Preserve original assertions and defer runtime verification until the implementation pass is complete.
