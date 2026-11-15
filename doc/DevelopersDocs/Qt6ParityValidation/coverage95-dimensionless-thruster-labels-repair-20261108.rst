Dimensionless thruster labels and Units wording review
=====================================================

The Qt resource metadata and specialist thruster form now label direction
components, duty cycle, thrust scale factor and electric fixed efficiency as
``dimensionless``. Their shipped guide entries define direction-vector components
and multiplicative/duty/efficiency ratios explicitly. The native numerical values,
ranges, defaults, setter behavior and equations are unchanged. Other physical-unit
labels and the coefficient arrays retain their existing conventions.

The existing hardware-unit GUI driver runs unchanged against two additional
fixture cases. All eleven resource/field pairs are checked against their actual
shipped guide Units entries. Both new baseline cases fail at the real duty-cycle
label, which displayed only its range. Both pass after the repair. Four original
hardware-unit cases also pass, including physical burn/thruster labels and all
32 chemical coefficient rows in the advanced table and coefficient dialog.

The final offscreen and visible runs each pass six cases. Each observes 118 unit
labels: 54 primary controls, 32 advanced coefficient cells and 32 dialog cells.
Four actual model-popup changes per profile exercise conditional electric
controls; disabled controls keep their labels. These twelve executions and 236
observations preserve native/source/history/file isolation and the independent
literal sentinel mission (42). They do not establish thruster performance science
or arbitrary input-unit semantics. New dimensionless advanced-table values follow
the shared metadata route; their individual cells were not directly asserted.

The documentation inventory parses 130 current reference documents: 772 GUI-tagged
table fields have 770 explicit Units strings. The two blank entries are in the
command template. These are documentation table counts, including duplicated,
legacy and template rows; they are not an inventory or semantic qualification of
all active controls. None/N/A/blank strings do not establish dimensionless data.

All Others E13 literally says non-dimensional input fields. That wording and its
fingerprint are retained, alongside E14-E16 and their Suggestion to Remove text.
The full all-input obligation and intended dimensional interpretation remain open.
The guide K6/K8/K10 C-exponent versus native K-exponent discrepancy also remains
open. No coefficient units or formulas were silently changed to resolve it.

All 1,334 protected original inputs are unchanged. Twenty-six current owners are
archived. No root layout, virtual ordering, scientific equation or tolerance
changed. All build/test handles are terminal. This finite component adds zero
requirement credit: canonical acceptance remains 1,078/1,158 (93.1%), with 80 open
and 23 more required for 95%. No admission reader reran.

Artifacts: build/nasa-coverage/coverage95-20261108-units-wording-current-review.
Continue automatically to the Resource Tree inventory plan, retaining all 33
original resource children, interface service and Clone/Rename/Delete context.
