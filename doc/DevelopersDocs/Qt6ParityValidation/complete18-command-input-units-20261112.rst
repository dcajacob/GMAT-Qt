Command numeric-input unit descriptions
======================================

The full-input review found that command forms did not display the units of
their numeric operands. Captions now describe Vary controls, initial values,
perturbations, bounds, maximum steps and scale factors; Achieve goals, values
and tolerances; Minimize objectives; constraint operands; assignments; and
For-loop values. Units follow the selected quantity. Numeric literals inherit
their receiving quantity's units, and user-defined variables and arrays retain
explicit user-defined units. Vary's multiplicative scale has reciprocal
control units.

The lookup reads existing metadata or an unnamed, unregistered parameter
prototype. It does not evaluate expressions, execute commands, register a
parameter instance or convert values. Unresolved and arbitrary expressions
remain explicitly described instead of being assigned a fabricated physical
unit. A missing DryMass metadata fallback was repaired using its documented
kilogram definition.

Ten command cases check 40 initial and changed-control observations per full
profile. Editing a Vary control from X to ECC updates the related captions
from kilometres and reciprocal kilometres to dimensionless. The configured
object graph, source file, main editor history and parameter name/type inventory
remain unchanged. No command is applied and no mission runs.

The first run retained 40 missing-label failures. The next run provided 37
passing observations and exposed three missing kilogram descriptions. Only
those three observations were rerun after repair, followed by the full visible
unit profile. This yields 80 passing observations and 43 preserved failures.
The original numerical assertions and protected inputs remain unchanged.

Evidence is in
``build/nasa-coverage/complete18-20261112-command-input-units-v1``.
Its immutable replay checks source/runtime provenance and rejects stale
control units and missing reciprocal scale units without launching native
tests. Remaining propagation, condition, comparison and full-domain joins
stay open. Coverage remains 1,111/1,158 and 10/18 agreed rows.
