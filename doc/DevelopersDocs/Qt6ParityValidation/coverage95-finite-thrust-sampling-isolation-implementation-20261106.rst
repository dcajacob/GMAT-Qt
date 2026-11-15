Finite-thrust sampling isolation — implementation, 2026-11-06
==============================================================

Per-spacecraft finite-thrust queries now evaluate private spacecraft and burn
copies. Previously the query called the live burn's maneuver-start setter,
which fires, and then Fire again. It could leave a shared burn pointing at a
short-lived private spacecraft and replace its reported/last-fire data. The
sampling path no longer retargets that live burn. Complete target identity,
function isolation and scientific qualification remain open.

Private evaluation
-------------------

GetDerivativesForSpacecraft retains the configured target-name filter and zero
result for unselected/empty-burn queries. It guards a native spacecraft clone and
initializes it through the existing private-copy path, avoiding ConfigManager
hardware reassociation. Each selected burn uses the new nonvirtual const
EvaluateForSpacecraft helper. That helper guards a private native burn clone,
binds the scratch spacecraft directly, clears evaluation-only thruster/type/
initialization/epoch/state/mass/origin caches and calls Fire once at the sample
epoch. It does not invoke the maneuver-start setter just to bind a target.

The live burn retains its target, reported vectors/mass-flow data, last-fire
markers, initialization and derivative overrides. Normal throttle/hardware
calculation acts on scratch spacecraft components. Native initialization can
still touch shared SolarSystem/kernel/frame/provider caches; no universal
immutability claim is made. RAII releases private evaluations on errors. Native
maneuver-start and actual integration behavior remain; the acceleration/mass-flow
formulas, tolerances and expected values are unchanged. Scratch copy/init cost
and numerical/precision behavior remain unmeasured.

Remaining scope
----------------

mySpacecraft name matching is unchanged. Exact configured-target identity,
same-name independent values, alias/provenance compatibility and full live burn
caller/function lifetime still need work. The propagated spacecraft array is not
an immutable configured-target registry. EphemerisFile still attaches caller
live transients using native names; its private graphs intentionally do not
freeze that live context. Private graph children may have no attached context
because a child-to-owner lease would create a cycle. A blanket pointer-equality
replacement would therefore not prove full sampling-alias compatibility.
Source findings are retained in next-target-identity-source-research.json.

Other standalone burn/hardware, PropSetup/PSM, GroundStation/event/subscriber/
provider graphs, globals/private bodies/kernels, deferred configuration, Parameter/
type/cache/recursive/exceptional scoping, later owner replacement and cross-Sandbox/
scientific body/frame/command domains remain. E452/E460 entire objects and E466
input preservation are unchanged. No broad parent row is credited or scope waived.

Compilation and retained work
-------------------------------

The private integrated build passes 77 steps / 139.97 s without warnings. Core,
Qt, console, plugins and Python bindings compile/link. Final sources match v1;
session 82555 is closed, no live build. Three exact preimages, before/at-build/
final hashes, focused patch, source notes/research, log/receipt and planning
preimages are under
``build/nasa-coverage/coverage95-20261106-implementation-finite-thrust-sampling-isolation``.
No existing resource field or virtual slot is added; the helper is nonvirtual.
External binary/cross-platform behavior remains unqualified.

No tests, GUI, mission/function/model/solver execution, numerical comparisons,
acceptance readers or seals ran. Original assertions, fixtures, tolerances,
requirement texts/fingerprints, denominator, ledger and qualification gates remain.
Tracking stays 1093/1158 (94.39%), 65 other original rows; recorded source-versioned
verified coverage stays 1053/1158 (90.9%), all 105 acceptance rows open. No installed/
shared runtime, package, commit or push changed. Goal active; implementation incomplete.
