Dimensionless input labels, 2026-11-11
=====================================

Six GUI source files now supply explicit dimensionless labels for independently
defined ratios, normalized/reference-vector components, orbital eccentricity and
equinoctial components, rendering scales, opacity/phase values and tank mixture
weights. The classification is explicit; an unknown or blank native unit tag
does not automatically become dimensionless. Orbit-selector refreshes retain
the labels, and the specialist body, model-scale and view-scale forms display
them. Native unit getters, scalar/vector values, equations and numerical bounds
are unchanged.

The supplementary fixture was frozen before the repair. Its41 cases cover eight
hardware factory spellings, six orbital representations, two attitude models,
eleven default bodies, four user-body factory types and the remaining selected
scalar/model controls. Each profile checks147 unit observations, including
15 repeated orbital element checks after representation-selector round trips.
All five groups pass in private native X11 and on the established watchable
Xephyr desktop:294 observations across ten GUI executions. The watchable outer
window starts minimized and remains resizable. No mission is executed.

Every observation preserves the authored source, undo history, saved file and
native resource serialization. The independent evidence reader checks the exact
frozen field/case census, raw observations, stdout, source archives, terminal
receipts and real window exposure. Four corrupt-evidence controls are rejected.
The before/after compiled-source inventory has the same357 input-construction
sites across191 source/header files. These are templates, not a count or
qualification of every instantiated input field.

One directly affected original orbit GUI test was rebuilt and run unchanged.
It fails at its old exact empty-unit expectation for ECC. The displayed value
is0.19999999999999993 against0.2, an error of8.33e-17 within its unchanged1e-6
numeric bound. The new visible unit is ``dimensionless``. That original failure
and test are preserved; it is not counted as passed or used as whole-current-
build qualification. Earlier numerical conversion passes retain their own
executed source versions. The earlier supplementary navigation/launcher failures
and the stale-review preflight failure also remain recorded.

All1,334 protected original test inputs remain unchanged. No full regression or
scientific qualification was run. Artifact root:
``build/nasa-coverage/complete18-20261111-dimensionless-inputs-v1``.

All Others!E13 remains open. Its complete non-dimensional input obligation still
needs the count/identifier, solver/estimator, expression/goal, array/matrix and
conditional-coefficient meanings reconciled against the saved field inventories
and current input-construction closure. The447 unreconciled numeric records in
the saved inventory include physical fields and already passing evidence; they
are a meaning-review queue, not447 defects or planned tests. Existing matching
passes are to be reused. Canonical coverage remains1,104/1,158 with3 of the18
agreed rows complete. This engineering batch awards no partial row credit.
