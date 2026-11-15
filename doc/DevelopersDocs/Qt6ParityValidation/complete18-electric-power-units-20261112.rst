Electric-thruster and power-system input units
=============================================

The electric-thruster coefficient table omitted units and its separate editor
displayed ``See Docs``. Both now show the units derived from the native
polynomials: mN and mg/s, divided by the appropriate power of input kW.
The native equations, coefficient metadata and public interfaces are unchanged.

A targeted model-switch check also exposed a stale coefficient-editor button.
It listened to a hidden Advanced selector instead of the active Setup control.
The button now follows the pending model, enabling for ThrustMassPolynomial and
disabling for ConstantThrustAndIsp and FixedEfficiency before Apply. Discarding
the draft preserves native objects, source, saved files and undo history.

The batch contains 104 passing GUI observations across headless and visible
profiles: ten coefficients in both the Advanced table and coefficient dialog,
plus six physical fields in each of two power-system types. Both profiles also
check five model-availability states. Previously passing headless power and
default-polynomial observations are reused at their recorded source versions.
The new headless check runs only the failed model-switch case. Twenty original
unit failures, one initial supplementary-worker interaction failure and the
model-switch failure remain in their original receipts.

Independent replay checks the frozen guide definitions and native scaling,
source/build/execution identities, exact narrow production changes, raw results
and rejection of wrong scales or incomplete model transitions. No mission or
full regression runs. Original protected assertions and tolerances are intact;
the previously authorized ASCII window-title change remains the sole exception
in that protected-input inventory.

This completes 22 saved numeric-unit worklist entries, leaving 182 for review.
That number is neither a defect count nor the complete current input universe.
All Others E13 remains open. Canonical acceptance stays at 1,111/1,158, ten of
eighteen agreed requirements; this stage claims no new requirement credit or
whole-current-build qualification.

Evidence is sealed under
``build/nasa-coverage/complete18-20261112-electric-power-units-v1``.
