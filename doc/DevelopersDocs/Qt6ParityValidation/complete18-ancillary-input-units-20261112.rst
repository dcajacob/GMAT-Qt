Propagation, condition and comparison units
==========================================

Single-stop propagation captions now show the selected quantity's units for
the parameter, goal and stop tolerance. Changing seconds to days updates all
three captions. Variable stops retain user-defined units. Apsis events show
no goal and a dimensionless residual/tolerance, matching the native dot
product of normalized position and velocity vectors.

The multiple-stop dialog displays units for its selected row. The condition
dialog displays units for both selected operands, with numeric literals
inheriting the other operand's units. Editing an operand refreshes the unit
description. Numerical file and folder comparison tolerances are labeled in
the units of the compared values/columns; the tooltips state that no automatic
unit conversion is performed.

The changes affect captions and explanatory labels only. Native algorithms,
command serialization, condition operators and input values are unchanged.
The supplementary worker verifies exact stopping/condition source preservation,
native object and parameter-name/type inventories, saved files and editor
history. It does not run propagation, comparisons or folder missions.

The complete pre-repair run retained 24 missing-unit observations. The repaired
headless and visible runs provide 48 passing observations. An earlier test
interaction failure is preserved; the worker was corrected to select the
condition cell before opening its editor. No original assertion was changed.

Evidence is in
``build/nasa-coverage/complete18-20261112-ancillary-input-units-v1``.
Frozen replay also rejects incorrect time and apsis units without native tests.
Numeric picker context and the independent full input-domain evidence join
remain open. Coverage stays at 1,111/1,158 and 10/18 agreed rows.
