Vary numeric control repair, 2026-11-08
======================================

Vary now keeps numeric controls and numerical options independent when their
authored text is identical. The Qt selector exposes finite numeric controls
through an explicit option and preserves that mode when reopening a number.
Canonical acceptance remains 1078/1158 (93.1%); this stage admits no whole row.

The initial twelve-case native witness passed six and failed six. A control
spelled 1 shared its mutable NumberWrapper with options also spelled 1. Native
Target RunInitialGuess therefore changed the control and its scale/options,
producing -0.5 instead of the independent expected physical value 2. Rebinding
equal numeric text also leaked wrappers. SetElementWrapper now stages separate
numeric clones for each role, publishes them only after complete successful
staging, and retires previous bindings by pointer identity. ClearWrappers retains
the retirement queue until deletion. Caller ownership and old bindings survive
a thrown Clone. Explicit named-resource sharing and cold copies remain intact.

That repair passed eleven of the original twelve cases. The remaining case
proved that resetting one of two numeric controls spelled 1 refreshed both
solver columns: [5,5] instead of [5,4]. Numeric Vary refresh now uses its actual
registered solver position and name. Solver's new helper is nonvirtual and
validates the full payload and destination before writing. Existing all-name
refresh remains for named resources, and the native RETAIN no-op stays first.
No class fields, virtual slots, provider layout, equation or tolerance changed.

The final native run passes all thirteen cases: the original twelve plus a
shared-Variable duplicate-control case. It includes physical/scaled native
Target values, signed scientific controls, actual Variable/array/parameter
owners, independent numeric options, repeated binding/destruction, cold copies,
clone-failure rollback, independent numeric resets, retained named-resource
sharing, and atomic invalid indexed refresh rejection.

The new GUI witness passes headless and visible. It chooses a number, signed
scientific number, then a named Variable through the actual selector; checks
Cancel, pending state, Apply, repeated Apply, exact source, Undo/Redo, Unicode
Save/Save As/reopen, reopened controls and actual native mission values. The
three unchanged original GUI checks also pass in each profile: NasaVaryParameter,
NativeNasaVaryOptimizeProperty (real prebuilt VF13ad), and ParameterSelection.
The original default-picker rejection of 17 stays intact. Nonfinite numbers,
String, bare arrays and out-of-range cells remain unavailable.

The first new GUI witness crashed because it reused a retired editor button
after Apply replaced the editor inside the MDI child. GDB identified the second
Apply click in the witness; reacquiring the current editor/button corrected it.
The product source did not change for that correction. The failed run and exact
prior witness remain retained. All builds, native tests, GUI tests and debugger
handles are terminal. All 1334 protected original inputs remain unchanged.

The stage retains all thirty original FRC-2 context rows, including the original
DEFERRED unsupported-setting warning. Full arbitrary scalar/control/solver
families, fmincon integration, complete parent conjunctions, science, platform
and current-build qualification remain open. No denominator, original fixture,
assertion or tolerance changed. The next finite audit group is the full Achieve
control-family parent; no question or approval is pending.
