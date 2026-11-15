Dynamic data and XY plot units — targeted evidence
=================================================

Dynamic-data warning and critical bound captions now follow the chosen
parameter's units as the reference is edited. Bounds remain stored numeric
values; no conversion or compatibility inference is performed. Native dynamic
updates compare these bounds directly with the parameter value.

XY style controls display the corresponding axis's data units for Minimum and
Maximum. Arbitrary axis-label text is not treated as dimensional metadata.
Line width and marker size captions state pixels; a tooltip specifies Qt
logical pixels, independent of the data coordinate system.

The frozen before run reproduced 14 missing-caption failures. Headless and
visible after runs passed 28 observations total, including a live position to
mass reference change. Inspection and Cancel preserve native objects, source,
undo history, the saved script, parameter inventories, dynamic bounds and
pending settings, and plot axis/style/sample data. No mission ran. Both
production changes are caption-only and native library hashes stayed fixed.

The evidence manifest preserves raw failures, source archives, build/runtime
identities, expected observations and negative controls. Its reader replays
without launching native tests. This contributes no whole-row credit: the
full All Others!E13 default/configured/nested/command conjunction remains open.
Canonical coverage is 1,111/1,158, with 10/18 agreed rows complete.
