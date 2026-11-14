Managed-array clone self references and Resource operations review
================================================================

Cloning a managed array formula grid now rebinds references to that array itself.
Previously only assignment targets changed. An ordered two-cell initializer
produced [19,2] in the original but [21,2] in its copy because the copied formula
read the completed original array. The valid baseline reproduces this for Cells
and X. The repaired clone produces [19,2] independently in both cases.

The change is confined to the copied GUI-managed initializer. Its lexer skips
quoted text/comments and replaces object-root identifiers rather than terminal
properties or identifier fragments. The X case preserves Anchor.X and XMore;
both cases preserve an external array and unrelated literal comment. Ordinary
mission assignments remain unchanged. The Resource Tree guide now explains own
versus external array references. No native instance layout, virtual ordering,
scientific equation, original assertion or tolerance changed.

New native-widget checks cover the initial run, repeat without Build and Unicode
save/reopen. A third case copies an initializer from an included file through
the existing explicit copy-includes policy. Both arrays retain the independent
result and the included file remains byte-identical. Clone Undo/Redo/source/file
isolation and raw report values are checked. All three cases pass in each runner
profile, with nine execution phases per profile. The included variant was added
after repair; no pre-repair baseline is claimed for it.

Existing checks and retained failures
------------------------------------

Eight full-field page/resource checks, the ReportFile reference case, thirteen
policy cases, seven include cases and the generic clone workflow pass. They
retain their bounded field/default/reference/independent-edit/deletion/history/
reopen/continuation assertions. Standard restrictions plus explicit propagator,
built-in frame/point/default-body policies are unchanged. These are finite
configuration and source contracts, not every native graph or physical domain.

The original Barycenter full-inventory case remains a failure: its declared four
editable fields omit the newer EphemerisSource field. It is not counted as a
pass and neither its fixture nor driver changed. The source mode is conditional:
actual native choices are MassWeighted and SolarSystemBarycenter; BodyNames is
read-only in the latter. SPICE is not a choice of this property.

A supplementary fixture pins MassWeighted as an explicitly checked invariant.
A generated driver extends only the strict inventory comparison to include it;
every original four-field/control change assertion and all full native-property
copy comparisons remain byte-identical. The complete five-field native map is
compared through cloning and independent edits. The mode is not claimed to have
been changed or its GUI alternatives qualified. Reversing the generated edits
recovers the original driver exactly; removing the invariant/metadata restores
the original fixture structure. This supplementary case passes in both profiles.

All earlier witness problems are retained. The first self-reference launch used
the generic offscreen runner despite Rig's native-widget requirement. The next
fixture put a comment inside a strictly managed expression block and was rejected.
Both were corrected before the valid numeric baseline. The first supplementary
Barycenter attempt asked ChangeAll to change the source mode to its current value;
its valid guard failed. The corrected invariant witness preserves every original
field-change assertion. These setup attempts are not product baselines or passes.

The final explicitly composed selection has 34 PASS / 1 retained original FAIL
per profile, or 68 passing executions. Original regression receipts are combined
with the corrected supplementary and final three self-reference cases; all earlier
runs remain visible. Product MainWindow/help hashes match across those repaired
runs. All builds/tests are terminal. All 1,334 protected inputs match baseline;
605 current source owners and the generated driver/receipt are archived.

Scope and continuation
----------------------

The literal operations parent references FRAC-4.5, absent from current workbook
identifiers. Exact text and all 40 adjacent resource/operation rows remain in the
immutable plan. Current GUI objects and all 33 adjacent resource types remain
in scope. The 570 native factory entries include non-resource categories and do
not alone define a GUI clone universe. Full property/reference/attached graphs,
conditional/provider/scientific/native OS/platform/current-build and whole parent
requirements remain open. No private SDK storage or native physics was invented.

Canonical acceptance remains 1,078/1,158 (93.1%): 80 open, 23 more needed for 95%.
This component adds zero row credit; no admission reader reran. Continue
without a user handoff to the Mission Tree plan retaining all 63 literal
E160-E222 command, solver/branch, operation and state-summary context rows.

Artifacts: build/nasa-coverage/coverage95-20261108-resource-cloning-operations-current-review.
