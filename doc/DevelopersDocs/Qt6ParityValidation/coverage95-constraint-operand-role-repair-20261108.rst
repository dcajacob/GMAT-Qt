Constraint operand role repair, 2026-11-08
=========================================

NonlinearConstraint comparison operands and tolerance now own separate numeric
values when their authored text is identical. Valid native SetField tolerance
updates succeed instead of changing values and then throwing a metadata type
error. Rebinding now retires replaced wrappers for deletion. Canonical acceptance
remains 1078/1158 (93.1%); no complete requirement row is admitted by this stage.

The eight-case native baseline passed the explicit shared-Variable and cold-copy
controls and failed six cases. Three RHS/tolerance setter cases changed all
numeric roles rather than the selected one. SetField(Tolerance,0.25) changed all
three roles to 0.25 and then threw because Tolerance has STRING_TYPE script
metadata. Five repeated bindings leaked four old wrappers. The final failure was
the new failed-clone transaction check, absent from the old direct setter.

The repaired setter stages separate numeric clones, validates their identity
and type, and publishes only after complete cloning and ownership staging.
Existing bindings and incoming caller ownership survive failure. Actual named
Variable/array-element/Parameter references retain sharing. Replaced bindings
and accepted numeric templates retire by pointer identity, and ClearWrappers
keeps that queue until deletion. Successful wrapper-backed SetField updates
return directly; a failed wrapper write reports an error. Production behavior
changes only in NonlinearConstraint.cpp. No public fields, virtual slots, solver
equations, tolerances or physics change. The same eight native cases now pass.

The current cost-function compatibility implementation was already present:
Interpreter selects native Minimize for a comparison-free NonlinearConstraint
argument, while comparisons retain native NonlinearConstraint. Minimize preserves
worksheet spelling and command labels through generation and cloning. Optimize
counts native objectives, so both spellings share its one-objective rule.
CommandForm contains both objective and comparison routes. These are current
source findings; no replacement objective implementation was invented.

Four unchanged existing GUI checks pass in each profile after repair:
ImplementationCostForm, NasaMinimizeProperty, NasaConstraintScalarHyperBdotR,
and NasaConstraintScalarTankFuelMass. CostForm runs both command spellings with
one objective and one comparison, verifies native class/clone/serialization and
the independent constrained optimum X=2,cost=1. MinimizeProperty exercises the
actual standard-spelling objective editor and independent changed optimum.
The two comparison cases check both operand roles, actual property pickers,
source/history/persistence, independent numerical inputs and solver progress.
This does not establish a complete worksheet-spelling GUI edit lifecycle, every
scalar/backend/domain, or all mixed-objective rejection paths. Those verification
scopes remain open along with broader scientific/platform/current-build gates.

The two comparison cases initially stopped before their numerical assertions:
their exact existing SPK fixture was missing from the private project layout.
The original file's SHA-256 was checked against the unchanged case fixture, then
copied into the isolated layout expected by the original tests. Both tests passed
on reexecution. No original input, assertion or tolerance changed; all 1334
protected originals remain intact. Final builds/tests are terminal. Raw failed
setup, native baseline, repaired results, source snapshots and exact physical
fixture setup are retained in the stage.

Stage: build/nasa-coverage/coverage95-20261108-constraint-objective-current-review.
All ten original FRC-6/FRC-7 context rows remain retained. The full finite source
audit continues to numerical Propagate capacity and ephemeris-count policies;
both current compatibility modes must be inspected before inferring absent
implementation from the older wording-conflict classification.
